/**
 * @file population.hpp
 * @brief Encapsuler une collection mutable d’individus.
 */

#pragma once

#include "training/individual.hpp"

#include <cstddef>
#include <random>
#include <vector>

namespace training {

/**
 * @class Population
 * @brief Encapsuler les individus manipulés par une génération.
 */
class Population {
public:
    /** @brief Prendre possession d’un ensemble initial d’individus. */
    explicit Population(std::vector<Individual> individuals);

    std::size_t size() const;

    Individual& operator[](std::size_t index);

    std::vector<Individual>& individuals();
    const std::vector<Individual>& individuals() const;

    /** @brief Mélanger les paires futures avec le générateur reproductible. */
    void shuffle(std::mt19937& rng);

private:
    std::vector<Individual> individuals_;
};

} // namespace training
