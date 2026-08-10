/**
 * @file population.cpp
 * @brief Fournir les opérations de taille, accès et mélange d’une population.
 */

#include "training/population.hpp"

#include <stdexcept>
#include <utility>

namespace training {

namespace {

void requireEven(std::size_t size) {
    if (size == 0 || size % 2 != 0) {
        throw std::invalid_argument("population size must be even and greater than zero");
    }
}

} // namespace

Population::Population(std::vector<Individual> individuals)
    : individuals_(std::move(individuals)) {
    requireEven(individuals_.size());
}

std::size_t Population::size() const {
    return individuals_.size();
}

Individual& Population::operator[](std::size_t index) {
    return individuals_.at(index);
}

std::vector<Individual>& Population::individuals() {
    return individuals_;
}

const std::vector<Individual>& Population::individuals() const {
    return individuals_;
}

void Population::shuffle(std::mt19937& rng) {
    std::shuffle(individuals_.begin(), individuals_.end(), rng);
}

} // namespace training
