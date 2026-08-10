/**
 * @file chess_ai.cpp
 * @brief Coordonner bibliothèque d’ouvertures et recherche calculée.
 */

#include "search/chess_ai.hpp"

#include "search/evaluation/evaluator.hpp"
#include "search/searcher.hpp"

#include <algorithm>
#include <utility>

namespace ai {

ChessAI::ChessAI(EvaluationParameters parameters)
    : parameters_(std::move(parameters))
    , rng_(std::random_device{}()) {
    clampParameters(parameters_);
}

ChessAI::ChessAI(EvaluationParameters parameters, OpeningBook openingBook, OpeningBookMode openingBookMode)
    : parameters_(std::move(parameters))
    , openingBook_(std::move(openingBook))
    , openingBookMode_(openingBookMode)
    , rng_(std::random_device{}()) {
    clampParameters(parameters_);
}

chess::Move ChessAI::chooseMove(const chess::Position& board) {
    SearchLimits limits;
    limits.maxDepth = parameters_.searchDepth;
    return chooseMove(board, limits);
}

chess::Move ChessAI::chooseMove(
    const chess::Position& board,
    SearchLimits limits) {
    if (std::optional<chess::Move> bookMove = openingBook_.findMove(board, openingBookMode_, rng_)) {
        lastSearchStats_ = {};
        return *bookMove;
    }

    limits.maxDepth = std::max(1, limits.maxDepth);
    return analyze(board, limits).bestMove;
}

SearchResult ChessAI::analyze(const chess::Position& board, SearchLimits limits) {
    EvaluationParameters copy = parameters_;
    clampParameters(copy);
    Searcher searcher{copy};
    SearchResult result = searcher.search(board, limits);
    lastSearchStats_ = result.stats;
    return result;
}

const SearchStats& ChessAI::lastSearchStats() const {
    return lastSearchStats_;
}

} // namespace ai
