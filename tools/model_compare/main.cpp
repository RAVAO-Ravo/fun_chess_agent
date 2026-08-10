/**
 * @file main.cpp
 * @brief Comparer un modèle candidat à une référence hors entraînement.
 *
 * L’outil combine une mesure sur corpus de validation et deux parties à
 * couleurs inversées pour éviter de conclure depuis un seul indicateur.
 */

#include "search/evaluation/parameters.hpp"
#include "training/corpus.hpp"
#include "training/individual.hpp"
#include "training/tournament.hpp"
#include "util/cli_options.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
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
        // === Évaluation supervisée ===

        const util::CliOptions options(argc, argv);
        const std::string candidatePath = options.value("--candidate");
        const std::string referencePath = options.value("--reference");
        if (candidatePath.empty() || referencePath.empty()) {
            throw std::invalid_argument(
                "--candidate and --reference are required");
        }
        const std::string corpusPath = options.value(
            "--corpus",
            "data/positions/holdout_corpus.tsv");
        const int maxHalfMoves = options.integer("--max-halfmoves", 300);

        const ai::EvaluationParameters candidateParameters =
            ai::loadParametersFromJson(candidatePath);
        const ai::EvaluationParameters referenceParameters =
            ai::loadParametersFromJson(referencePath);
        const std::vector<training::CorpusSample> corpus =
            training::loadCorpus(corpusPath);

        std::cout << "candidate_corpus_fitness "
                  << training::corpusFitness(candidateParameters, corpus) << '\n'
                  << "reference_corpus_fitness "
                  << training::corpusFitness(referenceParameters, corpus) << '\n';

        // === Confrontation à couleurs inversées ===

        const training::Individual candidate(candidateParameters);
        const training::Individual reference(referenceParameters);
        const training::Tournament tournament(maxHalfMoves);
        const training::MatchResult candidateWhite =
            tournament.playGame(candidate, reference);
        const training::MatchResult candidateBlack =
            tournament.playGame(reference, candidate);
        std::cout << "candidate_white " << resultName(candidateWhite.result)
                  << " halfmoves " << candidateWhite.halfMovesPlayed << '\n'
                  << "candidate_black " << resultName(candidateBlack.result)
                  << " halfmoves " << candidateBlack.halfMovesPlayed << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
