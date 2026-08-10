/**
 * @file corpus.hpp
 * @brief Déclarer les exemples supervisés utilisés pour noter les individus.
 */

#pragma once

#include "chess/position.hpp"
#include "search/evaluation/parameters.hpp"

#include <string>
#include <vector>

namespace training {

/**
 * @struct CorpusSample
 * @brief Associer une position à son score blanc cible.
 */
struct CorpusSample {
    chess::Position position;
    int targetScore = 0;
};

/** @brief Charger des couples FEN-score depuis un fichier tabulé. */
std::vector<CorpusSample> loadCorpus(const std::string& path);
/** @brief Mesurer l’erreur des paramètres sur l’ensemble du corpus. */
double corpusFitness(
    const ai::EvaluationParameters& parameters,
    const std::vector<CorpusSample>& corpus);

} // namespace training
