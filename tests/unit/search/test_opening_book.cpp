/**
 * @file test_opening_book.cpp
 * @brief Vérifier le chargement et la sélection des coups d’ouverture.
 */

#include "test_util.hpp"

#include "search/opening_book.hpp"
#include "training/tournament.hpp"

#include <filesystem>
#include <fstream>
#include <random>

void test_opening_book(TestSuite& suite) {
    ai::OpeningBook book = ai::OpeningBook::fromLines({
        "e2e4 e7e5 g1f3",
        "e2e4 c7c5",
        "d2d4",
    });
    std::mt19937 rng(7);

    chess::Position board;
    std::optional<chess::Move> first = book.findMove(board, ai::OpeningBookMode::Competition, rng);
    REQUIRE(suite, first.has_value());
    REQUIRE_EQ(suite, first->toUci(), std::string("e2e4"));

    board.makeMove(*first);
    std::optional<chess::Move> blackReply = book.findMove(board, ai::OpeningBookMode::Competition, rng);
    REQUIRE(suite, blackReply.has_value());
    REQUIRE_EQ(suite, blackReply->toUci(), std::string("e7e5"));

    board.makeMove(*blackReply);
    std::optional<chess::Move> whiteReply = book.findMove(board, ai::OpeningBookMode::Competition, rng);
    REQUIRE(suite, whiteReply.has_value());
    REQUIRE_EQ(suite, whiteReply->toUci(), std::string("g1f3"));

    chess::Position outOfBook = chess::Position::fromFen("rnbqkbnr/pppppppp/8/8/3P4/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 1");
    REQUIRE(suite, !book.findMove(outOfBook, ai::OpeningBookMode::Competition, rng).has_value());

    REQUIRE_EQ(suite, ai::parseOpeningBookMode("chill"), ai::OpeningBookMode::Chill);
    REQUIRE_EQ(suite, ai::parseOpeningBookMode("competition"), ai::OpeningBookMode::Competition);

    std::filesystem::create_directories("/tmp/probcomp_eval_smoke");
    const std::string path = "/tmp/probcomp_eval_smoke/training_positions.fen";
    {
        std::ofstream output(path);
        output << "# comment\n";
        output << "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3\n";
    }

    std::vector<chess::Position> positions = training::loadTrainingPositionsFromFenFile(path);
    REQUIRE_EQ(suite, positions.size(), static_cast<std::size_t>(1));
    REQUIRE_EQ(suite, positions.front().sideToMove(), chess::Color::Black);
}
