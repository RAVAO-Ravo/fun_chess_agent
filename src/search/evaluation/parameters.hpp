/**
 * @file parameters.hpp
 * @brief Définir, borner et sérialiser les paramètres du moteur.
 */

#pragma once

#include "chess/piece_type.hpp"

#include <string>

namespace ai {

constexpr int MinSearchDepth = 1;
constexpr int MaxSearchDepth = 10;
constexpr int MinLmrMinPly = 0;
constexpr int MaxLmrMinPly = MaxSearchDepth;
constexpr int MinLmrFullDepthMoves = 3;
constexpr int MaxLmrFullDepthMoves = 64;

enum class SearchMode {
    Classic,
    Instinct,
    InstinctLmr,
};

/**
 * @struct EvaluationParameters
 * @brief Regrouper les choix de recherche et les poids de l’évaluation.
 *
 * Toutes les valeurs peuvent être chargées depuis un modèle entraîné, puis
 * sont bornées avant de participer à un calcul.
 */
struct EvaluationParameters {
    int searchDepth = 2;
    SearchMode searchMode = SearchMode::Classic;
    int lmrMinPly = 3;
    int lmrFullDepthMoves = 4;

    int pawnValue = 100;
    int knightValue = 320;
    int bishopValue = 330;
    int rookValue = 500;
    int queenValue = 900;
    int kingValue = 20000;

    int doubledPawnPenalty = 20;
    int isolatedPawnPenalty = 12;
    int passedPawnBonus = 35;
    int protectedPawnBonus = 8;
    int mobilityBonus = 2;
    int bishopPairBonus = 25;
    int kingShieldBonus = 8;
    int undevelopedMinorPenalty = 8;

};

/** @brief Construire un jeu de paramètres conventionnel et immédiatement sûr. */
EvaluationParameters defaultEvaluationParameters();
/** @brief Projeter tous les paramètres dans leurs bornes absolues. */
void clampParameters(EvaluationParameters& parameters);
/** @brief Décoder le nom externe d’un mode de recherche. */
SearchMode parseSearchMode(const std::string& value);
/** @brief Sérialiser un mode sous son nom de configuration stable. */
std::string searchModeName(SearchMode mode);
/** @brief Lire la valeur entraînée correspondant à un type de pièce. */
int pieceValue(const EvaluationParameters& parameters, chess::PieceType type);

/** @brief Charger un modèle JSON en complétant les champs absents. */
EvaluationParameters loadParametersFromJson(const std::string& path);
/** @brief Écrire un modèle JSON borné et adapté à son mode de recherche. */
void saveParametersToJson(const EvaluationParameters& parameters, const std::string& path);

} // namespace ai
