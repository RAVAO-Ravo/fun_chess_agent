/**
 * @file move.cpp
 * @brief Convertir les coups depuis et vers la notation UCI.
 */

#include "chess/move.hpp"

#include <stdexcept>

namespace chess {

std::string Move::toUci() const {
    if (!from_.isValid() || !to_.isValid()) {
        return "0000";
    }
    std::string text = from_.toAlgebraic() + to_.toAlgebraic();
    if (isPromotion()) {
        text.push_back(promotionChar(promotion_));
    }
    return text;
}

Move Move::fromUci(const std::string& text) {
    if (text.size() != 4 && text.size() != 5) {
        throw std::invalid_argument("invalid move format: " + text);
    }
    Move move(Square::fromAlgebraic(text.substr(0, 2)), Square::fromAlgebraic(text.substr(2, 2)));
    if (text.size() == 5) {
        PieceType promotion = promotionTypeFromChar(text[4]);
        if (promotion == PieceType::None) {
            throw std::invalid_argument("invalid promotion piece: " + text);
        }
        move.setPromotion(promotion);
    }
    return move;
}

} // namespace chess
