/**
 * @file search_limits.hpp
 * @brief Regrouper les limites et optimisations activables d’une recherche.
 */

#pragma once

#include <chrono>

namespace ai {

/**
 * @struct SearchLimits
 * @brief Définir le budget et les optimisations d’un appel de recherche.
 *
 * Une durée nulle signifie qu’aucune échéance temporelle ne s’applique. La
 * quiescence possède sa propre borne pour maîtriser les suites tactiques.
 */
struct SearchLimits {
    int maxDepth = 1;
    std::chrono::milliseconds timeLimit{0};
    int quiescenceMaxPly = 10;
    bool usePrincipalVariationSearch = true;
    bool useAspirationWindows = true;
};

} // namespace ai
