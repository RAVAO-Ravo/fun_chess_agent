# -*- coding:utf-8 -*-

"""
Afficher une partie d’échecs et synchroniser la vue avec le moteur C++.

Le module gère la sélection des cases, l’orientation du plateau, les
promotions, l’historique et les retours sonores. Le moteur demeure la source
de vérité : la GUI ne décide jamais elle-même qu’un coup est légal.
"""

from __future__ import annotations

from collections import OrderedDict
from collections.abc import Callable
from functools import partial
import importlib
import logging
import os
from pathlib import Path
import threading
import tkinter as tk
from tkinter import messagebox, ttk
from typing import cast

from PIL import Image, ImageTk

from ..engine_client import EngineClient, EngineResponse, fen_to_matrix


LOGGER = logging.getLogger(__name__)

MATERIAL: dict[str, int] = {
    "P": 1,
    "N": 3,
    "B": 3,
    "R": 5,
    "Q": 9,
    "K": 0,
}

PROMOTION_OPTIONS: tuple[tuple[str, str], ...] = (
    ("q", "Dame"),
    ("r", "Tour"),
    ("b", "Fou"),
    ("n", "Cavalier"),
)

PIECE_ASSET_DIR: Path = (
    Path(__file__).resolve().parents[1] / "assets" / "pieces"
)
PIECE_ASSET_FILES: dict[str, str] = {
    "K": "white_king.png",
    "Q": "white_queen.png",
    "R": "white_rook.png",
    "B": "white_bishop.png",
    "N": "white_knight.png",
    "P": "white_pawn.png",
    "k": "black_king.png",
    "q": "black_queen.png",
    "r": "black_rook.png",
    "b": "black_bishop.png",
    "n": "black_knight.png",
    "p": "black_pawn.png",
}

SoundPattern = tuple[tuple[int, int], ...]

SOUND_PATTERNS: dict[str, SoundPattern] = {
    "move": ((660, 55),),
    "check": ((960, 80), (0, 35), (1280, 120)),
    "checkmate": ((1320, 95), (0, 35), (1040, 95), (0, 35), (1560, 170)),
    "error": ((220, 120), (0, 35), (180, 150)),
}


class PieceImageStore:
    """
    Charger les pièces une fois et mettre en cache leurs redimensionnements.

    Le redimensionnement LANCZOS est relativement coûteux pendant un événement
    ``Configure``. Le cache LRU borné conserve les tailles récentes sans faire
    croître indéfiniment les références Tkinter.

    Attributs:
        master (tk.Misc): Widget propriétaire des images Tkinter.
        sources (dict[str, Image.Image]): Images originales en haute qualité.
        cache (OrderedDict[tuple[str, int], ImageTk.PhotoImage]): Cache LRU.
    """

    def __init__(self, master: tk.Misc) -> None:
        """Charger les ressources originales et initialiser le cache vide."""
        self.master: tk.Misc = master
        self.sources: dict[str, Image.Image] = {
            piece: Image.open(PIECE_ASSET_DIR / filename).convert("RGBA")
            for piece, filename in PIECE_ASSET_FILES.items()
        }
        self.cache: OrderedDict[tuple[str, int], ImageTk.PhotoImage] = OrderedDict()

    def get(self, piece: str, size: float) -> ImageTk.PhotoImage:
        """
        Renvoyer une image de pièce à la taille entière la plus proche.

        Args:
            piece (str): Caractère FEN désignant la pièce et sa couleur.
            size (float): Taille cible en pixels.

        Returns:
            ImageTk.PhotoImage: Image conservée dans le cache de la vue.
        """
        pixel_size = max(16, int(round(size)))
        key = (piece, pixel_size)
        cached = self.cache.get(key)
        if cached is not None:
            self.cache.move_to_end(key)
            return cached

        resized = self.sources[piece].resize(
            (pixel_size, pixel_size),
            Image.Resampling.LANCZOS,
        )
        image = ImageTk.PhotoImage(resized, master=self.master)
        self.cache[key] = image
        # Quarante-huit entrées couvrent plusieurs redimensionnements successifs
        # des douze pièces tout en maintenant une empreinte mémoire bornée.
        while len(self.cache) > 48:
            self.cache.popitem(last=False)
        return image


class ChessBoardWidget(ttk.Frame):
    """
    Présenter une partie et orchestrer les actions du joueur.

    L’état graphique est reconstruit à partir des réponses FEN du moteur. Les
    calculs de l’IA sont déportés dans un thread afin que la boucle Tkinter
    continue de redessiner la fenêtre.

    Attributs:
        client (EngineClient): Connexion active au moteur.
        board (list[list[str]]): Placement courant sous forme matricielle.
        legal_moves (list[str]): Coups légaux de la pièce sélectionnée.
        move_history (list[tuple[str, str, str]]): Historique affichable.
        busy (bool): Indiquer qu’un calcul asynchrone est en cours.
    """

    def __init__(self, master: tk.Misc, client: EngineClient) -> None:
        """
        Construire la barre d’outils, l’historique et le plateau.

        Args:
            master (tk.Misc): Conteneur Tkinter parent.
            client (EngineClient): Processus moteur déjà démarré.

        Returns:
            None: La vue est créée puis initialisée par une nouvelle partie.
        """
        super().__init__(master)
        self.client = client
        self.game_mode = tk.StringVar(value="Humain vs IA")
        self.human_color = tk.StringVar(value="white")
        self.ai_depth = tk.StringVar(value=str(client.depth or 3))
        self.search_mode = tk.StringVar(value=client.search_mode or "instinct_lmr")
        self.book_mode = tk.StringVar(value=client.book_mode or "chill")
        self.selected: tuple[int, int] | None = None
        self.legal_moves: list[str] = []
        self.board: list[list[str]] = [["."] * 8 for _ in range(8)]
        self.current_fen = ""
        self.move_history: list[tuple[str, str, str]] = []
        self.busy = False
        self.pending_ai_job: str | None = None
        self.last_terminal_status = ""
        self.current_status = "status playing"

        self.columnconfigure(1, weight=1)
        self.rowconfigure(1, weight=1)

        # === Barre de contrôle ===

        toolbar = ttk.Frame(self)
        toolbar.grid(row=0, column=0, columnspan=2, sticky="ew", pady=(0, 8))
        ttk.Combobox(
            toolbar,
            values=("Humain vs IA", "Deux humains"),
            width=13,
            state="readonly",
            textvariable=self.game_mode,
        ).pack(side="left")
        ttk.Radiobutton(
            toolbar,
            text="Blancs",
            variable=self.human_color,
            value="white",
        ).pack(side="left")
        ttk.Radiobutton(
            toolbar,
            text="Noirs",
            variable=self.human_color,
            value="black",
        ).pack(side="left", padx=(8, 0))
        ttk.Label(toolbar, text="Profondeur IA").pack(side="left", padx=(16, 4))
        ttk.Spinbox(
            toolbar,
            from_=1,
            to=10,
            width=4,
            textvariable=self.ai_depth,
            command=self.restart_engine,
        ).pack(side="left")
        ttk.Label(toolbar, text="Recherche").pack(side="left", padx=(16, 4))
        ttk.Combobox(
            toolbar,
            values=("classic", "instinct", "instinct_lmr"),
            width=12,
            state="readonly",
            textvariable=self.search_mode,
        ).pack(side="left")
        ttk.Label(toolbar, text="Ouvertures").pack(side="left", padx=(16, 4))
        ttk.Combobox(
            toolbar,
            values=("chill", "competition"),
            width=12,
            state="readonly",
            textvariable=self.book_mode,
        ).pack(side="left")
        self.book_mode.trace_add("write", self.on_engine_setting_changed)
        self.search_mode.trace_add("write", self.on_engine_setting_changed)
        self.game_mode.trace_add("write", self.on_game_mode_changed)
        ttk.Button(
            toolbar,
            text="Nouvelle partie",
            command=self.new_game,
        ).pack(side="left", padx=(16, 0))
        ttk.Button(toolbar, text="Annuler", command=self.undo_turn).pack(side="left", padx=(8, 0))

        self.status = tk.StringVar(value="")
        ttk.Label(toolbar, textvariable=self.status).pack(side="right")

        # === Historique des coups ===

        history_frame = ttk.Frame(self)
        history_frame.grid(row=1, column=0, sticky="nsw", padx=(0, 12))
        ttk.Label(history_frame, text="Historique").pack(anchor="w", pady=(0, 6))
        self.history = ttk.Treeview(
            history_frame,
            columns=("side", "move", "score"),
            show="headings",
            height=24,
        )
        self.history.heading("side", text="Camp")
        self.history.heading("move", text="Coup")
        self.history.heading("score", text="Blanc/Noir")
        self.history.column("side", width=56, anchor="center")
        self.history.column("move", width=82, anchor="w")
        self.history.column("score", width=82, anchor="center")
        self.history.pack(side="left", fill="y")
        scrollbar = ttk.Scrollbar(history_frame, orient="vertical", command=self.history.yview)
        scrollbar.pack(side="right", fill="y")
        self.history.configure(yscrollcommand=scrollbar.set)

        # === Plateau redimensionnable ===

        board_frame = ttk.Frame(self)
        board_frame.grid(row=1, column=1, sticky="nsew")
        board_frame.columnconfigure(0, weight=1)
        board_frame.rowconfigure(0, weight=1)

        self.canvas = tk.Canvas(board_frame, highlightthickness=0, background="#242424")
        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.canvas.bind("<Button-1>", self.on_canvas_clicked)
        self.canvas.bind("<Configure>", self.on_resize)
        self.piece_images = PieceImageStore(self.canvas)

        self.new_game()

    def restart_engine(self) -> None:
        """
        Valider les réglages visibles et redémarrer le processus moteur.

        Toute tâche planifiée est annulée avant le remplacement du client pour
        qu’un ancien calcul ne puisse pas modifier la nouvelle partie.

        Returns:
            None: Une nouvelle partie est créée si le redémarrage réussit.
        """
        try:
            depth = int(self.ai_depth.get())
        except ValueError:
            self.status.set("Profondeur invalide")
            return
        depth = max(1, min(depth, 10))
        self.ai_depth.set(str(depth))
        book_mode = self.book_mode.get()
        if book_mode not in ("chill", "competition"):
            book_mode = "chill"
            self.book_mode.set(book_mode)
        search_mode = self.search_mode.get()
        if search_mode not in ("classic", "instinct", "instinct_lmr"):
            search_mode = "instinct_lmr"
            self.search_mode.set(search_mode)
        if self.pending_ai_job is not None:
            self.after_cancel(self.pending_ai_job)
            self.pending_ai_job = None
        self.busy = False
        try:
            self.client.restart(
                depth=depth,
                book_mode=book_mode,
                search_mode=search_mode,
            )
        except OSError as error:
            self.status.set(str(error))
            self.play_sound("error")
            return
        self.new_game()

    def on_engine_setting_changed(self, *_args: str) -> None:
        """Redémarrer le moteur lorsqu’un réglage persistant change."""
        self.restart_engine()

    def on_game_mode_changed(self, *_args: str) -> None:
        """Réinitialiser la partie lors du changement de mode de jeu."""
        self.new_game()

    def new_game(self) -> None:
        """
        Réinitialiser tous les états temporaires et demander une nouvelle partie.

        Returns:
            None: La réponse initiale du moteur est immédiatement affichée.
        """
        if self.pending_ai_job is not None:
            self.after_cancel(self.pending_ai_job)
        self.selected = None
        self.legal_moves = []
        self.move_history = []
        self.busy = False
        self.pending_ai_job = None
        self.last_terminal_status = ""
        self.current_status = "status playing"
        for item in self.history.get_children():
            self.history.delete(item)
        human_color = "white" if self.is_two_human_game() else self.human_color.get()
        response = self.client.new_game(human_color)
        self.apply_response(response, move=response.ai_move, play_sound=False)

    def on_resize(self, _event: tk.Event) -> None:
        """Redessiner le plateau lorsque le canevas change de taille."""
        self.render()

    def on_canvas_clicked(self, event: tk.Event) -> None:
        """
        Convertir un clic du canevas en coordonnées de case affichée.

        Args:
            event (tk.Event): Événement contenant les coordonnées en pixels.

        Returns:
            None: Les clics hors plateau ou pendant un calcul sont ignorés.
        """
        if self.busy or self.is_terminal(self.current_status):
            return
        square_size, offset_x, offset_y, label_margin = self.geometry()
        display_col = int((event.x - offset_x) // square_size)
        display_row = int((event.y - offset_y) // square_size)
        if display_row < 0 or display_row > 7 or display_col < 0 or display_col > 7:
            return
        self.on_square_clicked(display_row, display_col)

    def on_square_clicked(self, display_row: int, display_col: int) -> None:
        """
        Sélectionner une origine ou soumettre le déplacement choisi.

        La seconde case ne suffit pas pour une promotion : seules les pièces
        proposées par la liste légale du moteur sont alors présentées.

        Args:
            display_row (int): Ligne dans l’orientation visible.
            display_col (int): Colonne dans l’orientation visible.

        Returns:
            None: La vue est mise à jour avec la réponse du moteur.
        """
        row, col = self.display_to_board(display_row, display_col)
        if self.selected is None:
            piece = self.board[row][col]
            if self.is_human_piece(piece):
                self.selected = (row, col)
                self.legal_moves = self.moves_from(row, col)
                self.render()
            return

        from_row, from_col = self.selected
        if (row, col) == self.selected:
            self.selected = None
            self.legal_moves = []
            self.render()
            return

        move = self.square_name(from_row, from_col) + self.square_name(row, col)
        piece = self.board[from_row][from_col]
        if piece in ("P", "p") and row in (0, 7):
            legal_promotions = self.legal_promotions_for(move)
            if not legal_promotions:
                self.selected = None
                self.legal_moves = []
                self.status.set("Coup illegal")
                self.render()
                self.play_sound("error")
                return
            promotion = self.choose_promotion(piece, legal_promotions)
            if promotion is None:
                self.selected = None
                self.legal_moves = []
                self.render()
                return
            move += promotion

        self.selected = None
        self.legal_moves = []
        response = (
            self.client.play_free_move(move)
            if self.is_two_human_game()
            else self.client.play_human_move(move)
        )
        self.apply_response(response, move=move)
        if response.ok and not self.is_two_human_game() and not self.is_terminal(response.status):
            self.busy = True
            self.status.set("L'IA reflechit...")
            self.pending_ai_job = self.after(500, self.start_ai_turn)

    def undo_turn(self) -> None:
        """
        Annuler le nombre de demi-coups cohérent avec le mode de jeu.

        En humain contre IA, un tour normal contient deux demi-coups. Si l’IA
        n’a pas encore joué, seul le coup humain doit être retiré.

        Returns:
            None: L’historique et le plateau sont resynchronisés.
        """
        if self.is_two_human_game():
            expected_removed = 1
        elif self.pending_ai_job is not None:
            self.after_cancel(self.pending_ai_job)
            self.pending_ai_job = None
            self.busy = False
            expected_removed = 1
        elif self.busy:
            self.status.set("Attends la fin du coup IA avant d'annuler.")
            return
        else:
            expected_removed = 2 if len(self.move_history) >= 2 else 1

        response = self.client.undo_move() if self.is_two_human_game() else self.client.undo_turn()
        if not response.ok:
            self.status.set(response.error or "Annulation impossible")
            self.play_sound("error")
            return

        self.remove_history_entries(expected_removed)
        self.last_terminal_status = ""
        self.selected = None
        self.legal_moves = []
        self.apply_response(response, play_sound=False)

    def start_ai_turn(self) -> None:
        """
        Lancer la recherche sans bloquer la boucle événementielle.

        Returns:
            None: Le résultat reviendra sur le thread Tkinter via ``after``.
        """
        self.pending_ai_job = None
        threading.Thread(target=self.request_ai_move, daemon=True).start()

    def choose_promotion(self, pawn: str, legal_promotions: set[str]) -> str | None:
        """
        Afficher une boîte modale limitée aux promotions légales.

        Args:
            pawn (str): Caractère FEN du pion, utilisé pour la couleur.
            legal_promotions (set[str]): Codes UCI autorisés par le moteur.

        Returns:
            str | None: Code choisi, ou ``None`` après annulation.
        """
        dialog = tk.Toplevel(self)
        dialog.title("Promotion")
        dialog.transient(self.winfo_toplevel())
        dialog.resizable(False, False)

        selected: str | None = None

        def finish(choice: str | None) -> None:
            """Mémoriser le choix puis fermer la boîte de dialogue modale."""
            nonlocal selected
            selected = choice
            dialog.destroy()

        ttk.Label(dialog, text="Choisir la piece").grid(
            row=0,
            column=0,
            columnspan=4,
            padx=12,
            pady=(12, 8),
        )
        for index, (code, label) in enumerate(PROMOTION_OPTIONS):
            piece = code.upper() if pawn.isupper() else code
            button = ttk.Button(
                dialog,
                image=self.piece_images.get(piece, 52),
                text=label,
                compound="top",
                width=9,
                command=partial(finish, code),
            )
            if code not in legal_promotions:
                button.state(["disabled"])
            button.grid(row=1, column=index, padx=6, pady=(0, 12))

        for code in legal_promotions:
            dialog.bind(code, partial(self.finish_promotion_event, finish, code))
            dialog.bind(
                code.upper(),
                partial(self.finish_promotion_event, finish, code),
            )
        dialog.bind(
            "<Escape>",
            partial(self.finish_promotion_event, finish, None),
        )
        dialog.protocol("WM_DELETE_WINDOW", partial(finish, None))
        dialog.update_idletasks()
        x = self.winfo_rootx() + max(0, (self.winfo_width() - dialog.winfo_width()) // 2)
        y = self.winfo_rooty() + max(0, (self.winfo_height() - dialog.winfo_height()) // 2)
        dialog.geometry(f"+{x}+{y}")

        dialog.grab_set()
        dialog.focus_set()
        self.wait_window(dialog)
        return selected

    @staticmethod
    def finish_promotion_event(
        finish: Callable[[str | None], None],
        choice: str | None,
        _event: tk.Event,
    ) -> None:
        """Adapter un événement Tkinter au rappel de fin de promotion."""
        finish(choice)

    def request_ai_move(self) -> None:
        """
        Attendre la réponse du moteur depuis le thread de travail.

        Tkinter n’étant pas sûr entre threads, seul le calcul bloquant est
        exécuté ici ; l’application du résultat est replanifiée sur la boucle
        principale.

        Returns:
            None: La réponse est transmise à ``finish_ai_turn``.
        """
        try:
            response = self.client.request_ai_move()
        except (OSError, RuntimeError) as error:
            response = EngineResponse(ok=False, error=str(error))
        self.after(0, partial(self.finish_ai_turn, response))

    def finish_ai_turn(self, response: EngineResponse) -> None:
        """Libérer l’interface et appliquer le coup calculé par l’IA."""
        self.busy = False
        self.apply_response(response, move=response.ai_move)

    def moves_from(self, row: int, col: int) -> list[str]:
        """
        Filtrer les coups légaux selon leur case d’origine.

        Args:
            row (int): Ligne interne du plateau.
            col (int): Colonne interne du plateau.

        Returns:
            list[str]: Coups UCI commençant sur la case demandée.
        """
        prefix = self.square_name(row, col)
        try:
            return [move for move in self.client.get_legal_moves() if move.startswith(prefix)]
        except (OSError, RuntimeError) as error:
            self.status.set(str(error))
            return []

    def legal_promotions_for(self, move_prefix: str) -> set[str]:
        """Extraire les suffixes de promotion légaux pour un déplacement."""
        return {
            move[4]
            for move in self.legal_moves
            if len(move) == 5 and move.startswith(move_prefix)
        }

    def apply_response(
        self,
        response: EngineResponse,
        move: str | None = None,
        play_sound: bool = True,
    ) -> None:
        """
        Remplacer l’état visuel par une réponse faisant autorité.

        Args:
            response (EngineResponse): Résultat décodé du moteur.
            move (str | None): Coup à ajouter à l’historique.
            play_sound (bool): Autoriser le retour sonore associé.

        Returns:
            None: Tous les widgets concernés sont synchronisés.
        """
        if not response.ok:
            self.status.set(response.error or "Erreur")
            self.render()
            self.play_sound("error")
            return
        self.current_status = response.status
        if response.fen:
            self.current_fen = response.fen
            self.board = fen_to_matrix(response.fen)
            if move:
                self.append_history(move, response.fen)
        self.status.set(self.status_text(response))
        self.render()
        if play_sound and move:
            self.play_sound(self.sound_for_status(response.status))
        self.show_terminal_message(response.status)

    def append_history(self, move: str, fen: str) -> None:
        """
        Ajouter un coup avec son camp et le matériel après déplacement.

        Args:
            move (str): Coup UCI joué.
            fen (str): Position résultante servant au calcul de l’affichage.

        Returns:
            None: La dernière ligne devient visible dans l’historique.
        """
        color = self.last_move_color(fen)
        label = (
            f"{(len(self.move_history) // 2) + 1}. {move}"
            if color == "Blanc"
            else f"... {move}"
        )
        score = self.material_score(fen)
        self.move_history.append((color, move, score))
        self.history.insert("", "end", values=(color, label, score))
        children = self.history.get_children()
        if children:
            self.history.see(children[-1])

    def remove_history_entries(self, count: int) -> None:
        """Retirer au plus ``count`` entrées de la fin de l’historique."""
        for _ in range(min(count, len(self.move_history))):
            self.move_history.pop()
            children = self.history.get_children()
            if children:
                self.history.delete(children[-1])

    def render(self) -> None:
        """
        Redessiner entièrement le plateau depuis l’état courant.

        Le rendu complet simplifie la cohérence lors des redimensionnements et
        reste peu coûteux pour seulement 64 cases.

        Returns:
            None: Le canevas reflète la sélection et les coups possibles.
        """
        self.canvas.delete("all")
        square_size, offset_x, offset_y, label_margin = self.geometry()
        label_size = max(10, int(square_size * 0.20))

        self.draw_coordinates(square_size, offset_x, offset_y, label_margin, label_size)

        for display_row in range(8):
            for display_col in range(8):
                row, col = self.display_to_board(display_row, display_col)
                x0 = offset_x + display_col * square_size
                y0 = offset_y + display_row * square_size
                x1 = x0 + square_size
                y1 = y0 + square_size
                color = "#f0d9b5" if (row + col) % 2 == 0 else "#b58863"

                self.canvas.create_rectangle(x0, y0, x1, y1, fill=color, outline="#7c6241")

                if self.selected == (row, col):
                    self.canvas.create_rectangle(
                        x0 + 3,
                        y0 + 3,
                        x1 - 3,
                        y1 - 3,
                        outline="#1d6f96",
                        width=max(3, int(square_size * 0.05)),
                    )

                if self.is_legal_target(row, col):
                    target_piece = self.board[row][col]
                    if target_piece == ".":
                        radius = square_size * 0.13
                        self.canvas.create_oval(
                            (x0 + x1) / 2 - radius,
                            (y0 + y1) / 2 - radius,
                            (x0 + x1) / 2 + radius,
                            (y0 + y1) / 2 + radius,
                            fill="#3f6f4e",
                            outline="",
                        )
                    else:
                        self.canvas.create_oval(
                            x0 + 7,
                            y0 + 7,
                            x1 - 7,
                            y1 - 7,
                            outline="#7d2d2d",
                            width=max(3, int(square_size * 0.05)),
                        )

                piece = self.board[row][col]
                if piece != ".":
                    self.canvas.create_image(
                        (x0 + x1) / 2,
                        (y0 + y1) / 2,
                        image=self.piece_images.get(piece, square_size * 0.94),
                    )

    def draw_coordinates(
        self,
        square_size: float,
        offset_x: float,
        offset_y: float,
        label_margin: float,
        label_size: int,
    ) -> None:
        """
        Dessiner les coordonnées selon l’orientation actuelle du plateau.

        Args:
            square_size (float): Côté d’une case en pixels.
            offset_x (float): Origine horizontale du plateau.
            offset_y (float): Origine verticale du plateau.
            label_margin (float): Marge réservée aux libellés.
            label_size (int): Taille de police calculée.

        Returns:
            None: Les huit colonnes et rangées sont affichées.
        """
        board_size = square_size * 8
        for display_col in range(8):
            _, board_col = self.display_to_board(0, display_col)
            file_label = chr(ord("a") + board_col)
            x = offset_x + display_col * square_size + square_size / 2
            self.canvas.create_text(
                x,
                offset_y + board_size + label_margin * 0.55,
                text=file_label,
                fill="#e6e1d8",
                font=("Segoe UI", label_size, "bold"),
            )

        for display_row in range(8):
            board_row, _ = self.display_to_board(display_row, 0)
            rank_label = str(8 - board_row)
            y = offset_y + display_row * square_size + square_size / 2
            self.canvas.create_text(
                offset_x - label_margin * 0.55,
                y,
                text=rank_label,
                fill="#e6e1d8",
                font=("Segoe UI", label_size, "bold"),
            )

    def geometry(self) -> tuple[float, float, float, float]:
        """
        Calculer un plateau carré centré avec une marge pour les coordonnées.

        Returns:
            tuple[float, float, float, float]: Taille d’une case, décalages
            horizontal et vertical, puis largeur de marge.
        """
        width = max(self.canvas.winfo_width(), 1)
        height = max(self.canvas.winfo_height(), 1)
        label_margin = max(22, min(width, height) * 0.04)
        size = max(240, min(width - label_margin, height - label_margin))
        size = min(size, width - label_margin * 1.5, height - label_margin * 1.5)
        size = max(240, size)
        return (
            size / 8,
            (width - size + label_margin * 0.5) / 2,
            (height - size - label_margin * 0.5) / 2,
            label_margin,
        )

    def is_legal_target(self, row: int, col: int) -> bool:
        """Indiquer si une case termine l’un des coups actuellement proposés."""
        target = self.square_name(row, col)
        return any(move[2:4] == target for move in self.legal_moves)

    def display_to_board(self, row: int, col: int) -> tuple[int, int]:
        """
        Convertir les coordonnées visibles vers l’orientation interne blanche.

        Args:
            row (int): Ligne visible.
            col (int): Colonne visible.

        Returns:
            tuple[int, int]: Coordonnées internes du moteur.
        """
        if not self.is_two_human_game() and self.human_color.get() == "black":
            return 7 - row, 7 - col
        return row, col

    def is_human_piece(self, piece: str) -> bool:
        """Vérifier qu’une pièce peut être sélectionnée par le joueur actif."""
        if piece == ".":
            return False
        if self.is_two_human_game():
            return piece.isupper() if self.side_to_move() == "white" else piece.islower()
        return piece.isupper() if self.human_color.get() == "white" else piece.islower()

    def play_sound(self, kind: str) -> None:
        """
        Jouer un motif sonore sans ralentir l’interface.

        Windows permet de contrôler hauteur et durée avec ``winsound``. Sur
        les autres systèmes, la cloche Tkinter reproduit seulement le rythme ;
        cette dégradation maintient le fonctionnement sans dépendance externe.

        Args:
            kind (str): Nom du motif souhaité.

        Returns:
            None: Une absence de périphérique sonore reste non bloquante.
        """
        pattern = SOUND_PATTERNS.get(kind, SOUND_PATTERNS["move"])
        try:
            if os.name == "nt":
                winsound = importlib.import_module("winsound")
                beep = cast(
                    Callable[[int, int], None],
                    getattr(winsound, "Beep"),
                )

                def play_pattern() -> None:
                    """Jouer séquentiellement le motif depuis le thread sonore."""
                    for frequency, duration in pattern:
                        if frequency:
                            beep(frequency, duration)
                        else:
                            threading.Event().wait(duration / 1000)

                threading.Thread(target=play_pattern, daemon=True).start()
            else:
                delay = 0
                for frequency, duration in pattern:
                    if frequency:
                        self.after(delay, self.bell)
                    delay += duration
        except (ImportError, OSError, RuntimeError, tk.TclError):
            LOGGER.debug("Impossible de jouer le son demandé.", exc_info=True)

    def is_two_human_game(self) -> bool:
        """Indiquer si les deux camps sont contrôlés localement."""
        return self.game_mode.get() == "Deux humains"

    def side_to_move(self) -> str:
        """Lire dans la FEN le camp ayant actuellement le trait."""
        if not self.current_fen:
            return "white"
        parts = self.current_fen.split()
        return "black" if len(parts) > 1 and parts[1] == "b" else "white"

    @staticmethod
    def sound_for_status(status: str) -> str:
        """Associer l’état protocolaire à un motif sonore connu."""
        parts = status.split()
        if len(parts) >= 2 and parts[1] == "checkmate":
            return "checkmate"
        if len(parts) >= 2 and parts[1] == "check":
            return "check"
        return "move"

    def show_terminal_message(self, status: str) -> None:
        """
        Afficher une seule boîte de dialogue pour chaque état terminal.

        Args:
            status (str): État protocolaire reçu.

        Returns:
            None: Les répétitions du même état sont ignorées.
        """
        if status == self.last_terminal_status or not self.is_terminal(status):
            return
        self.last_terminal_status = status
        message = self.status_text(EngineResponse(ok=True, status=status))
        messagebox.showinfo("Partie terminee", message)

    @staticmethod
    def square_name(row: int, col: int) -> str:
        """Convertir des coordonnées internes en nom algébrique."""
        return chr(ord("a") + col) + str(8 - row)

    @staticmethod
    def material_score(fen: str) -> str:
        """
        Calculer un bilan matériel simple depuis le placement FEN.

        Args:
            fen (str): Position contenant le placement des pièces.

        Returns:
            str: Totaux blancs et noirs sous la forme ``blanc/noir``.
        """
        white = 0
        black = 0
        for char in fen.split()[0]:
            if char.isalpha():
                value = MATERIAL[char.upper()]
                if char.isupper():
                    white += value
                else:
                    black += value
        return f"{white}/{black}"

    @staticmethod
    def last_move_color(fen: str) -> str:
        """Déduire le camp du dernier coup à partir du prochain camp au trait."""
        side_to_move = fen.split()[1]
        return "Noir" if side_to_move == "w" else "Blanc"

    @staticmethod
    def is_terminal(status: str) -> bool:
        """Reconnaître les états qui interdisent un nouveau déplacement."""
        return "checkmate" in status or "stalemate" in status or "draw" in status

    @staticmethod
    def status_text(response: EngineResponse) -> str:
        """
        Traduire une réponse protocolaire en message français lisible.

        Args:
            response (EngineResponse): Réponse contenant l’état et le coup.

        Returns:
            str: Texte destiné à la barre d’état ou à une boîte de dialogue.
        """
        parts = response.status.split()
        color_names = {"white": "blancs", "black": "noirs"}
        if response.ai_move and len(parts) == 2 and parts[1] == "playing":
            return f"IA: {response.ai_move}"
        if len(parts) >= 3 and parts[1] == "checkmate":
            return f"Echec et mat. Victoire des {color_names.get(parts[2], parts[2])}."
        if len(parts) >= 3 and parts[1] == "check":
            return f"Echec: {color_names.get(parts[2], parts[2])}"
        if len(parts) >= 2 and parts[1] == "stalemate":
            return "Pat. Partie nulle."
        if len(parts) >= 2 and parts[1] == "draw":
            return "Partie nulle."
        return response.ai_move or ""
