/**
 * @file training_config.hpp
 * @brief Déclarer le chargement de la configuration d’entraînement.
 */

#pragma once

#include "training/genetic_trainer.hpp"
#include "training/search_space.hpp"

#include <string>

namespace training {

/** @brief Charger, valider et résoudre les chemins d’une configuration JSON. */
GeneticOptions loadGeneticOptionsFromJson(const std::string& path);

} // namespace training
