/**
 * @file search_space.cpp
 * @brief Charger les bornes JSON et y projeter les paramètres générés.
 */

#include "training/search_space.hpp"

#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace training {

namespace {

int randomInt(std::mt19937& rng, IntRange range) {
    if (range.min > range.max) {
        throw std::invalid_argument("invalid search-space range");
    }
    return std::uniform_int_distribution<int>(range.min, range.max)(rng);
}

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot load search space from " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

IntRange extractRange(const std::string& content, const std::string& key, IntRange fallback) {
    const std::regex pattern(
        "\"" + key
        + "\"\\s*:\\s*\\{[^}]*\"bounds\"\\s*:\\s*"
          "\\[\\s*(-?\\d+)\\s*,\\s*(-?\\d+)\\s*\\]");
    std::smatch match;
    if (!std::regex_search(content, match, pattern)) {
        return fallback;
    }
    IntRange range{std::stoi(match[1].str()), std::stoi(match[2].str())};
    if (range.min > range.max) {
        throw std::invalid_argument("invalid bounds for " + key);
    }
    return range;
}

std::string extractString(
    const std::string& content,
    const std::string& key,
    const std::string& fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(content, match, pattern)) {
        return match[1].str();
    }
    return fallback;
}

int extractInt(const std::string& content, const std::string& key, int fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (std::regex_search(content, match, pattern)) {
        return std::stoi(match[1].str());
    }
    return fallback;
}

int clampInt(int value, IntRange range) {
    return std::max(range.min, std::min(value, range.max));
}

} // namespace

SearchSpace SearchSpace::defaults() {
    return SearchSpace();
}

SearchSpace loadSearchSpaceFromJson(const std::string& path) {
    const std::string content = readFile(path);
    SearchSpace searchSpace = SearchSpace::defaults();
    searchSpace.searchDepth = extractRange(content, "searchDepth", searchSpace.searchDepth);
    searchSpace.searchMode = ai::parseSearchMode(
        extractString(
            content,
            "searchMode",
            ai::searchModeName(searchSpace.searchMode)));
    searchSpace.lmrMinPly = extractInt(content, "lmrMinPly", searchSpace.lmrMinPly);
    searchSpace.lmrFullDepthMoves = extractInt(content, "lmrFullDepthMoves", searchSpace.lmrFullDepthMoves);
    searchSpace.pawnValue = extractRange(content, "pawnValue", searchSpace.pawnValue);
    searchSpace.knightValue = extractRange(content, "knightValue", searchSpace.knightValue);
    searchSpace.bishopValue = extractRange(content, "bishopValue", searchSpace.bishopValue);
    searchSpace.rookValue = extractRange(content, "rookValue", searchSpace.rookValue);
    searchSpace.queenValue = extractRange(content, "queenValue", searchSpace.queenValue);
    searchSpace.kingValue = extractRange(content, "kingValue", searchSpace.kingValue);
    searchSpace.doubledPawnPenalty = extractRange(content, "doubledPawnPenalty", searchSpace.doubledPawnPenalty);
    searchSpace.isolatedPawnPenalty = extractRange(content, "isolatedPawnPenalty", searchSpace.isolatedPawnPenalty);
    searchSpace.passedPawnBonus = extractRange(content, "passedPawnBonus", searchSpace.passedPawnBonus);
    searchSpace.protectedPawnBonus = extractRange(content, "protectedPawnBonus", searchSpace.protectedPawnBonus);
    searchSpace.mobilityBonus = extractRange(content, "mobilityBonus", searchSpace.mobilityBonus);
    searchSpace.bishopPairBonus = extractRange(content, "bishopPairBonus", searchSpace.bishopPairBonus);
    searchSpace.kingShieldBonus = extractRange(content, "kingShieldBonus", searchSpace.kingShieldBonus);
    searchSpace.undevelopedMinorPenalty = extractRange(
        content,
        "undevelopedMinorPenalty",
        searchSpace.undevelopedMinorPenalty);
    return searchSpace;
}

ai::EvaluationParameters randomParametersFromSearchSpace(
    const SearchSpace& searchSpace,
    std::mt19937& rng) {
    ai::EvaluationParameters parameters;
    parameters.searchDepth = randomInt(rng, searchSpace.searchDepth);
    parameters.searchMode = searchSpace.searchMode;
    parameters.lmrMinPly = searchSpace.lmrMinPly;
    parameters.lmrFullDepthMoves = searchSpace.lmrFullDepthMoves;
    parameters.pawnValue = randomInt(rng, searchSpace.pawnValue);
    parameters.knightValue = randomInt(rng, searchSpace.knightValue);
    parameters.bishopValue = randomInt(rng, searchSpace.bishopValue);
    parameters.rookValue = randomInt(rng, searchSpace.rookValue);
    parameters.queenValue = randomInt(rng, searchSpace.queenValue);
    parameters.kingValue = randomInt(rng, searchSpace.kingValue);
    parameters.doubledPawnPenalty = randomInt(rng, searchSpace.doubledPawnPenalty);
    parameters.isolatedPawnPenalty = randomInt(rng, searchSpace.isolatedPawnPenalty);
    parameters.passedPawnBonus = randomInt(rng, searchSpace.passedPawnBonus);
    parameters.protectedPawnBonus = randomInt(rng, searchSpace.protectedPawnBonus);
    parameters.mobilityBonus = randomInt(rng, searchSpace.mobilityBonus);
    parameters.bishopPairBonus = randomInt(rng, searchSpace.bishopPairBonus);
    parameters.kingShieldBonus = randomInt(rng, searchSpace.kingShieldBonus);
    parameters.undevelopedMinorPenalty = randomInt(rng, searchSpace.undevelopedMinorPenalty);
    clampToSearchSpace(parameters, searchSpace);
    ai::clampParameters(parameters);
    return parameters;
}

void clampToSearchSpace(ai::EvaluationParameters& parameters, const SearchSpace& searchSpace) {
    parameters.searchDepth = clampInt(parameters.searchDepth, searchSpace.searchDepth);
    parameters.searchMode = searchSpace.searchMode;
    parameters.lmrMinPly = std::max(
        ai::MinLmrMinPly,
        std::min(searchSpace.lmrMinPly, ai::MaxLmrMinPly));
    parameters.lmrFullDepthMoves = std::max(
        ai::MinLmrFullDepthMoves,
        std::min(
            searchSpace.lmrFullDepthMoves,
            ai::MaxLmrFullDepthMoves));
    parameters.pawnValue = clampInt(parameters.pawnValue, searchSpace.pawnValue);
    parameters.knightValue = clampInt(parameters.knightValue, searchSpace.knightValue);
    parameters.bishopValue = clampInt(parameters.bishopValue, searchSpace.bishopValue);
    parameters.rookValue = clampInt(parameters.rookValue, searchSpace.rookValue);
    parameters.queenValue = clampInt(parameters.queenValue, searchSpace.queenValue);
    parameters.kingValue = clampInt(parameters.kingValue, searchSpace.kingValue);
    parameters.doubledPawnPenalty =
        clampInt(parameters.doubledPawnPenalty, searchSpace.doubledPawnPenalty);
    parameters.isolatedPawnPenalty =
        clampInt(parameters.isolatedPawnPenalty, searchSpace.isolatedPawnPenalty);
    parameters.passedPawnBonus =
        clampInt(parameters.passedPawnBonus, searchSpace.passedPawnBonus);
    parameters.protectedPawnBonus =
        clampInt(parameters.protectedPawnBonus, searchSpace.protectedPawnBonus);
    parameters.mobilityBonus = clampInt(parameters.mobilityBonus, searchSpace.mobilityBonus);
    parameters.bishopPairBonus = clampInt(parameters.bishopPairBonus, searchSpace.bishopPairBonus);
    parameters.kingShieldBonus = clampInt(parameters.kingShieldBonus, searchSpace.kingShieldBonus);
    parameters.undevelopedMinorPenalty = clampInt(
        parameters.undevelopedMinorPenalty,
        searchSpace.undevelopedMinorPenalty);
}

} // namespace training
