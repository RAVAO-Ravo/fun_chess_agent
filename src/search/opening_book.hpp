/**
 * @file opening_book.hpp
 * @brief Déclarer une bibliothèque d’ouvertures indexée par position.
 */

#pragma once

#include "chess/position.hpp"
#include "chess/move.hpp"

#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace ai {

enum class OpeningBookMode {
    Chill,
    Competition
};

struct BookMove {
    chess::Move move;
    int weight = 1;
};

/**
 * @class OpeningBook
 * @brief Indexer des coups légaux par les quatre champs positionnels de FEN.
 *
 * Le poids d’un coup correspond au nombre de lignes où il apparaît. Le mode
 * détendu effectue un tirage pondéré, le mode compétition retient le maximum.
 */
class OpeningBook {
public:
    /** @brief Charger et valider toutes les lignes d’un fichier texte. */
    static OpeningBook loadFromFile(const std::string& path);
    /** @brief Construire une bibliothèque en mémoire, notamment pour les tests. */
    static OpeningBook fromLines(const std::vector<std::string>& lines);

    /** @brief Sélectionner un coup encore légal dans la position demandée. */
    std::optional<chess::Move> findMove(const chess::Position& board, OpeningBookMode mode, std::mt19937& rng) const;

private:
    void addLine(const std::string& line, int lineNumber);
    void addMove(const std::string& key, const chess::Move& move);

    static std::string positionKey(const chess::Position& board);

    std::unordered_map<std::string, std::vector<BookMove>> movesByPosition_;
};

/** @brief Décoder les alias acceptés d’une politique de bibliothèque. */
OpeningBookMode parseOpeningBookMode(const std::string& text);

} // namespace ai
