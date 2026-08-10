/**
 * @file test_moves.cpp
 * @brief Vérifier les déplacements ordinaires et spéciaux.
 */

#include "test_util.hpp"

#include "chess/move_generator.hpp"
#include "chess/position.hpp"

#include <algorithm>
#include <vector>

namespace {

bool hasMove(const chess::Position& board, const std::string& uci) {
    for (const chess::Move& move : board.legalMoves()) {
        if (move.toUci() == uci) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> moveNames(std::vector<chess::Move> moves) {
    std::vector<std::string> names;
    names.reserve(moves.size());
    for (const chess::Move& move : moves) {
        names.push_back(move.toUci());
    }
    std::sort(names.begin(), names.end());
    return names;
}

void requireTacticalGenerationMatchesFilter(
    TestSuite& suite,
    const chess::Position& position) {
    std::vector<chess::Move> filtered = position.legalMoves();
    filtered.erase(
        std::remove_if(
            filtered.begin(),
            filtered.end(),
            [](const chess::Move& move) {
                return !move.isCapture() && !move.isPromotion();
            }),
        filtered.end());
    const std::vector<chess::Move> tactical =
        chess::MoveGenerator::legalMoves(
            position,
            chess::MoveGenerationMode::Tactical);
    REQUIRE_EQ(suite, moveNames(tactical), moveNames(filtered));

    const std::vector<chess::Move> annotated =
        chess::MoveGenerator::legalMoves(
            position,
            chess::MoveGenerationMode::All,
            chess::MoveAnnotationMode::Ordering);
    REQUIRE_EQ(suite, moveNames(annotated), moveNames(position.legalMoves()));
    for (const chess::Move& move : annotated) {
        REQUIRE(suite, move.hasOrderingMetadata());
        chess::Position afterMove = position.copyWithoutHistory();
        REQUIRE(suite, afterMove.makeMove(move));
        REQUIRE_EQ(
            suite,
            move.givesCheck(),
            afterMove.isInCheck(afterMove.sideToMove()));
        REQUIRE_EQ(
            suite,
            move.destinationIsAttacked(),
            chess::MoveGenerator::isSquareAttacked(
                afterMove,
                move.to(),
                afterMove.sideToMove()));
    }
}

} // namespace

void test_moves(TestSuite& suite) {
    chess::Position initial;
    REQUIRE(suite, hasMove(initial, "e2e3"));
    REQUIRE(suite, hasMove(initial, "e2e4"));
    REQUIRE(suite, hasMove(initial, "g1f3"));
    REQUIRE(suite, !hasMove(initial, "f1e2"));
    REQUIRE(suite, !hasMove(initial, "a1a3"));

    chess::Position blocked = chess::Position::fromFen("4k3/8/8/8/8/4P3/4P3/4K3 w - - 0 1");
    REQUIRE(suite, !hasMove(blocked, "e2e3"));

    chess::Position capture = chess::Position::fromFen("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1");
    REQUIRE(suite, hasMove(capture, "e4d5"));

    chess::Position queen = chess::Position::fromFen("4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1");
    REQUIRE(suite, hasMove(queen, "d4h4"));
    REQUIRE(suite, hasMove(queen, "d4g7"));

    chess::Position king = chess::Position::fromFen("4k3/8/8/8/8/8/4K3/8 w - - 0 1");
    REQUIRE(suite, hasMove(king, "e2e3"));

    chess::Position promotion = chess::Position::fromFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    REQUIRE(suite, hasMove(promotion, "a7a8q"));
    REQUIRE(suite, hasMove(promotion, "a7a8n"));

    chess::Position enPassant = chess::Position::fromFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    REQUIRE(suite, hasMove(enPassant, "e5d6"));

    chess::Position castling = chess::Position::fromFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    REQUIRE(suite, hasMove(castling, "e1g1"));
    REQUIRE(suite, hasMove(castling, "e1c1"));

    const chess::Position tacticalPromotion =
        chess::Position::fromFen("1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    const chess::Position stalemate =
        chess::Position::fromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    for (const chess::Position& position : {
             initial,
             capture,
             enPassant,
             tacticalPromotion,
             castling,
         }) {
        requireTacticalGenerationMatchesFilter(suite, position);
        REQUIRE_EQ(
            suite,
            chess::MoveGenerator::hasAnyLegalMove(position),
            !position.legalMoves().empty());
    }
    REQUIRE(suite, !chess::MoveGenerator::hasAnyLegalMove(stalemate));
}
