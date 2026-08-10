/**
 * @file piece.hpp
 * @brief Représenter une pièce par son type et sa couleur.
 */

#pragma once

#include "chess/color.hpp"
#include "chess/piece_type.hpp"

namespace chess {

/**
 * @class Piece
 * @brief Associer un type à une couleur avec une valeur vide cohérente.
 */
class Piece {
public:
    constexpr Piece() = default;
    constexpr Piece(PieceType type, Color color)
        : type_(type)
        , color_(type == PieceType::None ? Color::None : color) {
    }

    constexpr PieceType type() const {
        return type_;
    }
    constexpr Color color() const {
        return color_;
    }

    constexpr bool isEmpty() const {
        return type_ == PieceType::None || color_ == Color::None;
    }
    /** @brief Convertir la pièce en caractère FEN sensible à la couleur. */
    char toChar() const;

    /** @brief Construire une pièce depuis un caractère FEN validé. */
    static Piece fromChar(char c);

private:
    PieceType type_ = PieceType::None;
    Color color_ = Color::None;
};

} // namespace chess
