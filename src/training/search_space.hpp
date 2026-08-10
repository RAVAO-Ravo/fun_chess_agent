/**
 * @file search_space.hpp
 * @brief Définir les bornes explorables de chaque paramètre entraînable.
 */

#pragma once

#include "search/evaluation/parameters.hpp"

#include <random>
#include <string>

namespace training {

/**
 * @struct IntRange
 * @brief Définir deux bornes entières inclusives.
 */
struct IntRange {
    int min = 0;
    int max = 0;
};

/**
 * @struct SearchSpace
 * @brief Décrire les valeurs autorisées pendant une expérience génétique.
 *
 * Cet espace peut être plus étroit que les bornes absolues du moteur afin de
 * concentrer l’apprentissage sur une région plausible.
 */
struct SearchSpace {
    IntRange searchDepth{1, 3};
    ai::SearchMode searchMode = ai::SearchMode::Classic;
    int lmrMinPly = 3;
    int lmrFullDepthMoves = 4;
    IntRange pawnValue{80, 130};
    IntRange knightValue{250, 380};
    IntRange bishopValue{260, 390};
    IntRange rookValue{430, 580};
    IntRange queenValue{780, 1050};
    IntRange kingValue{18000, 24000};
    IntRange doubledPawnPenalty{0, 50};
    IntRange isolatedPawnPenalty{0, 50};
    IntRange passedPawnBonus{0, 90};
    IntRange protectedPawnBonus{0, 30};
    IntRange mobilityBonus{0, 8};
    IntRange bishopPairBonus{0, 80};
    IntRange kingShieldBonus{0, 30};
    IntRange undevelopedMinorPenalty{0, 40};

    /** @brief Retourner l’espace d’exploration maintenu par défaut. */
    static SearchSpace defaults();
};

/** @brief Charger et valider toutes les bornes depuis JSON. */
SearchSpace loadSearchSpaceFromJson(const std::string& path);
/** @brief Générer un modèle uniforme dans l’espace fourni. */
ai::EvaluationParameters randomParametersFromSearchSpace(const SearchSpace& searchSpace, std::mt19937& rng);
/** @brief Projeter un modèle dans chaque intervalle de l’expérience. */
void clampToSearchSpace(ai::EvaluationParameters& parameters, const SearchSpace& searchSpace);

} // namespace training
