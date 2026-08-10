/**
 * @file fen.hpp
 * @brief Déclarer la conversion entre une position interne et le format FEN.
 */

#pragma once

#include "chess/position.hpp"

#include <string>

namespace chess {

/**
 * @class Fen
 * @brief Centraliser la conversion complète du format Forsyth-Edwards.
 */
class Fen {
public:
    /** @brief Sérialiser les six champs réglementaires d’une position. */
    static std::string toFen(const Position& board);
    /** @brief Construire et valider une position depuis six champs FEN. */
    static Position fromFen(const std::string& fen);
};

} // namespace chess
