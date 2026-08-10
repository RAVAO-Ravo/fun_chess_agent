/**
 * @file termination.hpp
 * @brief Énumérer les états terminaux reconnus par le moteur.
 */

#pragma once

namespace chess {

enum class GameTermination {
    Ongoing,
    Checkmate,
    Stalemate,
    RuleDraw,
};

} // namespace chess
