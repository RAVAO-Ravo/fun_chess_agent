/**
 * @file individual.cpp
 * @brief Gérer l’identité, les paramètres et le bilan d’un individu.
 */

#include "training/individual.hpp"

#include <atomic>
#include <utility>

namespace training {

Individual::Individual()
    : id_(nextId())
    , parameters_(ai::defaultEvaluationParameters()) {
}

Individual::Individual(ai::EvaluationParameters parameters)
    : id_(nextId())
    , parameters_(std::move(parameters)) {
    ai::clampParameters(parameters_);
}

std::size_t Individual::nextId() {
    static std::atomic_size_t id{0};
    return id++;
}

std::size_t Individual::id() const {
    return id_;
}

const ai::EvaluationParameters& Individual::parameters() const {
    return parameters_;
}

ai::EvaluationParameters& Individual::parameters() {
    return parameters_;
}

double Individual::fitness() const {
    return fitness_;
}

void Individual::setFitness(double value) {
    fitness_ = value;
}

int Individual::wins() const {
    return wins_;
}

int Individual::losses() const {
    return losses_;
}

int Individual::draws() const {
    return draws_;
}

void Individual::resetStats() {
    fitness_ = 0.0;
    wins_ = 0;
    losses_ = 0;
    draws_ = 0;
}

void Individual::addWin() {
    ++wins_;
}

void Individual::addLoss() {
    ++losses_;
}

void Individual::addDraw() {
    ++draws_;
}

} // namespace training
