/**
 * @file zobrist.hpp
 * @brief Fournir les clés stables servant à l’empreinte Zobrist.
 */

#pragma once

#include "chess/position.hpp"

#include <cstdint>

namespace chess {

/**
 * @class Zobrist
 * @brief Fournir des clés pseudo-aléatoires déterministes pour chaque état.
 *
 * Les clés sont stables entre exécutions afin de rendre les tests et recherches
 * reproductibles ; elles ne sont pas destinées à un usage cryptographique.
 */
class Zobrist {
public:
    /** @brief Recalculer l’empreinte complète d’une position. */
    static std::uint64_t hash(const Position& board);
    /** @brief Obtenir la clé d’une pièce placée sur une case. */
    static std::uint64_t pieceKey(Piece piece, Square square);
    /** @brief Obtenir la clé du camp au trait. */
    static std::uint64_t sideToMoveKey(Color color);
    /** @brief Obtenir la clé d’un droit de roque individuel. */
    static std::uint64_t castlingRightKey(Color color, bool kingSide);
    /** @brief Obtenir la clé d’une colonne de prise en passant. */
    static std::uint64_t enPassantFileKey(int file);
    /** @brief Obtenir la clé ajoutée au cache pour la règle des cinquante coups. */
    static std::uint64_t halfmoveClockKey(int halfmoveClock);
};

} // namespace chess
