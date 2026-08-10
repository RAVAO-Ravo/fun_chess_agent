/**
 * @file transposition_table.cpp
 * @brief Stocker les évaluations réutilisables entre branches transposées.
 */

#include "search/transposition_table.hpp"

#include <algorithm>

namespace ai {

TranspositionTable::TranspositionTable(std::size_t maxEntries)
    : maxEntries_(std::max<std::size_t>(1, maxEntries))
    , entries_(maxEntries_) {
}

void TranspositionTable::clear() {
    entries_.assign(maxEntries_, Slot{});
    size_ = 0;
    generation_ = 0;
}

void TranspositionTable::newSearch() {
    ++generation_;
    if (generation_ == 0) {
        // Après débordement du compteur, aucune ancienne entrée ne doit paraître
        // appartenir par erreur à la nouvelle génération.
        clear();
        generation_ = 1;
    }
}

std::size_t TranspositionTable::size() const {
    return size_;
}

void TranspositionTable::store(std::uint64_t key, TranspositionEntry entry) {
    // L’adressage direct garantit un accès constant et aucune allocation. La
    // clé complète distingue une vraie correspondance d’une simple collision.
    Slot& slot = entries_[static_cast<std::size_t>(key % maxEntries_)];
    const bool replace =
        !slot.occupied
        || slot.key == key
        || slot.entry.generation != generation_
        || entry.depth >= slot.entry.depth;
    // Une entrée courante plus profonde contient une information plus coûteuse
    // et plus fiable ; elle est donc protégée d’un remplacement moins profond.
    if (!replace) {
        return;
    }
    if (!slot.occupied) {
        ++size_;
    }
    entry.generation = generation_;
    slot = Slot{key, entry, true};
}

std::optional<TranspositionEntry> TranspositionTable::lookup(std::uint64_t key) const {
    const Slot& slot = entries_[static_cast<std::size_t>(key % maxEntries_)];
    if (!slot.occupied || slot.key != key) {
        return std::nullopt;
    }
    return slot.entry;
}

} // namespace ai
