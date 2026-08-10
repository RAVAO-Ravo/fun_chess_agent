/**
 * @file test_protocol.cpp
 * @brief Vérifier les échanges complets du protocole interactif.
 */

#include "test_util.hpp"

#include "protocol/command_processor.hpp"
#include "protocol/game_session.hpp"
#include "search/evaluation/parameters.hpp"

#include <sstream>
#include <string>

void test_protocol(TestSuite& suite) {
    ai::EvaluationParameters parameters = ai::defaultEvaluationParameters();
    parameters.searchDepth = 1;
    app::GameSession session(ai::ChessAI(parameters), chess::Color::White);
    std::ostringstream output;
    app::CommandProcessor processor(session, output);

    REQUIRE(suite, processor.processLine("new_game white"));
    REQUIRE(suite, output.str().find("board_fen ") != std::string::npos);
    REQUIRE(suite, output.str().find("status playing") != std::string::npos);

    output.str("");
    output.clear();
    REQUIRE(suite, processor.processLine("human_move e2e4"));
    REQUIRE(suite, output.str().starts_with("ok\nboard_fen "));
    REQUIRE_EQ(suite, session.board().sideToMove(), chess::Color::Black);

    output.str("");
    output.clear();
    REQUIRE(suite, processor.processLine("get_legal_moves"));
    REQUIRE(suite, output.str().starts_with("legal_moves "));

    output.str("");
    output.clear();
    REQUIRE(suite, processor.processLine("go"));
    REQUIRE_EQ(suite, output.str(), std::string("error unknown command: go\n"));

    output.str("");
    output.clear();
    REQUIRE(suite, processor.processLine("get_board"));
    REQUIRE_EQ(suite, output.str(), std::string("error unknown command: get_board\n"));

    output.str("");
    output.clear();
    REQUIRE(suite, !processor.processLine("quit"));
    REQUIRE_EQ(suite, output.str(), std::string("ok\n"));
}
