/**
 * @file search_stats.cpp
 * @brief Calculer et formater les indicateurs de performance du moteur.
 */

#include "search/search_stats.hpp"

#include <iomanip>
#include <sstream>

namespace ai {

std::uint64_t SearchStats::totalNodes() const {
    return nodes + quiescenceNodes;
}

double SearchStats::nodesPerSecond() const {
    const double seconds = static_cast<double>(elapsed.count()) / 1'000'000.0;
    return seconds > 0.0 ? static_cast<double>(totalNodes()) / seconds : 0.0;
}

std::string SearchStats::toProtocolLine() const {
    std::ostringstream output;
    output << "search_stats"
           << " depth " << completedDepth
           << " nodes " << nodes
           << " qnodes " << quiescenceNodes
           << " time_us " << elapsed.count()
           << " nps " << std::fixed << std::setprecision(0) << nodesPerSecond()
           << " tt_probes " << transpositionProbes
           << " tt_hits " << transpositionHits
           << " tt_entries " << transpositionEntries
           << " cutoffs " << betaCutoffs;
    return output.str();
}

} // namespace ai
