/**
 * @file mutation.hpp
 * @brief Déclarer la génération, le croisement et la mutation des paramètres.
 */

#pragma once

#include "search/evaluation/parameters.hpp"
#include "training/individual.hpp"
#include "training/search_space.hpp"

#include <random>

namespace training {

/**
 * @class Mutation
 * @brief Fournir les opérateurs génétiques sans état partagé.
 */
class Mutation {
public:
    /** @brief Tirer chaque paramètre dans les bornes de l’expérience. */
    static ai::EvaluationParameters randomParameters(
        const SearchSpace& searchSpace,
        std::mt19937& rng);
    /** @brief Effectuer un croisement uniforme des deux génomes. */
    static Individual crossover(const Individual& parentA, const Individual& parentB, std::mt19937& rng);
    /** @brief Muter indépendamment les gènes puis appliquer les bornes. */
    static void mutate(
        Individual& individual,
        const SearchSpace& searchSpace,
        std::mt19937& rng,
        double mutationRateScalar,
        double mutationRateDepth,
        double mutationScaleFraction);
};

} // namespace training
