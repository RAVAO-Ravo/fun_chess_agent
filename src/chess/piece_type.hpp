/**
 * @file piece_type.hpp
 * @brief Définir les catégories de pièces et les conversions de promotion.
 */

#pragma once

namespace chess {

enum class PieceType {
    King,
    Queen,
    Rook,
    Bishop,
    Knight,
    Pawn,
    None
};

/** @brief Convertir un type autorisé en suffixe de promotion UCI. */
char promotionChar(PieceType type);
/** @brief Décoder un suffixe UCI en type de promotion. */
PieceType promotionTypeFromChar(char c);
/** @brief Obtenir l’indice compact utilisé par les compteurs de pièces. */
int pieceTypeIndex(PieceType type);

} // namespace chess
