/**
 * @file piece.cpp
 * @brief Convertir les pièces entre leur représentation interne et FEN.
 */

#include "chess/piece.hpp"

#include <cctype>
#include <stdexcept>

namespace chess {

const char* colorName(Color color) {
    switch (color) {
    case Color::White:
        return "white";
    case Color::Black:
        return "black";
    case Color::None:
        return "none";
    }
    return "none";
}

char promotionChar(PieceType type) {
    switch (type) {
    case PieceType::Queen:
        return 'q';
    case PieceType::Rook:
        return 'r';
    case PieceType::Bishop:
        return 'b';
    case PieceType::Knight:
        return 'n';
    default:
        return '\0';
    }
}

PieceType promotionTypeFromChar(char c) {
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(c)))) {
    case 'q':
        return PieceType::Queen;
    case 'r':
        return PieceType::Rook;
    case 'b':
        return PieceType::Bishop;
    case 'n':
        return PieceType::Knight;
    default:
        return PieceType::None;
    }
}

int pieceTypeIndex(PieceType type) {
    switch (type) {
    case PieceType::King:
        return 0;
    case PieceType::Queen:
        return 1;
    case PieceType::Rook:
        return 2;
    case PieceType::Bishop:
        return 3;
    case PieceType::Knight:
        return 4;
    case PieceType::Pawn:
        return 5;
    case PieceType::None:
        return -1;
    }
    return -1;
}

char Piece::toChar() const {
    if (isEmpty()) {
        return '.';
    }

    char c = '?';
    switch (type_) {
    case PieceType::King:
        c = 'k';
        break;
    case PieceType::Queen:
        c = 'q';
        break;
    case PieceType::Rook:
        c = 'r';
        break;
    case PieceType::Bishop:
        c = 'b';
        break;
    case PieceType::Knight:
        c = 'n';
        break;
    case PieceType::Pawn:
        c = 'p';
        break;
    case PieceType::None:
        c = '.';
        break;
    }

    if (color_ == Color::White) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return c;
}

Piece Piece::fromChar(char c) {
    if (c == '.' || c == '1') {
        return Piece();
    }

    Color color = std::isupper(static_cast<unsigned char>(c)) ? Color::White : Color::Black;
    switch (static_cast<char>(std::tolower(static_cast<unsigned char>(c)))) {
    case 'k':
        return Piece(PieceType::King, color);
    case 'q':
        return Piece(PieceType::Queen, color);
    case 'r':
        return Piece(PieceType::Rook, color);
    case 'b':
        return Piece(PieceType::Bishop, color);
    case 'n':
        return Piece(PieceType::Knight, color);
    case 'p':
        return Piece(PieceType::Pawn, color);
    default:
        throw std::invalid_argument("invalid piece character");
    }
}

} // namespace chess
