/**
 * @file genetic_trainer.hpp
 * @brief Déclarer le cycle de sélection et d’évolution des individus.
 */

#pragma once

#include "training/individual.hpp"
#include "training/corpus.hpp"
#include "training/population.hpp"
#include "training/search_space.hpp"
#include "training/tournament.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace training {

/**
 * @struct GenerationSettings
 * @brief Définir les réglages d’une plage de générations.
 */
struct GenerationSettings {
    int from = 1;
    int to = 1;
    std::size_t populationSize = 10;
    double mutationIndividualFraction = 1.0;
    double mutationRateScalar = 0.10;
    double mutationRateDepth = 0.03;
    double mutationScaleFraction = 0.10;
};

/**
 * @struct GeneticOptions
 * @brief Regrouper toutes les entrées nécessaires à un entraînement reproductible.
 */
struct GeneticOptions {
    std::size_t populationSize = 10;
    int generations = 3;
    unsigned int seed = 42;
    int maxHalfMoves = 120;
    int quiescenceMaxPly = 10;
    int threads = 1;
    std::string outputPath;
    std::string logDir;
    std::string runRoot = "runs";
    std::string searchSpacePath = "config/training/search_space.json";
    std::string trainingPositionsPath = "data/openings/generated/training_positions.fen";
    std::string fitnessMode = "matches";
    std::string corpusPath = "data/positions/training_corpus.tsv";
    SearchSpace searchSpace = SearchSpace::defaults();
    std::vector<chess::Position> trainingPositions;
    std::vector<CorpusSample> corpus;
    std::vector<GenerationSettings> generationSchedule;
};

/**
 * @class GeneticTrainer
 * @brief Faire évoluer une population et conserver les résultats de chaque étape.
 *
 * Le formateur peut adapter la taille et la mutation selon un calendrier. Il
 * délègue les parties au Tournament et conserve les sorties nécessaires à la
 * comparaison ou à la reprise d’une exécution.
 */
class GeneticTrainer {
public:
    /** @brief Valider et conserver les options d’une exécution. */
    explicit GeneticTrainer(GeneticOptions options);

    /** @brief Exécuter toutes les générations et retourner le meilleur individu. */
    Individual run();

private:
    GenerationSettings settingsForGeneration(int generation) const;
    void resizePopulation(
        Population& population,
        std::size_t targetSize,
        std::mt19937& rng,
        const GenerationSettings& settings) const;
    Individual chooseFinalBest(Population& population, std::mt19937& rng) const;
    void logGeneration(int generation, const Population& population) const;
    void writeRunMetadata() const;
    void printGenerationProgress(
        int generation,
        std::size_t completedPairs,
        std::size_t totalPairs,
        double elapsedSeconds) const;
    void printGenerationSummary(
        int generation,
        const Population& population,
        const GenerationSettings& settings,
        double elapsedSeconds) const;

    GeneticOptions options_;
};

} // namespace training
