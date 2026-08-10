/**
 * @file evaluator.hpp
 * @brief Déclarer l’évaluation statique d’une position.
 */

#pragma once

#include "search/evaluation/parameters.hpp"
#include "chess/position.hpp"

namespace ai {

/**
 * @class Evaluator
 * @brief Calculer un score statique positif lorsque les Blancs sont favorisés.
 */
class Evaluator {
public:
    /** @brief Conserver les poids utilisés pour toutes les évaluations. */
    explicit Evaluator(EvaluationParameters parameters = defaultEvaluationParameters());

    /** @brief Agréger matériel, mobilité, pions, développement et sécurité. */
    int evaluate(const chess::Position& board) const;

private:
    EvaluationParameters parameters_;
};

} // namespace ai
