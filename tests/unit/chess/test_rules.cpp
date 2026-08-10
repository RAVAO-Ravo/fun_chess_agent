/**
 * @file test_rules.cpp
 * @brief Vérifier les états terminaux et règles de partie nulle.
 */

#include "test_util.hpp"

#include "chess/position.hpp"
#include "chess/zobrist.hpp"

namespace {

bool hasLegal(const chess::Position& board, const std::string& uci) {
    return board.findLegalMove(uci).has_value();
}

void play(chess::Position& position, const std::string& uci) {
    const std::optional<chess::Move> move = position.findLegalMove(uci);
    if (!move.has_value() || !position.makeMove(*move)) {
        throw std::runtime_error("test move is not legal: " + uci);
    }
}

} // namespace

void test_rules(TestSuite& suite) {
    REQUIRE(suite, chess::Position::fromFen("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1").isInCheck(chess::Color::White));
    REQUIRE(suite, chess::Position::fromFen("4k3/8/8/8/8/2b5/8/4K3 w - - 0 1").isInCheck(chess::Color::White));
    REQUIRE(suite, chess::Position::fromFen("4k3/8/8/8/4q3/8/8/4K3 w - - 0 1").isInCheck(chess::Color::White));
    REQUIRE(suite, chess::Position::fromFen("4k3/8/8/8/8/5n2/8/4K3 w - - 0 1").isInCheck(chess::Color::White));

    chess::Position pinned = chess::Position::fromFen("k3r3/8/8/8/8/8/4R3/4K3 w - - 0 1");
    REQUIRE(suite, !hasLegal(pinned, "e2d2"));
    REQUIRE(suite, hasLegal(pinned, "e2e8"));

    chess::Position mate = chess::Position::fromFen("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
    REQUIRE(suite, mate.isCheckmate());
    chess::Position mateAtFiftyMove =
        chess::Position::fromFen("7k/6Q1/6K1/8/8/8/8/8 b - - 100 1");
    REQUIRE_EQ(
        suite,
        mateAtFiftyMove.termination(),
        chess::GameTermination::Checkmate);
    REQUIRE(suite, !mateAtFiftyMove.isDraw());

    chess::Position stalemate = chess::Position::fromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    REQUIRE(suite, stalemate.isStalemate());

    chess::Position fifty = chess::Position::fromFen("4k3/8/8/8/8/8/8/4K3 w - - 100 1");
    REQUIRE(suite, fifty.isDraw());

    chess::Position bareKings = chess::Position::fromFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    REQUIRE(suite, bareKings.hasInsufficientMaterial());

    chess::Position bishopOnly = chess::Position::fromFen("4k3/8/8/8/8/8/8/3BK3 w - - 0 1");
    REQUIRE(suite, bishopOnly.hasInsufficientMaterial());

    chess::Position hashPosition = chess::Position::fromFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    const std::uint64_t initialHash = hashPosition.zobristHash();
    REQUIRE_EQ(suite, initialHash, chess::Zobrist::hash(hashPosition));
    play(hashPosition, "h1h2");
    REQUIRE_EQ(suite, hashPosition.zobristHash(), chess::Zobrist::hash(hashPosition));
    REQUIRE(suite, hashPosition.zobristHash() != initialHash);
    hashPosition.undoMove();
    REQUIRE_EQ(suite, hashPosition.zobristHash(), initialHash);
    REQUIRE_EQ(suite, hashPosition.zobristHash(), chess::Zobrist::hash(hashPosition));

    chess::Position fromMoves;
    play(fromMoves, "g1f3");
    play(fromMoves, "g8f6");
    play(fromMoves, "f3g1");
    play(fromMoves, "f6g8");
    chess::Position fromFen = chess::Position::fromFen(fromMoves.toFen());
    REQUIRE_EQ(suite, fromMoves.zobristHash(), fromFen.zobristHash());

    chess::Position noCastling = chess::Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 4 3");
    REQUIRE(suite, fromMoves.zobristHash() != noCastling.zobristHash());

    play(fromMoves, "g1f3");
    play(fromMoves, "g8f6");
    play(fromMoves, "f3g1");
    play(fromMoves, "f6g8");
    REQUIRE(suite, fromMoves.isThreefoldRepetition());
    REQUIRE(suite, fromMoves.isRuleDraw());
}
