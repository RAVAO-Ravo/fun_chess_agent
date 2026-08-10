/**
 * @file genetic_trainer.cpp
 * @brief Piloter un entraînement génétique reproductible et reprenable.
 *
 * Chaque génération combine une présélection peu coûteuse sur corpus, un
 * tournoi entre candidats, l’élitisme, le croisement et la mutation. Les
 * graines, journaux et points de reprise assurent la traçabilité du calcul.
 */

#include "training/genetic_trainer.hpp"

#include "search/evaluation/parameters.hpp"
#include "training/mutation.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <future>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>

namespace training {

namespace {

struct PairOutcome {
    std::size_t index = 0;
    Individual first;
    Individual second;
    Individual survivor;
};

struct IndividualPairScore {
    double points = 0.0;
    double dominance = 0.0;
    double conversionSpeed = 0.0;
    double resilience = 0.0;
    int games = 0;
};

double scoreFor(GameResult result, chess::Color color) {
    if (result == GameResult::Draw) {
        return 0.5;
    }
    bool whiteWon = result == GameResult::WhiteWin;
    return (whiteWon && color == chess::Color::White) || (!whiteWon && color == chess::Color::Black) ? 1.0 : 0.0;
}

double clamp01(double value) {
    return std::max(0.0, std::min(value, 1.0));
}

double evaluationForColor(const MatchResult& result, chess::Color color) {
    return color == chess::Color::White ? result.finalEvaluation : -result.finalEvaluation;
}

double normalizedAdvantage(const MatchResult& result, chess::Color color) {
    // La tangente hyperbolique comprime les écarts matériels extrêmes dans
    // [0, 1] afin qu’ils ne dominent jamais entièrement le résultat de partie.
    return clamp01((std::tanh(evaluationForColor(result, color) / 800.0) + 1.0) / 2.0);
}

double depthEfficiency(const Individual& individual, const SearchSpace& searchSpace) {
    const int minDepth = searchSpace.searchDepth.min;
    const int maxDepth = searchSpace.searchDepth.max;
    if (maxDepth <= minDepth) {
        return 0.5;
    }
    // À force comparable, une profondeur plus faible reçoit un léger avantage
    // car elle représente un moteur moins coûteux en ressources.
    return clamp01(
        static_cast<double>(
            maxDepth - individual.parameters().searchDepth)
        / static_cast<double>(maxDepth - minDepth));
}

void recordResult(Individual& individual, double points) {
    if (points >= 1.0) {
        individual.addWin();
    } else if (points <= 0.0) {
        individual.addLoss();
    } else {
        individual.addDraw();
    }
}

void addGameScore(IndividualPairScore& score, const MatchResult& result, chess::Color color, int maxHalfMoves) {
    const double points = scoreFor(result.result, color);
    const double dominance = normalizedAdvantage(result, color);
    const double remainingGameFraction = maxHalfMoves > 0
        ? clamp01(1.0 - static_cast<double>(result.halfMovesPlayed) / static_cast<double>(maxHalfMoves))
        : 0.0;

    score.points += points;
    score.dominance += dominance;
    if (points >= 1.0) {
        // Une victoire rapide récompense la capacité à convertir l’avantage.
        score.conversionSpeed += remainingGameFraction;
        score.resilience += 1.0;
    } else if (points <= 0.0) {
        score.resilience += dominance;
    } else {
        score.conversionSpeed += 0.5;
        score.resilience += dominance;
    }
    ++score.games;
}

double pairFitness(const Individual& individual, const IndividualPairScore& score, const SearchSpace& searchSpace) {
    if (score.games == 0) {
        return 1.0;
    }

    const double games = static_cast<double>(score.games);
    const double resultScore = clamp01(score.points / games);
    const double dominanceScore = clamp01(score.dominance / games);
    const double speedScore = clamp01(score.conversionSpeed / games);
    const double resilienceScore = clamp01(score.resilience / games);
    // Le résultat reste le signal principal. Les critères secondaires départagent
    // des bilans égaux sans pouvoir transformer une défaite en bonne performance.
    const double gameQuality = clamp01(0.50 * dominanceScore + 0.30 * speedScore + 0.20 * resilienceScore);
    const double fitness = 1.0
        + 1.60 * resultScore
        + 0.25 * gameQuality
        + 0.15 * depthEfficiency(individual, searchSpace);
    return std::max(1.0, std::min(fitness, 3.0));
}

bool chance(std::mt19937& rng, double probability) {
    return std::bernoulli_distribution(probability)(rng);
}

PairOutcome playPair(
    std::size_t pairIndex,
    Individual a,
    Individual b,
    int maxHalfMoves,
    int quiescenceMaxPly,
    const SearchSpace& searchSpace,
    const std::vector<chess::Position>& trainingPositions) {
    Tournament tournament(maxHalfMoves, trainingPositions, quiescenceMaxPly);
    IndividualPairScore aScore;
    IndividualPairScore bScore;

    // Chaque paire joue avec les couleurs inversées pour neutraliser l’avantage
    // du premier trait et les biais propres à la position de départ.
    MatchResult first = tournament.playGame(a, b, pairIndex);
    addGameScore(aScore, first, chess::Color::White, maxHalfMoves);
    addGameScore(bScore, first, chess::Color::Black, maxHalfMoves);
    recordResult(a, scoreFor(first.result, chess::Color::White));
    recordResult(b, scoreFor(first.result, chess::Color::Black));

    MatchResult second = tournament.playGame(b, a, pairIndex);
    addGameScore(aScore, second, chess::Color::Black, maxHalfMoves);
    addGameScore(bScore, second, chess::Color::White, maxHalfMoves);
    recordResult(a, scoreFor(second.result, chess::Color::Black));
    recordResult(b, scoreFor(second.result, chess::Color::White));

    a.setFitness(pairFitness(a, aScore, searchSpace));
    b.setFitness(pairFitness(b, bScore, searchSpace));

    if (aScore.points > bScore.points) {
        return PairOutcome{pairIndex, a, b, a};
    }
    if (bScore.points > aScore.points) {
        return PairOutcome{pairIndex, a, b, b};
    }
    if (a.fitness() >= b.fitness()) {
        return PairOutcome{pairIndex, a, b, a};
    }
    return PairOutcome{pairIndex, a, b, b};
}

double duelPoints(
    const Individual& candidate,
    const Individual& opponent,
    const Tournament& tournament,
    std::size_t startingPositionIndex) {
    double points = 0.0;
    const MatchResult first =
        tournament.playGame(candidate, opponent, startingPositionIndex);
    points += scoreFor(first.result, chess::Color::White);
    const MatchResult second =
        tournament.playGame(opponent, candidate, startingPositionIndex);
    points += scoreFor(second.result, chess::Color::Black);
    return points;
}

void rankSurvivors(
    std::vector<Individual>& survivors,
    const std::vector<Individual>& champions,
    int maxHalfMoves,
    int quiescenceMaxPly,
    const std::vector<chess::Position>& trainingPositions,
    std::size_t generation) {
    if (survivors.empty()) {
        return;
    }
    std::stable_sort(
        survivors.begin(),
        survivors.end(),
        [](const Individual& left, const Individual& right) {
            return left.fitness() > right.fitness();
        });

    const std::size_t finalistCount =
        std::min(survivors.size(), std::max<std::size_t>(2, survivors.size() / 2));
    // La validation approfondie reste limitée aux meilleurs survivants. Elle
    // améliore la confiance du classement sans imposer un tournoi exhaustif.
    const int validationHalfMoves = std::min(maxHalfMoves, 60);
    const Tournament tournament(
        validationHalfMoves,
        trainingPositions,
        quiescenceMaxPly);
    std::vector<double> points(finalistCount, 0.0);
    std::vector<int> games(finalistCount, 0);

    for (std::size_t first = 0; first < finalistCount; ++first) {
        for (std::size_t second = first + 1; second < finalistCount; ++second) {
            const double firstPoints = duelPoints(
                survivors[first],
                survivors[second],
                tournament,
                generation + first + second);
            points[first] += firstPoints;
            points[second] += 2.0 - firstPoints;
            games[first] += 2;
            games[second] += 2;
        }
        for (std::size_t championIndex = 0;
             championIndex < std::min<std::size_t>(champions.size(), 2);
             ++championIndex) {
            // Les champions antérieurs servent d’ancrage : une génération ne
            // progresse pas seulement relativement à ses propres concurrents.
            points[first] += duelPoints(
                survivors[first],
                champions[championIndex],
                tournament,
                generation + first + championIndex);
            games[first] += 2;
        }
    }

    for (std::size_t index = 0; index < finalistCount; ++index) {
        const double normalized = games[index] > 0
            ? points[index] / static_cast<double>(games[index])
            : 0.5;
        survivors[index].setFitness(3.0 + 2.0 * normalized);
    }
    std::stable_sort(
        survivors.begin(),
        survivors.end(),
        [](const Individual& left, const Individual& right) {
            return left.fitness() > right.fitness();
        });
}

std::vector<Individual> selectByCorpus(
    Population& population,
    const std::vector<CorpusSample>& corpus) {
    for (Individual& individual : population.individuals()) {
        individual.setFitness(corpusFitness(individual.parameters(), corpus));
    }
    // La moitié supérieure suffit à alimenter l’élitisme et la reproduction,
    // avec un coût linéaire bien inférieur à celui de parties complètes.
    std::vector<Individual> ranked = population.individuals();
    std::stable_sort(
        ranked.begin(),
        ranked.end(),
        [](const Individual& left, const Individual& right) {
            return left.fitness() > right.fitness();
        });
    ranked.resize(ranked.size() / 2);
    return ranked;
}

std::string generationLogHeader() {
    return "generation,individual_id,fitness,wins,draws,losses,depth,"
        "search_mode,lmr_min_ply,lmr_full_depth_moves,"
        "pawn,knight,bishop,rook,queen,"
        "doubled_pawn_penalty,isolated_pawn_penalty,passed_pawn_bonus,protected_pawn_bonus,"
        "mobility_bonus,bishop_pair_bonus,king_shield_bonus,undeveloped_minor_penalty";
}

} // namespace

GeneticTrainer::GeneticTrainer(GeneticOptions options)
    : options_(std::move(options)) {
    if (options_.populationSize == 0 || options_.populationSize % 2 != 0) {
        throw std::invalid_argument("population size must be even");
    }
    if (options_.quiescenceMaxPly < 1) {
        throw std::invalid_argument("quiescence depth must be positive");
    }
}

Individual GeneticTrainer::run() {
    if (options_.outputPath.empty() || options_.logDir.empty()) {
        throw std::invalid_argument("training output and log directory must be configured");
    }
    std::filesystem::create_directories(options_.logDir);
    writeRunMetadata();

    // === Population initiale ===

    // Une graine unique alimente toutes les décisions pseudo-aléatoires ; une
    // exécution identique peut ainsi être reproduite exactement.
    std::mt19937 rng(options_.seed);
    std::vector<Individual> initialIndividuals;
    initialIndividuals.reserve(options_.populationSize);
    for (std::size_t i = 0; i < options_.populationSize; ++i) {
        initialIndividuals.emplace_back(Mutation::randomParameters(options_.searchSpace, rng));
    }
    Population population(std::move(initialIndividuals));
    std::cout << "Training started.\n";
    std::cout << "Generations: " << options_.generations
              << " | initial population: " << options_.populationSize
              << " | max_halfmoves: " << options_.maxHalfMoves
              << " | quiescence_max_ply: " << options_.quiescenceMaxPly
              << " | training positions: " << options_.trainingPositions.size()
              << " | threads: " << options_.threads
              << " | seed: " << options_.seed << "\n";
    std::cout << "Output: " << options_.outputPath << "\n";

    std::vector<Individual> champions;
    for (int generation = 0; generation < options_.generations; ++generation) {
        // === Évaluation de la génération ===

        auto startedAt = std::chrono::steady_clock::now();
        GenerationSettings settings = settingsForGeneration(generation + 1);
        resizePopulation(population, settings.populationSize, rng, settings);

        for (Individual& individual : population.individuals()) {
            individual.resetStats();
        }

        population.shuffle(rng);
        const std::size_t pairCount = population.size() / 2;
        std::vector<Individual> survivors;
        std::size_t completedPairs = 0;

        if (options_.fitnessMode == "corpus") {
            // Le corpus évalue directement les poids contre des scores de
            // référence, sans simuler de partie.
            survivors = selectByCorpus(population, options_.corpus);
            completedPairs = pairCount;
            printGenerationProgress(
                generation + 1,
                completedPairs,
                pairCount,
                0.0);
        } else {
            survivors.resize(pairCount);
            // Les résultats asynchrones sont réinsérés à leur indice d’origine
            // afin que l’ordonnancement des threads ne change pas la population.
            auto storeOutcome = [&](PairOutcome outcome) {
                population[outcome.index * 2] = std::move(outcome.first);
                population[outcome.index * 2 + 1] = std::move(outcome.second);
                survivors[outcome.index] = std::move(outcome.survivor);
                ++completedPairs;
                const auto now = std::chrono::steady_clock::now();
                const std::chrono::duration<double> elapsed = now - startedAt;
                printGenerationProgress(
                    generation + 1,
                    completedPairs,
                    pairCount,
                    elapsed.count());
            };

            printGenerationProgress(generation + 1, completedPairs, pairCount, 0.0);
            if (options_.threads <= 1 || pairCount <= 1) {
                for (std::size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
                    storeOutcome(playPair(
                        pairIndex,
                        population[pairIndex * 2],
                        population[pairIndex * 2 + 1],
                        std::min(options_.maxHalfMoves, 30),
                        options_.quiescenceMaxPly,
                        options_.searchSpace,
                        options_.trainingPositions));
                }
            } else {
                // Le nombre de futures actives est strictement borné par la
                // configuration pour éviter une création incontrôlée de threads.
                std::vector<std::future<PairOutcome>> active;
                active.reserve(static_cast<std::size_t>(options_.threads));

                for (std::size_t pairIndex = 0; pairIndex < pairCount; ++pairIndex) {
                    active.push_back(std::async(
                        std::launch::async,
                        playPair,
                        pairIndex,
                        population[pairIndex * 2],
                        population[pairIndex * 2 + 1],
                        std::min(options_.maxHalfMoves, 30),
                        options_.quiescenceMaxPly,
                        options_.searchSpace,
                        std::cref(options_.trainingPositions)));

                    if (active.size() >= static_cast<std::size_t>(options_.threads)) {
                        storeOutcome(active.front().get());
                        active.erase(active.begin());
                    }
                }
                for (std::future<PairOutcome>& future : active) {
                    storeOutcome(future.get());
                }
            }
        }
        std::cout << "\n";

        if (options_.fitnessMode == "matches") {
            // === Validation des survivants ===

            rankSurvivors(
                survivors,
                champions,
                options_.maxHalfMoves,
                options_.quiescenceMaxPly,
                options_.trainingPositions,
                static_cast<std::size_t>(generation));
        }
        if (!survivors.empty()) {
            // La courte mémoire de champions limite les régressions entre
            // générations sans figer durablement la diversité génétique.
            champions.insert(champions.begin(), survivors.front());
            if (champions.size() > 3) {
                champions.resize(3);
            }
            ai::saveParametersToJson(survivors.front().parameters(), options_.outputPath);
        }

        logGeneration(generation, population);

        auto finishedAt = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = finishedAt - startedAt;
        printGenerationSummary(generation + 1, population, settings, elapsed.count());

        // === Reproduction ===

        // Les survivants sont conservés tels quels : cet élitisme garantit que
        // la génération suivante contient au moins ses meilleurs paramètres.
        std::vector<Individual> next;
        next.reserve(population.size());
        next.insert(next.end(), survivors.begin(), survivors.end());

        std::vector<double> parentWeights;
        parentWeights.reserve(survivors.size());
        for (std::size_t index = 0; index < survivors.size(); ++index) {
            // Le rang détermine le poids de reproduction, plutôt que la fitness
            // brute dont l’échelle varie entre les modes d’évaluation.
            parentWeights.push_back(static_cast<double>(survivors.size() - index));
        }
        std::discrete_distribution<std::size_t> parentDist(
            parentWeights.begin(),
            parentWeights.end());
        while (next.size() < population.size()) {
            const Individual& parentA = survivors[parentDist(rng)];
            const Individual& parentB = survivors[parentDist(rng)];
            Individual child = Mutation::crossover(parentA, parentB, rng);
            if (chance(rng, settings.mutationIndividualFraction)) {
                Mutation::mutate(
                    child,
                    options_.searchSpace,
                    rng,
                    settings.mutationRateScalar,
                    settings.mutationRateDepth,
                    settings.mutationScaleFraction);
            } else {
                clampToSearchSpace(child.parameters(), options_.searchSpace);
                ai::clampParameters(child.parameters());
            }
            next.push_back(child);
        }

        population = Population(std::move(next));
    }

    // === Validation finale ===

    Individual best = chooseFinalBest(population, rng);
    ai::saveParametersToJson(best.parameters(), options_.outputPath);

    std::cout << "Training completed.\n";
    std::cout << "Best individual: " << best.id() << "\n";
    std::cout << "Final fitness: " << best.fitness() << "\n";
    std::cout << "Parameters saved to " << options_.outputPath << "\n";
    return best;
}

GenerationSettings GeneticTrainer::settingsForGeneration(int generation) const {
    for (const GenerationSettings& settings : options_.generationSchedule) {
        if (generation >= settings.from && generation <= settings.to) {
            return settings;
        }
    }
    GenerationSettings settings;
    settings.from = 1;
    settings.to = options_.generations;
    settings.populationSize = options_.populationSize;
    return settings;
}

void GeneticTrainer::resizePopulation(
    Population& population,
    std::size_t targetSize,
    std::mt19937& rng,
    const GenerationSettings& settings) const {
    if (targetSize == 0 || targetSize % 2 != 0) {
        throw std::invalid_argument("scheduled population size must be even");
    }
    if (population.size() == targetSize) {
        return;
    }

    std::vector<Individual> resized = population.individuals();
    // Mélanger avant une réduction évite de conserver systématiquement les
    // premiers identifiants lorsque le calendrier diminue la population.
    std::shuffle(resized.begin(), resized.end(), rng);

    if (resized.size() > targetSize) {
        resized.resize(targetSize);
        population = Population(std::move(resized));
        return;
    }

    while (resized.size() < targetSize) {
        if (resized.size() < 2) {
            resized.emplace_back(Mutation::randomParameters(options_.searchSpace, rng));
            continue;
        }
        std::uniform_int_distribution<std::size_t> dist(0, resized.size() - 1);
        // Une augmentation de population crée de nouveaux individus depuis le
        // patrimoine courant au lieu de réinjecter uniquement du hasard.
        Individual child = Mutation::crossover(resized[dist(rng)], resized[dist(rng)], rng);
        Mutation::mutate(
            child,
            options_.searchSpace,
            rng,
            settings.mutationRateScalar,
            settings.mutationRateDepth,
            settings.mutationScaleFraction);
        resized.push_back(child);
    }

    population = Population(std::move(resized));
}

Individual GeneticTrainer::chooseFinalBest(Population& population, std::mt19937& rng) const {
    if (options_.fitnessMode == "corpus") {
        std::vector<Individual> ranked = selectByCorpus(population, options_.corpus);
        if (ranked.empty()) {
            throw std::runtime_error("cannot select a corpus finalist");
        }
        return ranked.front();
    }

    Tournament tournament(
        options_.maxHalfMoves,
        options_.trainingPositions,
        options_.quiescenceMaxPly);
    std::vector<double> scores(population.size(), 0.0);
    // Quatre voisins circulaires bornent le coût final à O(n) confrontations
    // tout en offrant plusieurs adversaires et les deux couleurs à chacun.
    std::size_t opponents = std::min<std::size_t>(4, population.size() - 1);

    for (std::size_t i = 0; i < population.size(); ++i) {
        for (std::size_t offset = 1; offset <= opponents; ++offset) {
            std::size_t j = (i + offset) % population.size();
            MatchResult first = tournament.playGame(population[i], population[j], i + offset);
            scores[i] += scoreFor(first.result, chess::Color::White);
            scores[j] += scoreFor(first.result, chess::Color::Black);

            MatchResult second = tournament.playGame(population[j], population[i], i + offset);
            scores[i] += scoreFor(second.result, chess::Color::Black);
            scores[j] += scoreFor(second.result, chess::Color::White);
        }
    }

    double bestScore = -1.0;
    std::vector<std::size_t> bestIndices;
    for (std::size_t i = 0; i < scores.size(); ++i) {
        population[i].setFitness(scores[i]);
        if (scores[i] > bestScore) {
            bestScore = scores[i];
            bestIndices = {i};
        } else if (scores[i] == bestScore) {
            bestIndices.push_back(i);
        }
    }

    // Un tirage déterminé par la graine évite de favoriser arbitrairement le
    // premier identifiant en cas d’égalité parfaite.
    std::uniform_int_distribution<std::size_t> dist(0, bestIndices.size() - 1);
    return population[bestIndices[dist(rng)]];
}

void GeneticTrainer::logGeneration(int generation, const Population& population) const {
    std::filesystem::create_directories(options_.logDir);
    const std::string header = generationLogHeader();
    const std::filesystem::path path =
        std::filesystem::path(options_.logDir) / "generations.csv";
    // L’ajout préserve les générations antérieures et l’en-tête n’est écrit
    // qu’à la création du journal.
    const bool writeHeader = !std::filesystem::exists(path);
    std::ofstream output(path, std::ios::app);
    if (!output) {
        throw std::runtime_error("cannot write training log to " + path.string());
    }
    if (writeHeader) {
        output << header << '\n';
    }
    for (const Individual& individual : population.individuals()) {
        const ai::EvaluationParameters& p = individual.parameters();
        output << generation << ','
               << individual.id() << ','
               << individual.fitness() << ','
               << individual.wins() << ','
               << individual.draws() << ','
               << individual.losses() << ','
               << p.searchDepth << ','
               << ai::searchModeName(p.searchMode);
        if (p.searchMode == ai::SearchMode::InstinctLmr) {
            output << ',' << p.lmrMinPly << ',' << p.lmrFullDepthMoves;
        } else {
            output << ",,";
        }
        output << ',' << p.pawnValue << ','
               << p.knightValue << ','
               << p.bishopValue << ','
               << p.rookValue << ','
               << p.queenValue << ','
               << p.doubledPawnPenalty << ','
               << p.isolatedPawnPenalty << ','
               << p.passedPawnBonus << ','
               << p.protectedPawnBonus << ','
               << p.mobilityBonus << ','
               << p.bishopPairBonus << ','
               << p.kingShieldBonus << ','
               << p.undevelopedMinorPenalty << '\n';
    }
}

void GeneticTrainer::writeRunMetadata() const {
    const std::filesystem::path path =
        std::filesystem::path(options_.logDir) / "run_metadata.json";
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("cannot write run metadata to " + path.string());
    }

    // Les informations de compilation accompagnent les paramètres scientifiques
    // pour distinguer deux résultats obtenus dans des environnements différents.
#if defined(_WIN32)
    constexpr const char* systemName = "windows";
#elif defined(__APPLE__)
    constexpr const char* systemName = "macos";
#elif defined(__linux__)
    constexpr const char* systemName = "linux";
#else
    constexpr const char* systemName = "unknown";
#endif

#if INTPTR_MAX == INT64_MAX
    constexpr const char* architecture = "64-bit";
#elif INTPTR_MAX == INT32_MAX
    constexpr const char* architecture = "32-bit";
#else
    constexpr const char* architecture = "unknown";
#endif

#ifndef CHESS_BUILD_TYPE
#define CHESS_BUILD_TYPE "unknown"
#endif
#ifndef CHESS_COMPILER_ID
#define CHESS_COMPILER_ID "unknown"
#endif
#ifndef CHESS_COMPILER_VERSION
#define CHESS_COMPILER_VERSION "unknown"
#endif

    output << "{\n"
           << "  \"schema_version\": 2,\n"
           << "  \"engine_search_version\": \"iterative-negamax-qsearch-lmr-v2\",\n"
           << "  \"log_file\": \"generations.csv\",\n"
           << "  \"seed\": " << options_.seed << ",\n"
           << "  \"max_halfmoves\": " << options_.maxHalfMoves << ",\n"
           << "  \"quiescence_max_ply\": " << options_.quiescenceMaxPly << ",\n"
           << "  \"population_size\": " << options_.populationSize << ",\n"
           << "  \"generations\": " << options_.generations << ",\n"
           << "  \"threads\": " << options_.threads << ",\n"
           << "  \"training_positions\": " << options_.trainingPositions.size() << ",\n"
           << "  \"fitness_mode\": \"" << options_.fitnessMode << "\",\n"
           << "  \"corpus_path\": \"" << options_.corpusPath << "\",\n"
           << "  \"colors_reversed\": "
           << (options_.fitnessMode == "matches" ? "true" : "false") << ",\n"
           << "  \"search_space_path\": \"" << options_.searchSpacePath << "\",\n"
           << "  \"system\": \"" << systemName << "\",\n"
           << "  \"architecture\": \"" << architecture << "\",\n"
           << "  \"compiler\": \"" << CHESS_COMPILER_ID << ' ' << CHESS_COMPILER_VERSION << "\",\n"
           << "  \"build_type\": \"" << CHESS_BUILD_TYPE << "\"\n"
           << "}\n";
}

void GeneticTrainer::printGenerationProgress(
    int generation,
    std::size_t completedPairs,
    std::size_t totalPairs,
    double elapsedSeconds) const {
    const int percent = totalPairs > 0
        ? static_cast<int>(
              (100.0 * static_cast<double>(completedPairs))
              / static_cast<double>(totalPairs))
        : 100;
    const double pairsPerSecond = elapsedSeconds > 0.0
        ? static_cast<double>(completedPairs) / elapsedSeconds
        : 0.0;
    const double remainingPairs = completedPairs <= totalPairs
        ? static_cast<double>(totalPairs - completedPairs)
        : 0.0;
    const double etaSeconds =
        pairsPerSecond > 0.0 ? remainingPairs / pairsPerSecond : 0.0;

    std::cout << "\r"
              << "[gen " << generation << "/" << options_.generations << "] "
              << "matchs " << completedPairs << "/" << totalPairs
              << " (" << percent << "%)"
              << " elapsed=" << std::fixed << std::setprecision(1) << elapsedSeconds << "s"
              << " eta=" << etaSeconds << "s"
              << std::flush;
}

void GeneticTrainer::printGenerationSummary(
    int generation,
    const Population& population,
    const GenerationSettings& settings,
    double elapsedSeconds) const {
    if (population.size() == 0) {
        return;
    }

    double totalFitness = 0.0;
    double totalDepth = 0.0;
    double totalPawn = 0.0;
    double totalKnight = 0.0;
    double totalBishop = 0.0;
    double totalRook = 0.0;
    double totalQueen = 0.0;
    double bestFitness = -std::numeric_limits<double>::infinity();
    const Individual* best = nullptr;

    for (const Individual& individual : population.individuals()) {
        const ai::EvaluationParameters& parameters = individual.parameters();
        totalFitness += individual.fitness();
        totalDepth += parameters.searchDepth;
        totalPawn += parameters.pawnValue;
        totalKnight += parameters.knightValue;
        totalBishop += parameters.bishopValue;
        totalRook += parameters.rookValue;
        totalQueen += parameters.queenValue;
        if (individual.fitness() > bestFitness) {
            bestFitness = individual.fitness();
            best = &individual;
        }
    }

    const double size = static_cast<double>(population.size());
    const int percent = options_.generations > 0 ? static_cast<int>((100.0 * generation) / options_.generations) : 100;

    std::cout << std::fixed << std::setprecision(2)
              << "[gen " << generation << "/" << options_.generations << " | " << percent << "%]"
              << " pop=" << population.size()
              << " matches=" << (population.size() / 2)
              << " survivors=" << (population.size() / 2)
              << " time=" << elapsedSeconds << "s"
              << " best_fitness=" << bestFitness
              << " avg_fitness=" << (totalFitness / size)
              << " avg_depth=" << (totalDepth / size)
              << " mutation_individual=" << settings.mutationIndividualFraction
              << "\n";

    if (best != nullptr) {
        const ai::EvaluationParameters& parameters = best->parameters();
        std::cout << "  best_id=" << best->id()
                  << " W/D/L=" << best->wins() << "/" << best->draws() << "/" << best->losses()
                  << " depth=" << parameters.searchDepth
                  << " values=[P" << parameters.pawnValue
                  << " N" << parameters.knightValue
                  << " B" << parameters.bishopValue
                  << " R" << parameters.rookValue
                  << " Q" << parameters.queenValue << "]"
                  << " eval=[Dbl" << parameters.doubledPawnPenalty
                  << " Iso" << parameters.isolatedPawnPenalty
                  << " Pass" << parameters.passedPawnBonus
                  << " Prot" << parameters.protectedPawnBonus
                  << " Mob" << parameters.mobilityBonus
                  << " Pair" << parameters.bishopPairBonus
                  << " Shield" << parameters.kingShieldBonus
                  << " Dev" << parameters.undevelopedMinorPenalty << "]"
                  << "\n";
    }

    std::cout << "  avg_values=[P" << (totalPawn / size)
              << " N" << (totalKnight / size)
              << " B" << (totalBishop / size)
              << " R" << (totalRook / size)
              << " Q" << (totalQueen / size) << "]"
              << " saved=" << options_.outputPath
              << "\n";
}

} // namespace training
