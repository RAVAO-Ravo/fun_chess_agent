/**
 * @file test_genetic.cpp
 * @brief Vérifier les briques élémentaires de l’entraînement génétique.
 */

#include "test_util.hpp"

#include "training/training_config.hpp"
#include "training/corpus.hpp"
#include "training/genetic_trainer.hpp"
#include "training/mutation.hpp"
#include "training/population.hpp"

#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string projectFile(const std::string& relativePath) {
    return (std::filesystem::path(CHESS_PROJECT_ROOT) / relativePath).string();
}

} // namespace

void test_genetic(TestSuite& suite) {
    bool threw = false;
    try {
        training::Population invalid(std::vector<training::Individual>(3));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    REQUIRE(suite, threw);

    std::mt19937 rng(42);
    const training::SearchSpace defaultSearchSpace =
        training::SearchSpace::defaults();
    ai::EvaluationParameters randomParams =
        training::Mutation::randomParameters(defaultSearchSpace, rng);
    REQUIRE(suite, randomParams.searchDepth >= 1 && randomParams.searchDepth <= 5);
    REQUIRE(suite, randomParams.pawnValue >= 50 && randomParams.pawnValue <= 200);
    REQUIRE(suite, randomParams.mobilityBonus >= 0 && randomParams.mobilityBonus <= 8);
    REQUIRE(suite, randomParams.passedPawnBonus >= 0 && randomParams.passedPawnBonus <= 90);

    training::Individual a(randomParams);
    training::Individual b(
        training::Mutation::randomParameters(defaultSearchSpace, rng));
    training::Individual child = training::Mutation::crossover(a, b, rng);
    training::Mutation::mutate(
        child,
        defaultSearchSpace,
        rng,
        1.0,
        1.0,
        0.10);
    REQUIRE(suite, child.parameters().searchDepth >= 1 && child.parameters().searchDepth <= 5);
    REQUIRE(suite, child.parameters().queenValue >= 600 && child.parameters().queenValue <= 1400);
    REQUIRE(suite, child.parameters().doubledPawnPenalty >= 0 && child.parameters().doubledPawnPenalty <= 200);
    REQUIRE(suite, child.parameters().mobilityBonus >= 0 && child.parameters().mobilityBonus <= 30);
    training::GeneticOptions options;
    options.populationSize = 2;
    options.generations = 1;
    options.seed = 7;
    options.maxHalfMoves = 2;
    options.quiescenceMaxPly = 3;
    options.outputPath = "/tmp/probcomp_eval_smoke/best.json";
    options.logDir = "/tmp/probcomp_eval_smoke/logs_checked";
    std::filesystem::remove_all(options.logDir);
    training::GeneticTrainer algorithm(options);
    training::Individual best = algorithm.run();
    REQUIRE(suite, std::filesystem::exists(options.outputPath));
    REQUIRE(suite, best.parameters().searchDepth >= 1 && best.parameters().searchDepth <= 5);

    std::ifstream log(std::filesystem::path(options.logDir) / "generations.csv");
    REQUIRE(suite, static_cast<bool>(log));
    const std::filesystem::path metadataPath =
        std::filesystem::path(options.logDir) / "run_metadata.json";
    REQUIRE(suite, std::filesystem::exists(metadataPath));
    std::ifstream metadata(metadataPath);
    bool foundQuiescenceDepth = false;
    std::string metadataLine;
    while (std::getline(metadata, metadataLine)) {
        if (metadataLine.find("\"quiescence_max_ply\": 3") != std::string::npos) {
            foundQuiescenceDepth = true;
        }
    }
    REQUIRE(suite, foundQuiescenceDepth);
    std::string line;
    std::getline(log, line);
    int loggedIndividuals = 0;
    while (std::getline(log, line)) {
        std::vector<std::string> columns;
        std::size_t start = 0;
        while (start <= line.size()) {
            std::size_t comma = line.find(',', start);
            if (comma == std::string::npos) {
                columns.push_back(line.substr(start));
                break;
            }
            columns.push_back(line.substr(start, comma - start));
            start = comma + 1;
        }
        REQUIRE_EQ(suite, columns.size(), static_cast<std::size_t>(23));
        double fitness = std::stod(columns[2]);
        int wins = std::stoi(columns[3]);
        int draws = std::stoi(columns[4]);
        int losses = std::stoi(columns[5]);
        REQUIRE(suite, fitness >= 1.0 && fitness <= 3.0);
        REQUIRE_EQ(suite, wins + draws + losses, 2);
        ++loggedIndividuals;
    }
    REQUIRE_EQ(suite, loggedIndividuals, 2);

    training::SearchSpace searchSpace = training::loadSearchSpaceFromJson(
        projectFile("config/training/search_space.json"));
    REQUIRE_EQ(suite, searchSpace.searchMode, ai::SearchMode::InstinctLmr);
    REQUIRE_EQ(suite, searchSpace.lmrMinPly, 3);
    REQUIRE_EQ(suite, searchSpace.lmrFullDepthMoves, 4);
    REQUIRE_EQ(suite, searchSpace.searchDepth.min, 4);
    REQUIRE_EQ(suite, searchSpace.searchDepth.max, 4);
    REQUIRE_EQ(suite, searchSpace.mobilityBonus.max, 30);
    REQUIRE_EQ(suite, searchSpace.undevelopedMinorPenalty.max, 80);
    training::GeneticOptions loaded = training::loadGeneticOptionsFromJson(projectFile("config/training/genetic.json"));
    REQUIRE_EQ(suite, loaded.generations, 100);
    REQUIRE_EQ(suite, loaded.populationSize, static_cast<std::size_t>(32));
    REQUIRE_EQ(suite, loaded.quiescenceMaxPly, 10);
    REQUIRE(suite, loaded.trainingPositionsPath.find("training_positions.fen") != std::string::npos);
    REQUIRE_EQ(suite, loaded.fitnessMode, std::string("matches"));
    const std::vector<training::CorpusSample> corpus =
        training::loadCorpus(projectFile("data/positions/holdout_corpus.tsv"));
    REQUIRE_EQ(suite, corpus.size(), static_cast<std::size_t>(4));
    const double corpusScore = training::corpusFitness(randomParams, corpus);
    REQUIRE(suite, corpusScore >= 1.0 && corpusScore <= 3.0);
    REQUIRE(suite, !loaded.generationSchedule.empty());
}
