/**
 * @file test_fen.cpp
 * @brief Vérifier la lecture et la sérialisation des positions FEN.
 */

#include "test_util.hpp"

#include "chess/position.hpp"
#include "chess/zobrist.hpp"

namespace {

void makeLegal(TestSuite& suite, chess::Position& board, const std::string& uci) {
    for (const chess::Move& move : board.legalMoves()) {
        if (move.toUci() == uci) {
            board.makeMove(move);
            REQUIRE_EQ(suite, board.zobristHash(), chess::Zobrist::hash(board));
            return;
        }
    }
    REQUIRE(suite, false);
}

} // namespace

void test_fen(TestSuite& suite) {
    const std::string initial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    chess::Position board = chess::Position::fromFen(initial);
    REQUIRE_EQ(suite, board.toFen(), initial);
    REQUIRE_EQ(suite, board.zobristHash(), chess::Zobrist::hash(board));

    makeLegal(suite, board, "e2e4");
    REQUIRE_EQ(suite, board.toFen(), std::string("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"));
    board.undoMove();
    REQUIRE_EQ(suite, board.toFen(), initial);
    REQUIRE_EQ(suite, board.zobristHash(), chess::Zobrist::hash(board));

    chess::Position castle = chess::Position::fromFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    REQUIRE_EQ(suite, castle.zobristHash(), chess::Zobrist::hash(castle));
    makeLegal(suite, castle, "e1g1");
    castle.undoMove();
    REQUIRE_EQ(suite, castle.zobristHash(), chess::Zobrist::hash(castle));

    chess::Position enPassant = chess::Position::fromFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    REQUIRE_EQ(suite, enPassant.zobristHash(), chess::Zobrist::hash(enPassant));
    makeLegal(suite, enPassant, "e5d6");
    enPassant.undoMove();
    REQUIRE_EQ(suite, enPassant.zobristHash(), chess::Zobrist::hash(enPassant));
}
