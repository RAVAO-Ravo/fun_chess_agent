/**
 * @file zobrist.cpp
 * @brief Construire de manière déterministe les clés de hachage du plateau.
 */

#include "chess/zobrist.hpp"

#include <algorithm>
#include <array>

namespace chess {

namespace {

std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t result = (state += 0x9e3779b97f4a7c15ull);
    result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9ull;
    result = (result ^ (result >> 27)) * 0x94d049bb133111ebull;
    return result ^ (result >> 31);
}

struct ZobristTables {
    std::array<std::array<std::uint64_t, 64>, 12> pieces{};
    std::array<std::uint64_t, 4> castling{};
    std::array<std::uint64_t, 8> enPassantFile{};
    std::array<std::uint64_t, 101> halfmoveClock{};
    std::uint64_t blackToMove = 0;
};

const ZobristTables& tables() {
    static const ZobristTables generated = [] {
        ZobristTables result;
        std::uint64_t seed = 0x6a09e667f3bcc909ull;
        for (auto& pieceTable : result.pieces) {
            for (std::uint64_t& value : pieceTable) {
                value = splitmix64(seed);
            }
        }
        for (std::uint64_t& value : result.castling) {
            value = splitmix64(seed);
        }
        for (std::uint64_t& value : result.enPassantFile) {
            value = splitmix64(seed);
        }
        for (std::uint64_t& value : result.halfmoveClock) {
            value = splitmix64(seed);
        }
        result.blackToMove = splitmix64(seed);
        return result;
    }();
    return generated;
}

int pieceHashIndex(Piece piece) {
    if (piece.isEmpty()) {
        return -1;
    }

    int typeOffset = -1;
    switch (piece.type()) {
    case PieceType::King:
        typeOffset = 0;
        break;
    case PieceType::Queen:
        typeOffset = 1;
        break;
    case PieceType::Rook:
        typeOffset = 2;
        break;
    case PieceType::Bishop:
        typeOffset = 3;
        break;
    case PieceType::Knight:
        typeOffset = 4;
        break;
    case PieceType::Pawn:
        typeOffset = 5;
        break;
    case PieceType::None:
        return -1;
    }

    return (piece.color() == Color::White ? 0 : 6) + typeOffset;
}

} // namespace

std::uint64_t Zobrist::pieceKey(Piece piece, Square square) {
    int index = pieceHashIndex(piece);
    if (index < 0 || !square.isValid()) {
        return 0;
    }
    return tables().pieces[static_cast<std::size_t>(index)][static_cast<std::size_t>(square.index())];
}

std::uint64_t Zobrist::sideToMoveKey(Color color) {
    return color == Color::Black ? tables().blackToMove : 0;
}

std::uint64_t Zobrist::castlingRightKey(Color color, bool kingSide) {
    if (color == Color::White) {
        return tables().castling[kingSide ? 0 : 1];
    }
    if (color == Color::Black) {
        return tables().castling[kingSide ? 2 : 3];
    }
    return 0;
}

std::uint64_t Zobrist::enPassantFileKey(int file) {
    if (file < 0 || file > 7) {
        return 0;
    }
    return tables().enPassantFile[static_cast<std::size_t>(file)];
}

std::uint64_t Zobrist::halfmoveClockKey(int halfmoveClock) {
    return tables().halfmoveClock[
        static_cast<std::size_t>(std::clamp(halfmoveClock, 0, 100))];
}

std::uint64_t Zobrist::hash(const Position& board) {
    const ZobristTables& z = tables();
    std::uint64_t hash = 0;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Square square(row, col);
            Piece piece = board.pieceAt(square);
            int index = pieceHashIndex(piece);
            if (index >= 0) {
                hash ^= z.pieces[static_cast<std::size_t>(index)][static_cast<std::size_t>(square.index())];
            }
        }
    }

    if (board.sideToMove() == Color::Black) {
        hash ^= z.blackToMove;
    }
    if (board.canCastleKingSide(Color::White)) {
        hash ^= z.castling[0];
    }
    if (board.canCastleQueenSide(Color::White)) {
        hash ^= z.castling[1];
    }
    if (board.canCastleKingSide(Color::Black)) {
        hash ^= z.castling[2];
    }
    if (board.canCastleQueenSide(Color::Black)) {
        hash ^= z.castling[3];
    }
    if (board.enPassantSquare().has_value()) {
        hash ^= z.enPassantFile[static_cast<std::size_t>(board.enPassantSquare()->col())];
    }
    return hash;
}

} // namespace chess
