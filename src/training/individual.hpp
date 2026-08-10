/**
 * @file individual.hpp
 * @brief Représenter un jeu de paramètres et ses résultats d’entraînement.
 */

#pragma once

#include "search/evaluation/parameters.hpp"

#include <cstddef>

namespace training {

/**
 * @class Individual
 * @brief Porter un génome de paramètres et ses mesures de sélection.
 *
 * L’identifiant croissant permet de suivre un individu dans les journaux même
 * lorsqu’il est copié entre une population, un tournoi et une génération.
 */
class Individual {
public:
    /** @brief Créer un individu depuis les paramètres par défaut. */
    Individual();
    /** @brief Créer un individu depuis un génome explicite. */
    explicit Individual(ai::EvaluationParameters parameters);

    std::size_t id() const;

    const ai::EvaluationParameters& parameters() const;
    ai::EvaluationParameters& parameters();

    double fitness() const;
    void setFitness(double value);

    int wins() const;
    int losses() const;
    int draws() const;

    /** @brief Réinitialiser uniquement le bilan avant une nouvelle génération. */
    void resetStats();
    void addWin();
    void addLoss();
    void addDraw();

private:
    static std::size_t nextId();

    std::size_t id_ = 0;
    ai::EvaluationParameters parameters_;
    double fitness_ = 0.0;
    int wins_ = 0;
    int losses_ = 0;
    int draws_ = 0;
};

} // namespace training
