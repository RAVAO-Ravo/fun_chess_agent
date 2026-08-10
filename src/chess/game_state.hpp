/**
 * @file game_state.hpp
 * @brief Conserver l’état réversible nécessaire à l’annulation d’un coup.
 */

#pragma once

#include "chess/color.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"
#include "chess/square.hpp"

#include <cstdint>
#include <optional>

namespace chess {

/**
 * @struct GameState
 * @brief Capturer l’état antérieur minimal permettant une annulation exacte.
 *
 * La structure mémorise le coup, la capture éventuelle et tous les champs FEN
 * modifiés. L’empreinte antérieure garantit une restauration sans dérive.
 */
struct GameState {
    Move move;
    Piece movedPiece;
    Piece capturedPiece;
    std::optional<Square> capturedSquare;

    std::optional<Square> oldEnPassantSquare;
    bool oldWhiteCanCastleKingSide = false;
    bool oldWhiteCanCastleQueenSide = false;
    bool oldBlackCanCastleKingSide = false;
    bool oldBlackCanCastleQueenSide = false;
    int oldHalfmoveClock = 0;
    int oldFullmoveNumber = 1;
    Color oldSideToMove = Color::White;
    std::uint64_t oldZobristHash = 0;
};

} // namespace chess
