/**
 * @file square.cpp
 * @brief Convertir les cases entre coordonnées matricielles et algébriques.
 */

#include "chess/square.hpp"

#include <stdexcept>

namespace chess {

std::string Square::toAlgebraic() const {
    if (!isValid()) {
        return "-";
    }
    std::string text;
    text.push_back(static_cast<char>('a' + col_));
    text.push_back(static_cast<char>('8' - row_));
    return text;
}

Square Square::fromAlgebraic(const std::string& text) {
    if (text.size() != 2) {
        throw std::invalid_argument("invalid square notation: " + text);
    }
    char file = text[0];
    char rank = text[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        throw std::invalid_argument("invalid square notation: " + text);
    }
    return Square('8' - rank, file - 'a');
}

} // namespace chess
