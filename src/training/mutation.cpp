/**
 * @file mutation.cpp
 * @brief Faire évoluer les individus à l’intérieur d’un espace borné.
 */

#include "training/mutation.hpp"

#include <algorithm>

namespace training {

namespace {

int randomInt(std::mt19937& rng, int low, int high) {
    return std::uniform_int_distribution<int>(low, high)(rng);
}

bool chance(std::mt19937& rng, double probability) {
    return std::bernoulli_distribution(probability)(rng);
}

void mutateBoundedInt(std::mt19937& rng, int& value, IntRange range, double scaleFraction) {
    int width = std::max(1, range.max - range.min);
    // L’amplitude dépend de l’espace autorisé : une même fraction produit une
    // exploration proportionnée pour les petites et grandes valeurs.
    int deltaLimit = std::max(1, static_cast<int>(width * scaleFraction));
    value += randomInt(rng, -deltaLimit, deltaLimit);
    value = std::max(range.min, std::min(value, range.max));
}

} // namespace

ai::EvaluationParameters Mutation::randomParameters(const SearchSpace& searchSpace, std::mt19937& rng) {
    return randomParametersFromSearchSpace(searchSpace, rng);
}

Individual Mutation::crossover(const Individual& parentA, const Individual& parentB, std::mt19937& rng) {
    // Le croisement uniforme laisse chaque paramètre hériter indépendamment
    // d’un parent, ce qui recombine les caractéristiques sans moyenne artificielle.
    ai::EvaluationParameters child;
    const ai::EvaluationParameters& a = parentA.parameters();
    const ai::EvaluationParameters& b = parentB.parameters();

    child.searchDepth = chance(rng, 0.5) ? a.searchDepth : b.searchDepth;
    child.searchMode = chance(rng, 0.5) ? a.searchMode : b.searchMode;
    child.lmrMinPly = chance(rng, 0.5) ? a.lmrMinPly : b.lmrMinPly;
    child.lmrFullDepthMoves = chance(rng, 0.5) ? a.lmrFullDepthMoves : b.lmrFullDepthMoves;
    child.pawnValue = chance(rng, 0.5) ? a.pawnValue : b.pawnValue;
    child.knightValue = chance(rng, 0.5) ? a.knightValue : b.knightValue;
    child.bishopValue = chance(rng, 0.5) ? a.bishopValue : b.bishopValue;
    child.rookValue = chance(rng, 0.5) ? a.rookValue : b.rookValue;
    child.queenValue = chance(rng, 0.5) ? a.queenValue : b.queenValue;
    child.kingValue = chance(rng, 0.5) ? a.kingValue : b.kingValue;
    child.doubledPawnPenalty = chance(rng, 0.5) ? a.doubledPawnPenalty : b.doubledPawnPenalty;
    child.isolatedPawnPenalty = chance(rng, 0.5) ? a.isolatedPawnPenalty : b.isolatedPawnPenalty;
    child.passedPawnBonus = chance(rng, 0.5) ? a.passedPawnBonus : b.passedPawnBonus;
    child.protectedPawnBonus = chance(rng, 0.5) ? a.protectedPawnBonus : b.protectedPawnBonus;
    child.mobilityBonus = chance(rng, 0.5) ? a.mobilityBonus : b.mobilityBonus;
    child.bishopPairBonus = chance(rng, 0.5) ? a.bishopPairBonus : b.bishopPairBonus;
    child.kingShieldBonus = chance(rng, 0.5) ? a.kingShieldBonus : b.kingShieldBonus;
    child.undevelopedMinorPenalty = chance(rng, 0.5) ? a.undevelopedMinorPenalty : b.undevelopedMinorPenalty;

    ai::clampParameters(child);
    return Individual(child);
}

void Mutation::mutate(
    Individual& individual,
    const SearchSpace& searchSpace,
    std::mt19937& rng,
    double mutationRateScalar,
    double mutationRateDepth,
    double mutationScaleFraction) {
    ai::EvaluationParameters& parameters = individual.parameters();

    // La profondeur utilise un taux séparé : son coût informatique et son effet
    // discret justifient une évolution plus prudente que les poids scalaires.
    if (chance(rng, mutationRateDepth)) {
        parameters.searchDepth += chance(rng, 0.5) ? 1 : -1;
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.pawnValue, searchSpace.pawnValue, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.knightValue, searchSpace.knightValue, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.bishopValue, searchSpace.bishopValue, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.rookValue, searchSpace.rookValue, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.queenValue, searchSpace.queenValue, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.kingValue, searchSpace.kingValue, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.doubledPawnPenalty, searchSpace.doubledPawnPenalty, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.isolatedPawnPenalty, searchSpace.isolatedPawnPenalty, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.passedPawnBonus, searchSpace.passedPawnBonus, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.protectedPawnBonus, searchSpace.protectedPawnBonus, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.mobilityBonus, searchSpace.mobilityBonus, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.bishopPairBonus, searchSpace.bishopPairBonus, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(rng, parameters.kingShieldBonus, searchSpace.kingShieldBonus, mutationScaleFraction);
    }
    if (chance(rng, mutationRateScalar)) {
        mutateBoundedInt(
            rng,
            parameters.undevelopedMinorPenalty,
            searchSpace.undevelopedMinorPenalty,
            mutationScaleFraction);
    }

    // Le double bornage respecte d’abord l’expérience configurée, puis les
    // invariants absolus acceptés par le moteur.
    clampToSearchSpace(parameters, searchSpace);
    ai::clampParameters(parameters);
}

} // namespace training
