/**
 * @file corpus.cpp
 * @brief Charger et valider un corpus tabulé de positions évaluées.
 */

#include "training/corpus.hpp"

#include "search/evaluation/evaluator.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace training {

std::vector<CorpusSample> loadCorpus(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot load evaluation corpus from " + path);
    }

    std::vector<CorpusSample> corpus;
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (line.empty() || line.starts_with('#')) {
            continue;
        }
        const std::size_t separator = line.find('\t');
        if (separator == std::string::npos) {
            throw std::runtime_error(
                "invalid corpus row " + std::to_string(lineNumber)
                + " in " + path);
        }
        try {
            const int target = std::stoi(line.substr(0, separator));
            corpus.push_back(CorpusSample{
                chess::Position::fromFen(line.substr(separator + 1)),
                target,
            });
        } catch (const std::exception& error) {
            throw std::runtime_error(
                "invalid corpus row " + std::to_string(lineNumber)
                + " in " + path + ": " + error.what());
        }
    }
    if (corpus.empty()) {
        throw std::runtime_error("evaluation corpus is empty: " + path);
    }
    return corpus;
}

double corpusFitness(
    const ai::EvaluationParameters& parameters,
    const std::vector<CorpusSample>& corpus) {
    if (corpus.empty()) {
        return 1.0;
    }
    const ai::Evaluator evaluator(parameters);
    double absoluteError = 0.0;
    for (const CorpusSample& sample : corpus) {
        absoluteError += std::abs(
            static_cast<double>(evaluator.evaluate(sample.position) - sample.targetScore));
    }
    const double meanError = absoluteError / static_cast<double>(corpus.size());
    return 1.0 + 2.0 / (1.0 + meanError / 100.0);
}

} // namespace training
