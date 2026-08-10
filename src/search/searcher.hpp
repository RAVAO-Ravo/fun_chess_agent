/**
 * @file searcher.hpp
 * @brief Déclarer la recherche itérative negamax avec élagage alpha-bêta.
 */

#pragma once

#include "chess/move.hpp"
#include "chess/position.hpp"
#include "search/evaluation/evaluator.hpp"
#include "search/move_ordering.hpp"
#include "search/search_limits.hpp"
#include "search/search_stats.hpp"
#include "search/transposition_table.hpp"

#include <chrono>
#include <utility>
#include <vector>

namespace ai {

/**
 * @struct SearchResult
 * @brief Regrouper le meilleur coup et les mesures d’une recherche terminée.
 */
struct SearchResult {
    chess::Move bestMove;
    int score = 0;
    SearchStats stats;
    bool stoppedByTime = false;
};

/**
 * @class Searcher
 * @brief Explorer une position par approfondissement itératif.
 *
 * Cette classe porte tout l’état temporaire d’une recherche : bornes de temps,
 * statistiques, table de transposition et heuristiques d’ordre des coups.
 * Une instance peut servir à plusieurs appels successifs, mais pas en parallèle.
 */
class Searcher {
public:
    /** @brief Construire une recherche avec des paramètres bornés. */
    explicit Searcher(EvaluationParameters parameters);

    /**
     * @brief Rechercher le meilleur coup dans les limites demandées.
     *
     * @param position (const chess::Position&) : Position racine non modifiée.
     * @param limits (SearchLimits) : Profondeur, temps et optimisations actives.
     *
     * @return SearchResult : Dernière itération entièrement achevée.
     */
    SearchResult search(const chess::Position& position, SearchLimits limits);

private:
    std::pair<chess::Move, int> searchRoot(
        chess::Position& position,
        int depth,
        const chess::Move& previousBest,
        int alpha,
        int beta);
    /** @brief Évaluer récursivement une position selon l’invariant negamax. */
    int negamax(chess::Position& position, int depth, int alpha, int beta, int ply);
    /** @brief Prolonger les échanges tactiques au-delà de la profondeur normale. */
    int quiescence(
        chess::Position& position,
        int alpha,
        int beta,
        int ply,
        int quiescencePly);
    /** @brief Orienter l’évaluation statique vers le camp au trait. */
    int evaluateForSideToMove(const chess::Position& position) const;
    /** @brief Mémoriser et signaler l’expiration de la limite temporelle. */
    bool shouldStop();
    bool usesInstinctOrdering() const;
    bool usesLateMoveReductions() const;

    EvaluationParameters parameters_;
    Evaluator evaluator_;
    TranspositionTable transpositionTable_;
    MoveOrdering moveOrdering_;
    SearchLimits limits_;
    SearchStats stats_;
    std::chrono::steady_clock::time_point startedAt_;
    std::chrono::steady_clock::time_point deadline_;
    bool stopped_ = false;
};

} // namespace ai
