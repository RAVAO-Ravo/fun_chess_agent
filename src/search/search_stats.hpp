/**
 * @file search_stats.hpp
 * @brief Décrire les mesures produites pendant une recherche.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace ai {

/**
 * @struct SearchStats
 * @brief Comptabiliser le travail et l’efficacité d’une recherche.
 */
struct SearchStats {
    std::uint64_t nodes = 0;
    std::uint64_t quiescenceNodes = 0;
    std::uint64_t transpositionProbes = 0;
    std::uint64_t transpositionHits = 0;
    std::uint64_t transpositionEntries = 0;
    std::uint64_t betaCutoffs = 0;
    int completedDepth = 0;
    std::chrono::microseconds elapsed{0};

    /** @brief Additionner les nœuds principaux et de quiescence. */
    std::uint64_t totalNodes() const;
    /** @brief Rapporter le total de nœuds au temps mesuré. */
    double nodesPerSecond() const;
    /** @brief Formater une ligne stable destinée au mode diagnostic. */
    std::string toProtocolLine() const;
};

} // namespace ai
