/**
 * @file transposition_table.hpp
 * @brief Déclarer le cache borné des positions déjà explorées.
 */

#pragma once

#include "chess/move.hpp"

#include <cstdint>
#include <cstddef>
#include <optional>
#include <vector>

namespace ai {

enum class BoundType {
    Exact,
    Lower,
    Upper
};

/**
 * @struct TranspositionEntry
 * @brief Décrire un résultat de recherche réutilisable pour une position.
 *
 * La borne précise si le score est exact ou seulement inférieur/supérieur à
 * la fenêtre qui a provoqué la coupure.
 */
struct TranspositionEntry {
    int depth = 0;
    int score = 0;
    BoundType bound = BoundType::Exact;
    chess::Move bestMove;
    std::uint32_t generation = 0;
};

/**
 * @class TranspositionTable
 * @brief Mémoriser un nombre borné de positions par adressage direct.
 *
 * Les collisions remplacent prioritairement les entrées anciennes ou moins
 * profondes. Cette politique évite les allocations pendant la recherche.
 */
class TranspositionTable {
public:
    /** @brief Allouer le nombre maximal d’emplacements demandé. */
    explicit TranspositionTable(std::size_t maxEntries = 1'000'000);

    /** @brief Vider toutes les entrées et remettre la génération à zéro. */
    void clear();
    /** @brief Avancer l’âge logique avant une nouvelle recherche racine. */
    void newSearch();
    /** @brief Retourner le nombre d’emplacements actuellement occupés. */
    std::size_t size() const;
    /** @brief Insérer une entrée selon la politique de remplacement. */
    void store(std::uint64_t key, TranspositionEntry entry);
    /** @brief Retrouver une entrée uniquement si sa clé complète correspond. */
    std::optional<TranspositionEntry> lookup(std::uint64_t key) const;

private:
    struct Slot {
        std::uint64_t key = 0;
        TranspositionEntry entry;
        bool occupied = false;
    };

    std::size_t maxEntries_ = 1'000'000;
    std::size_t size_ = 0;
    std::uint32_t generation_ = 0;
    std::vector<Slot> entries_;
};

} // namespace ai
