/**
 * @file move.hpp
 * @brief Représenter un coup et les métadonnées utiles à la recherche.
 */

#pragma once

#include "chess/piece_type.hpp"
#include "chess/square.hpp"

#include <string>

namespace chess {

/**
 * @class Move
 * @brief Porter un déplacement UCI et ses annotations calculées.
 *
 * Les drapeaux de règle sont produits par le générateur. Les métadonnées
 * d’ordre, plus coûteuses, ne sont renseignées que pour la recherche.
 */
class Move {
public:
    constexpr Move() = default;
    constexpr Move(Square from, Square to)
        : from_(from)
        , to_(to) {
    }

    constexpr Square from() const {
        return from_;
    }
    constexpr Square to() const {
        return to_;
    }

    constexpr PieceType promotion() const {
        return promotion_;
    }
    constexpr void setPromotion(PieceType type) {
        promotion_ = type;
    }

    constexpr bool isPromotion() const {
        return promotion_ != PieceType::None;
    }
    constexpr bool isCastle() const {
        return castle_;
    }
    constexpr bool isEnPassant() const {
        return enPassant_;
    }
    constexpr bool isCapture() const {
        return capture_;
    }
    constexpr bool hasOrderingMetadata() const {
        return orderingMetadata_;
    }
    constexpr bool givesCheck() const {
        return givesCheck_;
    }
    constexpr bool destinationIsAttacked() const {
        return destinationIsAttacked_;
    }

    constexpr void setCastle(bool value) {
        castle_ = value;
    }
    constexpr void setEnPassant(bool value) {
        enPassant_ = value;
    }
    constexpr void setCapture(bool value) {
        capture_ = value;
    }
    constexpr void setOrderingMetadata(
        bool givesCheck,
        bool destinationIsAttacked) {
        orderingMetadata_ = true;
        givesCheck_ = givesCheck;
        destinationIsAttacked_ = destinationIsAttacked;
    }

    /** @brief Sérialiser l’origine, la destination et la promotion en UCI. */
    std::string toUci() const;
    /** @brief Décoder un coup UCI sans décider de sa légalité positionnelle. */
    static Move fromUci(const std::string& text);

    /** @brief Comparer uniquement les composantes exprimées par la notation UCI. */
    constexpr bool sameUci(const Move& other) const {
        return from_ == other.from_
            && to_ == other.to_
            && promotion_ == other.promotion_;
    }

private:
    Square from_;
    Square to_;
    PieceType promotion_ = PieceType::None;
    bool castle_ = false;
    bool enPassant_ = false;
    bool capture_ = false;
    bool orderingMetadata_ = false;
    bool givesCheck_ = false;
    bool destinationIsAttacked_ = false;
};

} // namespace chess
