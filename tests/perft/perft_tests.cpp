/**
 * @file perft_tests.cpp
 * @brief Comparer la génération de coups à des décomptes de référence.
 */

#include "test_util.hpp"

#include "chess/position.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace {

std::uint64_t perft(chess::Position& board, int depth) {
    if (depth == 0) {
        return 1;
    }

    const std::vector<chess::Move> moves = board.legalMoves();
    if (depth == 1) {
        return static_cast<std::uint64_t>(moves.size());
    }

    std::uint64_t nodes = 0;
    for (const chess::Move& move : moves) {
        board.makeMove(move);
        nodes += perft(board, depth - 1);
        board.undoMove();
    }
    return nodes;
}

void requirePerft(
    TestSuite& suite,
    chess::Position& board,
    int depth,
    std::uint64_t expected,
    const std::string& label) {
    const std::uint64_t actual = perft(board, depth);
    suite.require(
        actual == expected,
        label + " depth " + std::to_string(depth) + ": actual=" + std::to_string(actual)
            + ", expected=" + std::to_string(expected),
        __FILE__,
        __LINE__);
}

} // namespace

void test_perft(TestSuite& suite) {
    chess::Position initial;
    constexpr std::array<std::uint64_t, 4> expected = {
        20,
        400,
        8'902,
        197'281,
    };

    for (int depth = 1; depth <= static_cast<int>(expected.size()); ++depth) {
        requirePerft(suite, initial, depth, expected[static_cast<std::size_t>(depth - 1)], "initial");
    }

    chess::Position kiwipete = chess::Position::fromFen(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    requirePerft(suite, kiwipete, 1, 48, "kiwipete");
    requirePerft(suite, kiwipete, 2, 2'039, "kiwipete");
    requirePerft(suite, kiwipete, 3, 97'862, "kiwipete");

    chess::Position enPassantAndChecks = chess::Position::fromFen(
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    requirePerft(suite, enPassantAndChecks, 1, 14, "en-passant-and-checks");
    requirePerft(suite, enPassantAndChecks, 2, 191, "en-passant-and-checks");
    requirePerft(suite, enPassantAndChecks, 3, 2'812, "en-passant-and-checks");

    chess::Position promotionsAndCastling = chess::Position::fromFen(
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    requirePerft(suite, promotionsAndCastling, 1, 6, "promotions-and-castling");
    requirePerft(suite, promotionsAndCastling, 2, 264, "promotions-and-castling");
    requirePerft(suite, promotionsAndCastling, 3, 9'467, "promotions-and-castling");
}
