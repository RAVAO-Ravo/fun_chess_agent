/**
 * @file fen.cpp
 * @brief Valider, lire et sérialiser les six champs d’une position FEN.
 */

#include "chess/fen.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace chess {

namespace {

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    std::istringstream stream(text);
    while (std::getline(stream, current, delimiter)) {
        parts.push_back(current);
    }
    return parts;
}

} // namespace

std::string Fen::toFen(const Position& board) {
    // === Placement des pièces ===

    std::ostringstream out;
    for (int row = 0; row < 8; ++row) {
        int emptyCount = 0;
        for (int col = 0; col < 8; ++col) {
            Piece piece = board.pieceAt(Square(row, col));
            if (piece.isEmpty()) {
                ++emptyCount;
                continue;
            }
            if (emptyCount > 0) {
                out << emptyCount;
                emptyCount = 0;
            }
            out << piece.toChar();
        }
        if (emptyCount > 0) {
            out << emptyCount;
        }
        if (row != 7) {
            out << '/';
        }
    }

    // === État de partie ===

    out << ' ' << (board.sideToMove() == Color::White ? 'w' : 'b') << ' ';

    std::string castling;
    if (board.canCastleKingSide(Color::White)) {
        castling.push_back('K');
    }
    if (board.canCastleQueenSide(Color::White)) {
        castling.push_back('Q');
    }
    if (board.canCastleKingSide(Color::Black)) {
        castling.push_back('k');
    }
    if (board.canCastleQueenSide(Color::Black)) {
        castling.push_back('q');
    }
    out << (castling.empty() ? "-" : castling);

    out << ' ';
    if (board.enPassantSquare().has_value()) {
        out << board.enPassantSquare()->toAlgebraic();
    } else {
        out << '-';
    }

    out << ' ' << board.halfmoveClock() << ' ' << board.fullmoveNumber();
    return out.str();
}

Position Fen::fromFen(const std::string& fen) {
    std::istringstream stream(fen);
    std::string placement;
    std::string side;
    std::string castling;
    std::string enPassant;
    int halfmove = 0;
    int fullmove = 1;

    // Exiger les six champs évite de construire une position partiellement
    // initialisée à partir d’une notation abrégée ambiguë.
    if (!(stream >> placement >> side >> castling >> enPassant >> halfmove >> fullmove)) {
        throw std::invalid_argument("invalid FEN: expected six fields");
    }

    Position board;
    board.clear();

    std::vector<std::string> rows = split(placement, '/');
    if (rows.size() != 8) {
        throw std::invalid_argument("invalid FEN: expected eight ranks");
    }

    // Chaque rangée doit développer exactement huit colonnes ; les chiffres
    // compressent les cases vides sans être des pièces.
    for (int row = 0; row < 8; ++row) {
        int col = 0;
        for (char c : rows[row]) {
            if (std::isdigit(static_cast<unsigned char>(c))) {
                col += c - '0';
                continue;
            }
            if (col >= 8) {
                throw std::invalid_argument("invalid FEN: too many files");
            }
            board.setPiece(Square(row, col), Piece::fromChar(c));
            ++col;
        }
        if (col != 8) {
            throw std::invalid_argument("invalid FEN: rank does not contain eight files");
        }
    }

    if (side == "w") {
        board.setSideToMove(Color::White);
    } else if (side == "b") {
        board.setSideToMove(Color::Black);
    } else {
        throw std::invalid_argument("invalid FEN: side to move must be w or b");
    }

    // Les droits sont lus indépendamment de la présence des tours. Le
    // générateur vérifiera ensuite les pièces réelles avant d’autoriser un roque.
    bool whiteKing = false;
    bool whiteQueen = false;
    bool blackKing = false;
    bool blackQueen = false;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
            case 'K':
                whiteKing = true;
                break;
            case 'Q':
                whiteQueen = true;
                break;
            case 'k':
                blackKing = true;
                break;
            case 'q':
                blackQueen = true;
                break;
            default:
                throw std::invalid_argument("invalid FEN: castling rights");
            }
        }
    }
    board.setCastlingRights(Color::White, whiteKing, whiteQueen);
    board.setCastlingRights(Color::Black, blackKing, blackQueen);

    if (enPassant == "-") {
        board.setEnPassantSquare(std::nullopt);
    } else {
        board.setEnPassantSquare(Square::fromAlgebraic(enPassant));
    }
    if (halfmove < 0 || fullmove < 1) {
        throw std::invalid_argument("invalid FEN: move counters");
    }
    board.setHalfmoveClock(halfmove);
    board.setFullmoveNumber(fullmove);
    return board;
}

} // namespace chess
