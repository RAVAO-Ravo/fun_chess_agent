/**
 * @file search_benchmark.cpp
 * @brief Mesurer les modes de recherche sur un corpus stable de positions.
 *
 * La sortie CSV inclut l’environnement de compilation, le résultat et les
 * compteurs internes afin de comparer les optimisations sans confondre vitesse
 * brute, profondeur atteinte et qualité du coup.
 */

#include "chess/position.hpp"
#include "search/evaluation/parameters.hpp"
#include "search/searcher.hpp"
#include "util/cli_options.hpp"

#include <array>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct BenchmarkPosition {
    std::string id;
    std::string category;
    chess::Position position;
};

std::vector<BenchmarkPosition> loadPositions(const std::string& path) {
    // Le format tabulé sépare identifiant, catégorie et FEN. Les catégories
    // permettent de détecter une régression propre aux positions tactiques.
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot load benchmark positions from " + path);
    }
    std::vector<BenchmarkPosition> positions;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line.starts_with('#')) {
            continue;
        }
        const std::size_t first = line.find('\t');
        const std::size_t second =
            first == std::string::npos ? std::string::npos : line.find('\t', first + 1);
        if (first == std::string::npos || second == std::string::npos) {
            throw std::runtime_error("invalid benchmark row: " + line);
        }
        positions.push_back(BenchmarkPosition{
            line.substr(0, first),
            line.substr(first + 1, second - first - 1),
            chess::Position::fromFen(line.substr(second + 1)),
        });
    }
    return positions;
}

std::vector<ai::SearchMode> selectedModes(const std::string& mode) {
    // Le mot-clé ``all`` exécute les modes dans un ordre stable pour produire
    // des fichiers directement comparables entre versions.
    if (mode == "all") {
        return {
            ai::SearchMode::Classic,
            ai::SearchMode::Instinct,
            ai::SearchMode::InstinctLmr,
        };
    }
    return {ai::parseSearchMode(mode)};
}

const char* systemName() {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}

} // namespace

int main(int argc, char** argv) {
    try {
        // === Configuration du protocole de mesure ===

        const util::CliOptions options(argc, argv);
        const std::string positionPath =
            options.value("--positions", "benchmarks/positions/search.tsv");
        const std::string outputPath = options.value("--output");
        const int depth = options.integer("--depth", 4);
        const int timeMs = options.integer("--time-ms", 0);

        ai::EvaluationParameters base = ai::defaultEvaluationParameters();
        const std::string parameterPath = options.value("--params");
        if (!parameterPath.empty()) {
            base = ai::loadParametersFromJson(parameterPath);
        }
        if (options.has("--lmr-min-ply")) {
            base.lmrMinPly =
                options.integer("--lmr-min-ply", base.lmrMinPly);
        }
        if (options.has("--lmr-full-depth-moves")) {
            base.lmrFullDepthMoves = options.integer(
                "--lmr-full-depth-moves",
                base.lmrFullDepthMoves);
        }
        ai::clampParameters(base);

        // La sortie standard facilite une inspection ponctuelle ; le fichier
        // optionnel est destiné à l’historique versionné des mesures.
        std::ofstream fileOutput;
        std::ostream* output = &std::cout;
        if (!outputPath.empty()) {
            fileOutput.open(outputPath);
            if (!fileOutput) {
                throw std::runtime_error("cannot write benchmark to " + outputPath);
            }
            output = &fileOutput;
        }

        *output << "# schema_version=2\n"
                << "# system=" << systemName() << "\n"
                << "# architecture=" << CHESS_ARCHITECTURE << "\n"
                << "# compiler=" << CHESS_COMPILER_ID << ' ' << CHESS_COMPILER_VERSION << "\n"
                << "# build_type=" << CHESS_BUILD_TYPE << "\n"
                << "# optimization=" << CHESS_OPTIMIZATION << "\n"
                << "id,category,mode,requested_depth,completed_depth,best_move,score,"
                << "time_us,nodes,qnodes,nps,tt_probes,tt_hits,tt_entries,cutoffs,stopped\n";

        // === Exécution de la matrice modes × positions ===

        const std::vector<BenchmarkPosition> positions = loadPositions(positionPath);
        for (const ai::SearchMode mode :
             selectedModes(options.value("--mode", "all"))) {
            ai::EvaluationParameters parameters = base;
            parameters.searchMode = mode;
            parameters.searchDepth = depth;
            ai::clampParameters(parameters);

            for (const BenchmarkPosition& benchmark : positions) {
                ai::Searcher searcher(parameters);
                ai::SearchLimits limits;
                limits.maxDepth = depth;
                limits.timeLimit = std::chrono::milliseconds(timeMs);
                limits.usePrincipalVariationSearch = !options.has("--no-pvs");
                limits.useAspirationWindows = !options.has("--no-aspiration");
                // Une nouvelle instance empêche la table de transposition d’une
                // position de favoriser artificiellement la suivante.
                const ai::SearchResult result =
                    searcher.search(benchmark.position, limits);
                *output << benchmark.id << ','
                        << benchmark.category << ','
                        << ai::searchModeName(mode) << ','
                        << depth << ','
                        << result.stats.completedDepth << ','
                        << result.bestMove.toUci() << ','
                        << result.score << ','
                        << result.stats.elapsed.count() << ','
                        << result.stats.nodes << ','
                        << result.stats.quiescenceNodes << ','
                        << static_cast<std::uint64_t>(result.stats.nodesPerSecond()) << ','
                        << result.stats.transpositionProbes << ','
                        << result.stats.transpositionHits << ','
                        << result.stats.transpositionEntries << ','
                        << result.stats.betaCutoffs << ','
                        << (result.stoppedByTime ? 1 : 0) << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
