/**
 * @file parameters.cpp
 * @brief Charger les réglages d’évaluation et de recherche depuis JSON.
 */

#include "search/evaluation/parameters.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace ai {

namespace {

int clampInt(int value, int low, int high) {
    return std::max(low, std::min(value, high));
}

int extractInt(const std::string& content, const std::string& key, int fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (std::regex_search(content, match, pattern)) {
        return std::stoi(match[1].str());
    }
    return fallback;
}

std::string extractString(const std::string& content, const std::string& key, const std::string& fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(content, match, pattern)) {
        return match[1].str();
    }
    return fallback;
}

} // namespace

EvaluationParameters defaultEvaluationParameters() {
    // Ces valeurs conventionnelles fournissent un moteur utilisable avant tout
    // entraînement et servent de repli pour les clés JSON absentes.
    EvaluationParameters parameters;
    parameters.searchDepth = 2;
    parameters.searchMode = SearchMode::Classic;
    parameters.lmrMinPly = 3;
    parameters.lmrFullDepthMoves = 4;
    parameters.pawnValue = 100;
    parameters.knightValue = 320;
    parameters.bishopValue = 330;
    parameters.rookValue = 500;
    parameters.queenValue = 900;
    parameters.kingValue = 20000;
    parameters.doubledPawnPenalty = 20;
    parameters.isolatedPawnPenalty = 12;
    parameters.passedPawnBonus = 35;
    parameters.protectedPawnBonus = 8;
    parameters.mobilityBonus = 2;
    parameters.bishopPairBonus = 25;
    parameters.kingShieldBonus = 8;
    parameters.undevelopedMinorPenalty = 8;
    return parameters;
}

void clampParameters(EvaluationParameters& parameters) {
    // Les bornes absolues protègent la recherche contre un modèle corrompu ou
    // des mutations extrêmes, indépendamment de l’espace d’entraînement.
    parameters.searchDepth = clampInt(parameters.searchDepth, MinSearchDepth, MaxSearchDepth);
    parameters.lmrMinPly = clampInt(parameters.lmrMinPly, MinLmrMinPly, MaxLmrMinPly);
    parameters.lmrFullDepthMoves = clampInt(parameters.lmrFullDepthMoves, MinLmrFullDepthMoves, MaxLmrFullDepthMoves);
    parameters.pawnValue = clampInt(parameters.pawnValue, 50, 200);
    parameters.knightValue = clampInt(parameters.knightValue, 150, 500);
    parameters.bishopValue = clampInt(parameters.bishopValue, 150, 500);
    parameters.rookValue = clampInt(parameters.rookValue, 300, 800);
    parameters.queenValue = clampInt(parameters.queenValue, 600, 1400);
    parameters.kingValue = clampInt(parameters.kingValue, 10000, 50000);
    parameters.doubledPawnPenalty = clampInt(parameters.doubledPawnPenalty, 0, 200);
    parameters.isolatedPawnPenalty = clampInt(parameters.isolatedPawnPenalty, 0, 200);
    parameters.passedPawnBonus = clampInt(parameters.passedPawnBonus, 0, 300);
    parameters.protectedPawnBonus = clampInt(parameters.protectedPawnBonus, 0, 100);
    parameters.mobilityBonus = clampInt(parameters.mobilityBonus, 0, 30);
    parameters.bishopPairBonus = clampInt(parameters.bishopPairBonus, 0, 200);
    parameters.kingShieldBonus = clampInt(parameters.kingShieldBonus, 0, 100);
    parameters.undevelopedMinorPenalty = clampInt(parameters.undevelopedMinorPenalty, 0, 100);
}

SearchMode parseSearchMode(const std::string& value) {
    if (value == "classic") {
        return SearchMode::Classic;
    }
    if (value == "instinct") {
        return SearchMode::Instinct;
    }
    if (value == "instinct_lmr") {
        return SearchMode::InstinctLmr;
    }
    throw std::invalid_argument("invalid searchMode: " + value);
}

std::string searchModeName(SearchMode mode) {
    switch (mode) {
    case SearchMode::Classic:
        return "classic";
    case SearchMode::Instinct:
        return "instinct";
    case SearchMode::InstinctLmr:
        return "instinct_lmr";
    }
    return "classic";
}

int pieceValue(const EvaluationParameters& parameters, chess::PieceType type) {
    switch (type) {
    case chess::PieceType::Pawn:
        return parameters.pawnValue;
    case chess::PieceType::Knight:
        return parameters.knightValue;
    case chess::PieceType::Bishop:
        return parameters.bishopValue;
    case chess::PieceType::Rook:
        return parameters.rookValue;
    case chess::PieceType::Queen:
        return parameters.queenValue;
    case chess::PieceType::King:
        return parameters.kingValue;
    case chess::PieceType::None:
        return 0;
    }
    return 0;
}

EvaluationParameters loadParametersFromJson(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot load parameters from " + path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();

    // Le chargement est tolérant aux champs absents : chaque clé remplace une
    // valeur par défaut, puis l’ensemble est borné avant utilisation.
    EvaluationParameters parameters = defaultEvaluationParameters();
    parameters.searchDepth = extractInt(content, "searchDepth", parameters.searchDepth);
    parameters.searchMode = parseSearchMode(
        extractString(
            content,
            "searchMode",
            searchModeName(parameters.searchMode)));
    parameters.lmrMinPly = extractInt(content, "lmrMinPly", parameters.lmrMinPly);
    parameters.lmrFullDepthMoves = extractInt(content, "lmrFullDepthMoves", parameters.lmrFullDepthMoves);
    parameters.pawnValue = extractInt(content, "pawn", parameters.pawnValue);
    parameters.knightValue = extractInt(content, "knight", parameters.knightValue);
    parameters.bishopValue = extractInt(content, "bishop", parameters.bishopValue);
    parameters.rookValue = extractInt(content, "rook", parameters.rookValue);
    parameters.queenValue = extractInt(content, "queen", parameters.queenValue);
    parameters.kingValue = extractInt(content, "king", parameters.kingValue);
    parameters.doubledPawnPenalty = extractInt(content, "doubledPawnPenalty", parameters.doubledPawnPenalty);
    parameters.isolatedPawnPenalty = extractInt(content, "isolatedPawnPenalty", parameters.isolatedPawnPenalty);
    parameters.passedPawnBonus = extractInt(content, "passedPawnBonus", parameters.passedPawnBonus);
    parameters.protectedPawnBonus = extractInt(content, "protectedPawnBonus", parameters.protectedPawnBonus);
    parameters.mobilityBonus = extractInt(content, "mobilityBonus", parameters.mobilityBonus);
    parameters.bishopPairBonus = extractInt(content, "bishopPairBonus", parameters.bishopPairBonus);
    parameters.kingShieldBonus = extractInt(content, "kingShieldBonus", parameters.kingShieldBonus);
    parameters.undevelopedMinorPenalty = extractInt(
        content,
        "undevelopedMinorPenalty",
        parameters.undevelopedMinorPenalty);

    clampParameters(parameters);
    return parameters;
}

void saveParametersToJson(const EvaluationParameters& parameters, const std::string& path) {
    EvaluationParameters copy = parameters;
    clampParameters(copy);

    // Créer le parent rend l’écriture utilisable directement pour un nouveau
    // répertoire d’exécution.
    std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("cannot save parameters to " + path);
    }

    output << "{\n";
    output << "  \"searchDepth\": " << copy.searchDepth << ",\n";
    output << "  \"searchMode\": \"" << searchModeName(copy.searchMode) << "\",\n";
    // Les paramètres LMR sont omis lorsqu’ils n’ont aucun effet, ce qui évite
    // de suggérer qu’ils modifient les modes classic ou instinct.
    if (copy.searchMode == SearchMode::InstinctLmr) {
        output << "  \"lmrMinPly\": " << copy.lmrMinPly << ",\n";
        output << "  \"lmrFullDepthMoves\": " << copy.lmrFullDepthMoves << ",\n";
    }
    output << "  \"pieceValues\": {\n";
    output << "    \"pawn\": " << copy.pawnValue << ",\n";
    output << "    \"knight\": " << copy.knightValue << ",\n";
    output << "    \"bishop\": " << copy.bishopValue << ",\n";
    output << "    \"rook\": " << copy.rookValue << ",\n";
    output << "    \"queen\": " << copy.queenValue << ",\n";
    output << "    \"king\": " << copy.kingValue << "\n";
    output << "  },\n";
    output << "  \"evaluationWeights\": {\n";
    output << "    \"doubledPawnPenalty\": " << copy.doubledPawnPenalty << ",\n";
    output << "    \"isolatedPawnPenalty\": " << copy.isolatedPawnPenalty << ",\n";
    output << "    \"passedPawnBonus\": " << copy.passedPawnBonus << ",\n";
    output << "    \"protectedPawnBonus\": " << copy.protectedPawnBonus << ",\n";
    output << "    \"mobilityBonus\": " << copy.mobilityBonus << ",\n";
    output << "    \"bishopPairBonus\": " << copy.bishopPairBonus << ",\n";
    output << "    \"kingShieldBonus\": " << copy.kingShieldBonus << ",\n";
    output << "    \"undevelopedMinorPenalty\": " << copy.undevelopedMinorPenalty << "\n";
    output << "  }\n";
    output << "}\n";
}

} // namespace ai
