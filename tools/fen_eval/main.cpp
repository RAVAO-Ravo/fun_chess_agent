/**
 * @file main.cpp
 * @brief Évaluer une position FEN avec un modèle choisi.
 */

#include "search/evaluation/parameters.hpp"
#include "search/evaluation/evaluator.hpp"
#include "chess/position.hpp"
#include "util/cli_options.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    try {
        // Cet outil isole l’évaluation statique de toute recherche, ce qui
        // facilite l’inspection d’un modèle entraîné.
        const util::CliOptions options(argc, argv);
        std::string fen = options.value("--fen");
        if (fen.empty()) {
            std::cerr << "Usage: fen_eval --fen \"<fen>\" [--params data/models/current.json]\n";
            return 1;
        }
        ai::EvaluationParameters parameters = ai::defaultEvaluationParameters();
        std::string paramsPath = options.value("--params");
        if (!paramsPath.empty()) {
            parameters = ai::loadParametersFromJson(paramsPath);
        }
        chess::Position board = chess::Position::fromFen(fen);
        std::cout << ai::Evaluator(parameters).evaluate(board) << "\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
