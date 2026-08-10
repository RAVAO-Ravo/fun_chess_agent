/**
 * @file test_position.cpp
 * @brief Vérifier les mutations et restaurations d’une position.
 */

#include "test_util.hpp"

#include "chess/position.hpp"

void test_board(TestSuite& suite) {
    chess::Position board;
    REQUIRE_EQ(suite, board.toFen(), std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
    REQUIRE_EQ(suite, chess::Square::fromAlgebraic("a8").row(), 0);
    REQUIRE_EQ(suite, chess::Square::fromAlgebraic("h1").col(), 7);
    REQUIRE_EQ(suite, chess::Square(4, 4).toAlgebraic(), std::string("e4"));
    REQUIRE_EQ(suite, board.pieceAt(chess::Square::fromAlgebraic("e1")).type(), chess::PieceType::King);
    REQUIRE_EQ(suite, board.pieceAt(chess::Square::fromAlgebraic("a7")).color(), chess::Color::Black);
    REQUIRE_EQ(suite, board.pieceCount(chess::Color::White, chess::PieceType::Pawn), 8);
    REQUIRE_EQ(suite, board.pieceCount(chess::Color::Black, chess::PieceType::Knight), 2);

    chess::Move move = chess::Move::fromUci("e2e4");
    REQUIRE_EQ(suite, move.toUci(), std::string("e2e4"));
    chess::Move promotion = chess::Move::fromUci("e7e8q");
    REQUIRE(suite, promotion.isPromotion());
    REQUIRE_EQ(suite, promotion.promotion(), chess::PieceType::Queen);

    const std::uint64_t hashBeforeMove = board.zobristHash();
    const std::optional<chess::Move> legalMove = board.findLegalMove("e2e4");
    REQUIRE(suite, legalMove.has_value());
    board.makeMove(*legalMove);
    board.undoMove();
    REQUIRE_EQ(suite, board.pieceCount(chess::Color::White, chess::PieceType::Pawn), 8);
    REQUIRE_EQ(suite, board.zobristHash(), hashBeforeMove);
}
