# -*- coding:utf-8 -*-

"""
Démarrer l’interface graphique et localiser ses ressources.

Ce module choisit le moteur correspondant aux presets CMake usuels, récupère
la profondeur du modèle courant et garantit la fermeture du processus enfant
à la destruction de la fenêtre.
"""

from __future__ import annotations

import json
import os
import sys
import tkinter as tk
from pathlib import Path
from tkinter import messagebox
from typing import cast

from .engine_client import EngineClient
from .views.game_view import ChessBoardWidget


def default_engine_path(root: Path) -> str:
    """
    Rechercher le binaire du moteur dans les répertoires de compilation.

    L’ordre privilégie les presets explicitement maintenus par le projet. Le
    nom exécutable seul reste un dernier recours afin de permettre une
    résolution par la variable système PATH.

    Args:
        root (Path): Racine du projet contenant le répertoire ``build``.

    Returns:
        str: Premier chemin existant ou nom générique du binaire.
    """
    names: list[str] = (
        ["chess_engine.exe"] if os.name == "nt" else ["chess_engine"]
    )
    candidates: list[Path] = []
    for name in names:
        candidates.extend(
            [
                root / "build" / "debug" / name,
                root / "build" / "release" / name,
                root / "build" / "benchmark" / name,
                root / "build" / "sanitize" / name,
                root / "build" / name,
            ]
        )
    for candidate in candidates:
        if candidate.exists():
            return str(candidate)
    return names[0]


def default_search_depth(params_path: Path) -> int:
    """
    Lire la profondeur par défaut du modèle sans bloquer le démarrage.

    Une configuration absente ou mal formée ne doit pas empêcher une partie :
    la profondeur prudente de trois demi-coups est alors utilisée. La valeur
    valide est bornée aux limites exposées par la GUI.

    Args:
        params_path (Path): Fichier JSON du modèle courant.

    Returns:
        int: Profondeur comprise entre 1 et 10.
    """
    try:
        with params_path.open("r", encoding="utf-8") as params_file:
            parsed: object = json.load(params_file)
        if not isinstance(parsed, dict):
            return 3
        data = cast(dict[str, object], parsed)
        raw_depth = data.get("searchDepth", 3)
        if isinstance(raw_depth, bool) or not isinstance(raw_depth, (int, str)):
            return 3
        return max(1, min(int(raw_depth), 10))
    except (json.JSONDecodeError, OSError, TypeError, ValueError):
        return 3


def main() -> int:
    """
    Construire la fenêtre et exécuter la boucle événementielle Tkinter.

    Returns:
        int: Zéro après une fermeture normale, un si le moteur ne démarre pas.
    """
    root_dir = Path(__file__).resolve().parents[2]
    engine_path = sys.argv[1] if len(sys.argv) > 1 else default_engine_path(root_dir)
    params_path = root_dir / "data" / "models" / "current.json"
    book_path = root_dir / "data" / "openings" / "generated" / "book.txt"
    default_depth = default_search_depth(params_path)

    try:
        client = EngineClient(
            engine_path,
            str(params_path),
            depth=default_depth,
            book_path=str(book_path),
            book_mode="chill",
            search_mode="instinct_lmr",
        )
    except OSError as error:
        messagebox.showerror("Erreur", str(error))
        return 1

    root = tk.Tk()
    root.title("Chess AI")
    root.geometry("1120x760")
    root.minsize(900, 560)

    game = ChessBoardWidget(root, client)
    game.pack(fill="both", expand=True, padx=12, pady=12)

    def on_close() -> None:
        """Fermer le processus moteur avant de détruire la fenêtre."""
        client.close()
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
