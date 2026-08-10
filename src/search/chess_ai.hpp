/**
 * @file chess_ai.hpp
 * @brief Exposer la façade de haut niveau du moteur de recherche.
 */

#pragma once

#include "search/evaluation/parameters.hpp"
#include "search/opening_book.hpp"
#include "search/searcher.hpp"
#include "chess/position.hpp"
#include "chess/move.hpp"

#include <random>

namespace ai {

/**
 * @class ChessAI
 * @brief Choisir entre un coup d’ouverture connu et une recherche calculée.
 *
 * Le générateur propre à l’instance permet les choix pondérés du mode détendu.
 * Les statistiques exposées correspondent toujours au dernier calcul effectif.
 */
class ChessAI {
public:
    /** @brief Construire un moteur sans bibliothèque d’ouvertures. */
    explicit ChessAI(EvaluationParameters parameters = defaultEvaluationParameters());
    /** @brief Construire un moteur avec une bibliothèque et sa politique. */
    ChessAI(
        EvaluationParameters parameters,
        OpeningBook openingBook,
        OpeningBookMode openingBookMode = OpeningBookMode::Chill);

    /** @brief Choisir un coup avec la profondeur portée par les paramètres. */
    chess::Move chooseMove(const chess::Position& board);
    /** @brief Choisir un coup avec des limites spécifiques à l’appel. */
    chess::Move chooseMove(const chess::Position& board, SearchLimits limits);
    /** @brief Retourner le résultat complet sans masquer son score. */
    SearchResult analyze(const chess::Position& board, SearchLimits limits);
    /** @brief Lire les mesures du dernier appel de recherche. */
    const SearchStats& lastSearchStats() const;

private:
    EvaluationParameters parameters_;
    OpeningBook openingBook_;
    OpeningBookMode openingBookMode_ = OpeningBookMode::Chill;
    std::mt19937 rng_;
    SearchStats lastSearchStats_;
};

} // namespace ai
