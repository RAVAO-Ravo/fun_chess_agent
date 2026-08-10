/**
 * @file main.cpp
 * @brief Démarrer le moteur interactif ou calculer directement un meilleur coup.
 */

#include "protocol/command_processor.hpp"
#include "protocol/game_session.hpp"
#include "search/evaluation/parameters.hpp"
#include "search/opening_book.hpp"
#include "util/cli_options.hpp"

#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

int main(int argc, char** argv) {
    try {
        // === Configuration du moteur ===

        const util::CliOptions options(argc, argv);
        if (!options.has("--interactive")) {
            std::cerr
                << "Usage: chess_engine --interactive"
                << " [--params data/models/current.json] [--depth 4]"
                << " [--search-mode classic|instinct|instinct_lmr]"
                << " [--book data/openings/generated/book.txt]"
                << " [--book-mode chill|competition] [--time-ms 1000]"
                << " [--diagnostics]\n";
            return 1;
        }

        ai::EvaluationParameters parameters = ai::defaultEvaluationParameters();
        const std::string paramsPath = options.value("--params");
        if (!paramsPath.empty()) {
            parameters = ai::loadParametersFromJson(paramsPath);
        }
        if (options.has("--depth")) {
            parameters.searchDepth = options.integer("--depth", parameters.searchDepth);
        }
        if (options.has("--search-mode")) {
            parameters.searchMode =
                ai::parseSearchMode(options.value("--search-mode"));
        }
        ai::clampParameters(parameters);

        // Une bibliothèque absente par défaut n’empêche pas le moteur de
        // démarrer. Un chemin explicitement demandé reste en revanche une erreur.
        ai::OpeningBook openingBook;
        const std::string bookPath =
            options.value("--book", "data/openings/generated/book.txt");
        if (!options.has("--no-book") && !bookPath.empty()) {
            if (std::filesystem::exists(bookPath)) {
                openingBook = ai::OpeningBook::loadFromFile(bookPath);
            } else if (options.has("--book")) {
                throw std::runtime_error("opening book not found: " + bookPath);
            }
        }

        const ai::OpeningBookMode openingBookMode =
            ai::parseOpeningBookMode(options.value("--book-mode", "chill"));
        app::GameSession session(
            ai::ChessAI(parameters, std::move(openingBook), openingBookMode),
            chess::Color::White);
        ai::SearchLimits limits;
        limits.maxDepth = parameters.searchDepth;
        const int timeLimitMs = options.integer("--time-ms", 0);
        if (timeLimitMs < 0) {
            throw std::invalid_argument("--time-ms must be non-negative");
        }
        limits.timeLimit = std::chrono::milliseconds(timeLimitMs);
        session.setSearchLimits(limits);
        app::CommandProcessor processor(session, std::cout, options.has("--diagnostics"));

        // === Boucle du protocole interactif ===

        // Chaque ligne produit une réponse complète avant la suivante, propriété
        // sur laquelle repose le client graphique synchrone.
        std::string line;
        while (std::getline(std::cin, line) && processor.processLine(line)) {
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
