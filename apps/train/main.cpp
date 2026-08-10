/**
 * @file main.cpp
 * @brief Démarrer un nouvel entraînement ou reprendre un point de contrôle.
 */

#include "training/genetic_trainer.hpp"
#include "training/training_config.hpp"
#include "util/cli_options.hpp"

#include <exception>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::filesystem::path uniqueRunDirectory(
    const std::filesystem::path& root,
    unsigned int seed) {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    // Les millisecondes et la graine réduisent les collisions tout en gardant
    // un nom de dossier interprétable lors de l’analyse des résultats.
    std::ostringstream name;
    name << std::put_time(&localTime, "%Y%m%d-%H%M%S")
         << '-' << std::setw(3) << std::setfill('0') << milliseconds.count()
         << "_seed" << seed;
    return root / name.str();
}

} // namespace

int main(int argc, char** argv) {
    try {
        // === Chargement et surcharge de la configuration ===

        const util::CliOptions cli(argc, argv);
        if (cli.has("--help") || cli.has("-h")) {
            std::cout
                << "Usage: train_genetic --config <file> [options]\n\n"
                << "Options:\n"
                << "  --generations <n>          Number of generations\n"
                << "  --population <n>           Initial population (must be even)\n"
                << "  --seed <n>                 Reproducible random seed\n"
                << "  --max-halfmoves <n>        Maximum half-moves per full match\n"
                << "  --quiescence-depth <n>     Maximum quiescence half-moves\n"
                << "  --threads <n>              Number of concurrent evaluations\n"
                << "  --fitness-mode <mode>      matches or corpus\n"
                << "  --corpus <file>            Labeled corpus for corpus mode\n"
                << "  --training-positions <file> Starting positions for matches\n"
                << "  --search-space <file>      Parameters and bounds to optimize\n"
                << "  --run-root <directory>     Parent directory for new runs\n"
                << "  --log-dir <directory>      Explicit log directory\n"
                << "  --output <file>            Explicit best-model path\n";
            return 0;
        }
        training::GeneticOptions options;
        std::string configPath = cli.value("--config");
        if (!configPath.empty()) {
            options = training::loadGeneticOptionsFromJson(configPath);
        }

        std::string searchSpacePath = cli.value("--search-space", options.searchSpacePath);
        if (!searchSpacePath.empty()) {
            options.searchSpacePath = searchSpacePath;
            options.searchSpace = training::loadSearchSpaceFromJson(searchSpacePath);
        }

        if (cli.has("--population")) {
            const int population = cli.integer("--population", 0);
            if (population <= 0) {
                throw std::invalid_argument("--population must be positive");
            }
            options.populationSize = static_cast<std::size_t>(population);
            if (!options.generationSchedule.empty()) {
                options.generationSchedule.front().populationSize = options.populationSize;
            }
        }
        if (cli.has("--generations")) {
            options.generations = cli.integer("--generations", options.generations);
        }
        if (cli.has("--seed")) {
            const int seed = cli.integer("--seed", static_cast<int>(options.seed));
            if (seed < 0) {
                throw std::invalid_argument("--seed must be non-negative");
            }
            options.seed = static_cast<unsigned int>(seed);
        }
        if (cli.has("--max-halfmoves")) {
            options.maxHalfMoves = cli.integer("--max-halfmoves", options.maxHalfMoves);
        }
        if (cli.has("--quiescence-depth")) {
            options.quiescenceMaxPly = cli.integer(
                "--quiescence-depth",
                options.quiescenceMaxPly);
            if (options.quiescenceMaxPly < 1) {
                throw std::invalid_argument("--quiescence-depth must be positive");
            }
        }
        if (cli.has("--threads")) {
            options.threads = cli.integer("--threads", options.threads);
        }
        options.trainingPositionsPath = cli.value("--training-positions", options.trainingPositionsPath);
        options.fitnessMode = cli.value("--fitness-mode", options.fitnessMode);
        options.corpusPath = cli.value("--corpus", options.corpusPath);
        options.runRoot = cli.value("--run-root", options.runRoot);
        // Chaque lancement reçoit par défaut son propre dossier immuable. Les
        // options explicites restent disponibles pour les pipelines automatisés.
        const std::filesystem::path runDirectory =
            uniqueRunDirectory(options.runRoot, options.seed);
        options.logDir = cli.value("--log-dir", runDirectory.string());
        options.outputPath = cli.value(
            "--output",
            (runDirectory / "best_model.json").string());

        // === Chargement des données d’évaluation ===

        if (options.fitnessMode == "corpus") {
            options.trainingPositions.clear();
        } else if (options.trainingPositionsPath == "none" || options.trainingPositionsPath == "-") {
            options.trainingPositionsPath.clear();
            options.trainingPositions.clear();
        } else if (!options.trainingPositionsPath.empty()) {
            if (!std::filesystem::exists(options.trainingPositionsPath)) {
                if (cli.has("--training-positions")) {
                    throw std::runtime_error("training positions file not found: " + options.trainingPositionsPath);
                }
            } else {
                options.trainingPositions = training::loadTrainingPositionsFromFenFile(options.trainingPositionsPath);
            }
        }
        if (options.fitnessMode == "corpus") {
            options.corpus = training::loadCorpus(options.corpusPath);
        } else if (options.fitnessMode != "matches") {
            throw std::invalid_argument("--fitness-mode must be matches or corpus");
        }

        // === Exécution de l’entraînement ===

        training::GeneticTrainer algorithm(options);
        algorithm.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
