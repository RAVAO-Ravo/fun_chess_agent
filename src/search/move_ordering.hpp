/**
 * @file move_ordering.hpp
 * @brief Déclarer les heuristiques de classement des coups.
 */

#pragma once

#include "chess/move.hpp"
#include "chess/position.hpp"
#include "search/evaluation/parameters.hpp"

#include <array>
#include <optional>
#include <vector>

namespace ai {

/**
 * @class MoveOrdering
 * @brief Accumuler les heuristiques qui classent les coups prometteurs.
 *
 * Les coups de la table de transposition, captures, promotions et échecs sont
 * prioritaires. Les coups calmes profitent des heuristiques killer et history,
 * apprises au fil des coupures de la recherche courante.
 */
class MoveOrdering {
public:
    static constexpr int MaxPly = 128;

    /** @brief Réinitialiser les heuristiques propres à une recherche. */
    void clear();
    /** @brief Trier en place les coups du plus au moins prometteur. */
    void order(
        const chess::Position& position,
        std::vector<chess::Move>& moves,
        const std::optional<chess::Move>& preferredMove,
        int ply,
        const EvaluationParameters& parameters,
        bool useInstinct) const;
    /** @brief Renforcer un coup calme ayant provoqué une coupure bêta. */
    void recordQuietCutoff(const chess::Move& move, chess::Color color, int ply, int depth);

private:
    bool isKiller(const chess::Move& move, int ply) const;
    int historyScore(const chess::Move& move, chess::Color color) const;

    std::array<std::array<chess::Move, 2>, MaxPly> killers_{};
    std::array<std::array<std::array<int, 64>, 64>, 2> history_{};
};

/** @brief Reconnaître un coup sans capture, promotion ni échec. */
bool isQuietMove(const chess::Move& move);

} // namespace ai
