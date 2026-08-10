# -*- coding:utf-8 -*-

"""
Piloter le moteur C++ au moyen de son protocole textuel.

Le moteur s’exécute comme processus enfant. Les commandes sont écrites sur
son entrée standard et les réponses sont consommées jusqu’à leur ligne
``status`` terminale. Ce module constitue l’unique frontière système de la
GUI : les règles et la recherche restent entièrement dans le moteur C++.
"""

from __future__ import annotations

import subprocess
from dataclasses import dataclass


@dataclass
class EngineResponse:
    """
    Regrouper les informations renvoyées après une commande de partie.

    Attributs:
        ok (bool): Indiquer si la commande a été acceptée.
        fen (str | None): Position résultante lorsqu’elle est fournie.
        ai_move (str | None): Coup UCI choisi par l’IA, le cas échéant.
        status (str): État protocolaire de la partie.
        error (str | None): Message expliquant un échec de commande.
    """

    ok: bool
    fen: str | None = None
    ai_move: str | None = None
    status: str = "status playing"
    error: str | None = None


def fen_to_matrix(fen: str) -> list[list[str]]:
    """
    Développer le placement compact FEN en matrice de 8 × 8 cases.

    Args:
        fen (str): Position FEN complète ou commençant par son placement.

    Returns:
        list[list[str]]: Pièces FEN et points représentant les cases vides.
    """
    placement = fen.split()[0]
    rows: list[list[str]] = []
    for rank in placement.split("/"):
        row: list[str] = []
        for char in rank:
            if char.isdigit():
                row.extend(["."] * int(char))
            else:
                row.append(char)
        rows.append(row)
    return rows


class EngineClient:
    """
    Maintenir un processus moteur et échanger des commandes synchrones.

    Le client conserve les options nécessaires à un redémarrage depuis la
    barre d’outils. Une seule commande doit être en vol à la fois, invariant
    respecté par la vue avant de déléguer le calcul à son thread de travail.
    """

    def __init__(
        self,
        engine_path: str,
        params_path: str,
        depth: int | None = None,
        book_path: str = "",
        book_mode: str = "chill",
        search_mode: str = "",
    ) -> None:
        """
        Initialiser les options et démarrer immédiatement le moteur.

        Args:
            engine_path (str): Chemin du binaire ``chess_engine``.
            params_path (str): Fichier JSON des paramètres d’évaluation.
            depth (int | None): Profondeur principale imposée.
            book_path (str): Chemin de la bibliothèque d’ouvertures.
            book_mode (str): Politique de sélection des ouvertures.
            search_mode (str): Combinaison d’heuristiques de recherche.

        Returns:
            None: Le processus est prêt à recevoir des commandes.

        Raises:
            OSError: Si le système ne peut pas démarrer le binaire.
        """
        self.engine_path: str = engine_path
        self.params_path: str = params_path
        self.depth: int | None = depth
        self.book_path: str = book_path
        self.book_mode: str = book_mode
        self.search_mode: str = search_mode
        self._process: subprocess.Popen[str] | None = None
        self.restart(depth=depth)

    def restart(
        self,
        depth: int | None = None,
        book_mode: str | None = None,
        search_mode: str | None = None,
    ) -> None:
        """
        Redémarrer le moteur avec les réglages éventuellement remplacés.

        Le redémarrage est nécessaire car les options de recherche sont des
        arguments du processus et non des commandes mutables du protocole.

        Args:
            depth (int | None): Nouvelle profondeur, si précisée.
            book_mode (str | None): Nouveau mode d’ouverture, si précisé.
            search_mode (str | None): Nouveau mode de recherche, si précisé.

        Returns:
            None: Le nouveau processus remplace l’ancien.

        Raises:
            OSError: Si le nouveau processus ne peut pas être créé.
        """
        self.close()
        if depth is not None:
            self.depth = depth
        if book_mode is not None:
            self.book_mode = book_mode
        if search_mode is not None:
            self.search_mode = search_mode
        command: list[str] = [self.engine_path, "--interactive"]
        if self.params_path:
            command.extend(["--params", self.params_path])
        if self.depth is not None:
            command.extend(["--depth", str(self.depth)])
        if self.search_mode:
            command.extend(["--search-mode", self.search_mode])
        if self.book_path:
            command.extend(["--book", self.book_path])
        if self.book_mode:
            command.extend(["--book-mode", self.book_mode])

        # Le mode texte et le tampon ligne par ligne maintiennent la frontière
        # de protocole lisible et évitent toute conversion manuelle d’octets.
        self._process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

    def new_game(self, human_color: str) -> EngineResponse:
        """Créer une partie et retourner sa position initiale."""
        self._send(f"new_game {human_color}")
        return self._read_response()

    def play_human_move(self, uci_move: str) -> EngineResponse:
        """Soumettre un coup humain dans une partie humain contre IA."""
        self._send(f"human_move {uci_move}")
        first = self._read_line()
        if first == "illegal_move":
            return EngineResponse(ok=False, error="Coup illegal")
        if first.startswith("error "):
            return EngineResponse(ok=False, error=first[6:])
        response = self._read_response()
        response.ok = first == "ok"
        return response

    def play_free_move(self, uci_move: str) -> EngineResponse:
        """Soumettre un coup sans contrainte de camp dans une partie locale."""
        self._send(f"free_move {uci_move}")
        first = self._read_line()
        if first == "illegal_move":
            return EngineResponse(ok=False, error="Coup illegal")
        if first.startswith("error "):
            return EngineResponse(ok=False, error=first[6:])
        response = self._read_response()
        response.ok = first == "ok"
        return response

    def request_ai_move(self) -> EngineResponse:
        """Demander au moteur de calculer puis jouer son prochain coup."""
        self._send("ai_move")
        return self._read_response()

    def undo_turn(self) -> EngineResponse:
        """Annuler un tour complet afin de rendre le trait au joueur humain."""
        self._send("undo_turn")
        first = self._read_line()
        if first == "cannot_undo":
            return EngineResponse(ok=False, error="Aucun coup a annuler")
        if first.startswith("error "):
            return EngineResponse(ok=False, error=first[6:])
        response = self._read_response()
        response.ok = first == "ok"
        return response

    def undo_move(self) -> EngineResponse:
        """Annuler un seul demi-coup dans une partie à deux joueurs."""
        self._send("undo_move")
        first = self._read_line()
        if first == "cannot_undo":
            return EngineResponse(ok=False, error="Aucun coup a annuler")
        if first.startswith("error "):
            return EngineResponse(ok=False, error=first[6:])
        response = self._read_response()
        response.ok = first == "ok"
        return response

    def get_legal_moves(self) -> list[str]:
        """
        Obtenir tous les coups légaux de la position courante.

        Returns:
            list[str]: Coups encodés au format UCI.

        Raises:
            RuntimeError: Si le moteur signale une erreur ou rompt le protocole.
        """
        self._send("get_legal_moves")
        line = self._read_line()
        if line.startswith("legal_moves"):
            parts = line.split()
            return parts[1:]
        if line.startswith("error "):
            raise RuntimeError(line[6:])
        raise RuntimeError(line)

    def close(self) -> None:
        """
        Arrêter proprement le processus encore actif.

        Une terminaison forcée est utilisée uniquement si le protocole est
        déjà rompu ou si le moteur ne répond pas dans le délai accordé.

        Returns:
            None: Le processus est terminé ou était déjà arrêté.
        """
        if self._process is None or self._process.poll() is not None:
            return
        try:
            self._send("quit")
            self._process.communicate(timeout=1)
        except (
            BrokenPipeError,
            OSError,
            RuntimeError,
            subprocess.TimeoutExpired,
        ):
            self._process.kill()

    def _send(self, command: str) -> None:
        """
        Écrire et vider immédiatement une commande protocolaire.

        Args:
            command (str): Ligne sans terminaison à transmettre.

        Returns:
            None: La commande est disponible pour le moteur.

        Raises:
            RuntimeError: Si l’entrée standard du moteur est indisponible.
        """
        if self._process is None or self._process.stdin is None:
            raise RuntimeError("engine stdin is closed")
        self._process.stdin.write(command + "\n")
        self._process.stdin.flush()

    def _read_line(self) -> str:
        """
        Lire une ligne et convertir une fin prématurée en erreur explicite.

        Returns:
            str: Ligne protocolaire sans espaces périphériques.

        Raises:
            RuntimeError: Si le processus est arrêté ou sa sortie indisponible.
        """
        if self._process is None or self._process.stdout is None:
            raise RuntimeError("engine stdout is closed")
        line = self._process.stdout.readline()
        if line == "":
            stderr = ""
            if self._process.stderr is not None:
                stderr = self._process.stderr.read()
            raise RuntimeError(stderr.strip() or "engine stopped")
        return line.strip()

    def _read_response(self) -> EngineResponse:
        """
        Agréger les lignes d’une réponse jusqu’à son état final.

        La limite protège la GUI contre un moteur qui émettrait indéfiniment
        des lignes inconnues sans jamais conclure sa réponse.

        Returns:
            EngineResponse: Réponse partielle ou complète décodée.
        """
        response = EngineResponse(ok=True)
        for _ in range(12):
            line = self._read_line()
            if line.startswith("ai_move "):
                response.ai_move = line[8:]
            elif line.startswith("board_fen "):
                response.fen = line[10:]
            elif line.startswith("fen "):
                response.fen = line[4:]
            elif line.startswith("status "):
                response.status = line
                return response
            elif line.startswith("error "):
                return EngineResponse(ok=False, error=line[6:])
            elif line == "illegal_move":
                return EngineResponse(ok=False, error="Coup illegal")
        return response
