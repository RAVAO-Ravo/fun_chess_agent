/**
 * @file cli_options.hpp
 * @brief Analyser les options simples des exécutables en ligne de commande.
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace util {

/**
 * @class CliOptions
 * @brief Lire des drapeaux et valeurs sans imposer une bibliothèque de CLI.
 *
 * Le parseur convient aux deux exécutables du projet et rejette explicitement
 * les valeurs manquantes ou les entiers partiellement valides.
 */
class CliOptions {
public:
    /** @brief Copier les arguments utilisateur en ignorant le nom du programme. */
    CliOptions(int argc, char** argv) {
        arguments_.reserve(static_cast<std::size_t>(argc > 1 ? argc - 1 : 0));
        for (int index = 1; index < argc; ++index) {
            arguments_.emplace_back(argv[index]);
        }
    }

    /** @brief Indiquer si une option exacte est présente. */
    bool has(const std::string& name) const {
        for (const std::string& argument : arguments_) {
            if (argument == name) {
                return true;
            }
        }
        return false;
    }

    /** @brief Lire la valeur suivant une option ou retourner le repli. */
    std::string value(
        const std::string& name,
        const std::string& fallback = "") const {
        for (std::size_t index = 0; index < arguments_.size(); ++index) {
            if (arguments_[index] != name) {
                continue;
            }
            if (index + 1 >= arguments_.size()
                || arguments_[index + 1].starts_with("--")) {
                throw std::invalid_argument(name + " requires a value");
            }
            return arguments_[index + 1];
        }
        return fallback;
    }

    /** @brief Convertir intégralement une valeur en entier. */
    int integer(const std::string& name, int fallback) const {
        if (!has(name)) {
            return fallback;
        }
        const std::string text = value(name);
        std::size_t consumed = 0;
        const int result = std::stoi(text, &consumed);
        if (consumed != text.size()) {
            throw std::invalid_argument(name + " requires an integer");
        }
        return result;
    }

private:
    std::vector<std::string> arguments_;
};

} // namespace util
