/**
 * @file color.hpp
 * @brief Définir les couleurs des joueurs et leur relation d’opposition.
 */

#pragma once

namespace chess {

enum class Color {
    White,
    Black,
    None
};

/** @brief Retourner le camp adverse ou None pour une couleur absente. */
constexpr Color opposite(Color color) {
    if (color == Color::White) {
        return Color::Black;
    }
    if (color == Color::Black) {
        return Color::White;
    }
    return Color::None;
}

/** @brief Convertir une couleur en nom stable destiné aux sorties textuelles. */
const char* colorName(Color color);

} // namespace chess
