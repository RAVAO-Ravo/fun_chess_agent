/**
 * @file square.hpp
 * @brief Représenter une case et ses coordonnées internes ou algébriques.
 */

#pragma once

#include <string>

namespace chess {

/**
 * @class Square
 * @brief Représenter une coordonnée de plateau, invalide par défaut.
 *
 * La ligne zéro correspond à la huitième rangée FEN et la colonne zéro à la
 * colonne ``a``. L’index compact suit l’ordre ligne majeure du tableau interne.
 */
class Square {
public:
    constexpr Square() = default;
    constexpr Square(int row, int col)
        : row_(row)
        , col_(col) {
    }

    constexpr int row() const {
        return row_;
    }
    constexpr int col() const {
        return col_;
    }

    constexpr bool isValid() const {
        return row_ >= 0 && row_ < 8 && col_ >= 0 && col_ < 8;
    }
    constexpr int index() const {
        return isValid() ? row_ * 8 + col_ : -1;
    }

    /** @brief Convertir une case valide en coordonnées algébriques. */
    std::string toAlgebraic() const;
    /** @brief Décoder et valider deux caractères algébriques. */
    static Square fromAlgebraic(const std::string& text);

    constexpr bool operator==(const Square& other) const {
        return row_ == other.row_ && col_ == other.col_;
    }
    constexpr bool operator!=(const Square& other) const {
        return !(*this == other);
    }

private:
    int row_ = -1;
    int col_ = -1;
};

} // namespace chess
