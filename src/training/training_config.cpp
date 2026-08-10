/**
 * @file training_config.cpp
 * @brief Valider la configuration JSON et résoudre ses chemins relatifs.
 */

#include "training/training_config.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace training {

namespace {

std::string readFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot load genetic config from " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

int extractInt(const std::string& content, const std::string& key, int fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (std::regex_search(content, match, pattern)) {
        return std::stoi(match[1].str());
    }
    return fallback;
}

unsigned int extractUInt(const std::string& content, const std::string& key, unsigned int fallback) {
    int value = extractInt(content, key, static_cast<int>(fallback));
    if (value < 0) {
        throw std::invalid_argument(key + " must be non-negative");
    }
    return static_cast<unsigned int>(value);
}

std::size_t extractSize(const std::string& content, const std::string& key, std::size_t fallback) {
    int value = extractInt(content, key, static_cast<int>(fallback));
    if (value <= 0) {
        throw std::invalid_argument(key + " must be positive");
    }
    return static_cast<std::size_t>(value);
}

double extractDouble(const std::string& content, const std::string& key, double fallback) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch match;
    if (std::regex_search(content, match, pattern)) {
        return std::stod(match[1].str());
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

std::vector<std::string> extractScheduleObjects(const std::string& content) {
    std::vector<std::string> objects;
    std::size_t key = content.find("\"generation_schedule\"");
    if (key == std::string::npos) {
        return objects;
    }
    std::size_t arrayOpen = content.find('[', key);
    if (arrayOpen == std::string::npos) {
        throw std::invalid_argument("generation_schedule must be an array");
    }
    int depth = 0;
    std::size_t objectStart = std::string::npos;
    // Une profondeur d’accolades suffit ici car le calendrier ne contient que
    // des objets numériques plats. Elle évite une dépendance JSON supplémentaire.
    for (std::size_t i = arrayOpen + 1; i < content.size(); ++i) {
        if (content[i] == '{') {
            if (depth == 0) {
                objectStart = i;
            }
            ++depth;
        } else if (content[i] == '}') {
            --depth;
            if (depth == 0 && objectStart != std::string::npos) {
                objects.push_back(content.substr(objectStart, i - objectStart + 1));
                objectStart = std::string::npos;
            }
        } else if (content[i] == ']' && depth == 0) {
            break;
        }
    }
    return objects;
}

std::string resolveRelativePath(const std::string& configPath, const std::string& path) {
    std::filesystem::path resolved(path);
    if (resolved.is_relative()) {
        resolved = std::filesystem::path(configPath).parent_path() / resolved;
    }
    return resolved.string();
}

void validateScheduleEntry(const GenerationSettings& settings) {
    if (settings.from < 1 || settings.to < settings.from) {
        throw std::invalid_argument("invalid generation schedule range");
    }
    if (settings.populationSize == 0 || settings.populationSize % 2 != 0) {
        throw std::invalid_argument("scheduled population_size must be even");
    }
}

} // namespace

GeneticOptions loadGeneticOptionsFromJson(const std::string& path) {
    const std::string content = readFile(path);
    GeneticOptions options;

    // === Paramètres généraux ===

    // Certains alias camelCase sont encore lus pour préserver la compatibilité
    // des configurations existantes ; les nouveaux fichiers utilisent snake_case.
    options.populationSize = extractSize(content, "population_size", options.populationSize);
    options.generations = extractInt(content, "generations", options.generations);
    options.seed = extractUInt(content, "random_state", options.seed);
    options.seed = extractUInt(content, "seed", options.seed);
    options.maxHalfMoves = extractInt(content, "max_halfmoves", options.maxHalfMoves);
    options.maxHalfMoves = extractInt(content, "maxHalfMoves", options.maxHalfMoves);
    options.quiescenceMaxPly = extractInt(
        content,
        "quiescence_max_ply",
        options.quiescenceMaxPly);
    options.threads = extractInt(content, "n_jobs", options.threads);
    options.threads = extractInt(content, "threads", options.threads);
    options.outputPath = extractString(content, "output_path", options.outputPath);
    options.outputPath = extractString(content, "outputPath", options.outputPath);
    options.logDir = extractString(content, "log_dir", options.logDir);
    options.logDir = extractString(content, "logDir", options.logDir);
    options.runRoot = extractString(content, "run_root", options.runRoot);
    options.runRoot = extractString(content, "runRoot", options.runRoot);
    options.searchSpacePath = extractString(content, "search_space_path", options.searchSpacePath);
    options.searchSpacePath = extractString(content, "searchSpacePath", options.searchSpacePath);
    options.trainingPositionsPath = extractString(content, "training_positions_path", options.trainingPositionsPath);
    options.trainingPositionsPath = extractString(content, "trainingPositionsPath", options.trainingPositionsPath);
    options.fitnessMode = extractString(content, "fitness_mode", options.fitnessMode);
    options.corpusPath = extractString(content, "corpus_path", options.corpusPath);

    // === Calendrier des générations ===

    for (const std::string& object : extractScheduleObjects(content)) {
        GenerationSettings settings;
        settings.from = extractInt(object, "from", settings.from);
        settings.to = extractInt(object, "to", settings.to);
        settings.populationSize =
            extractSize(object, "population_size", options.populationSize);
        settings.mutationIndividualFraction = extractDouble(
            object,
            "mutation_individual_fraction",
            settings.mutationIndividualFraction);
        settings.mutationRateScalar = extractDouble(
            object,
            "mutation_rate_scalar",
            settings.mutationRateScalar);
        settings.mutationRateScalar = extractDouble(
            object,
            "gene_mutation_rate",
            settings.mutationRateScalar);
        settings.mutationRateDepth = extractDouble(
            object,
            "mutation_rate_depth",
            settings.mutationRateDepth);
        settings.mutationScaleFraction = extractDouble(
            object,
            "mutation_scale_fraction",
            settings.mutationScaleFraction);
        validateScheduleEntry(settings);
        options.generationSchedule.push_back(settings);
    }

    // === Validation croisée ===

    if (options.generations < 0) {
        throw std::invalid_argument("generations must be non-negative");
    }
    if (options.threads < 1) {
        throw std::invalid_argument("threads must be positive");
    }
    if (options.quiescenceMaxPly < 1) {
        throw std::invalid_argument("quiescence_max_ply must be positive");
    }
    if (options.populationSize == 0 || options.populationSize % 2 != 0) {
        throw std::invalid_argument("population_size must be even");
    }
    if (!options.generationSchedule.empty()) {
        options.populationSize = options.generationSchedule.front().populationSize;
    }
    // Tous les chemins relatifs sont interprétés depuis le fichier de
    // configuration, pas depuis le répertoire courant de l’exécutable.
    if (!options.searchSpacePath.empty()) {
        options.searchSpacePath = resolveRelativePath(path, options.searchSpacePath);
        options.searchSpace = loadSearchSpaceFromJson(options.searchSpacePath);
    }
    if (!options.trainingPositionsPath.empty()) {
        options.trainingPositionsPath = resolveRelativePath(path, options.trainingPositionsPath);
    }
    if (!options.corpusPath.empty()) {
        options.corpusPath = resolveRelativePath(path, options.corpusPath);
    }
    if (options.fitnessMode != "matches" && options.fitnessMode != "corpus") {
        throw std::invalid_argument("fitness_mode must be matches or corpus");
    }
    return options;
}

} // namespace training
