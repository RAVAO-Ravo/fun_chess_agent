/**
 * @file main.cpp
 * @brief Jouer une confrontation directe entre deux modèles.
 */

#include "search/evaluation/parameters.hpp"
#include "training/individual.hpp"
#include "training/tournament.hpp"
#include "util/cli_options.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace {

const char* resultName(training::GameResult result) {
    switch (result) {
    case training::GameResult::WhiteWin:
        return "white_win";
    case training::GameResult::BlackWin:
        return "black_win";
    case training::GameResult::Draw:
        return "draw";
    }
    return "draw";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const util::CliOptions options(argc, argv);
        std::string whitePath = options.value("--white");
        std::string blackPath = options.value("--black");
        if (whitePath.empty() || blackPath.empty()) {
            std::cerr << "Usage: self_play --white data/ai1.json --black data/ai2.json [--max-halfmoves 300]\n";
            return 1;
        }
        int maxHalfMoves = options.integer("--max-halfmoves", 300);
        // Les couleurs sont volontairement fixées par les options ; pour une
        // comparaison équilibrée, l’appelant doit lancer aussi la partie inverse.
        training::Individual white(ai::loadParametersFromJson(whitePath));
        training::Individual black(ai::loadParametersFromJson(blackPath));
        training::MatchResult result = training::Tournament(maxHalfMoves).playGame(white, black);
        std::cout << "result " << resultName(result.result)
                  << " halfmoves " << result.halfMovesPlayed
                  << " eval " << result.finalEvaluation << "\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
