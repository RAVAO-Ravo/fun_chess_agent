/**
 * @file move_ordering.cpp
 * @brief Classer les coups pour provoquer plus tôt les coupures alpha-bêta.
 */

#include "search/move_ordering.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <limits>

namespace ai {

namespace {

constexpr int PreferredMoveBonus = 10'000'000;
constexpr int PromotionBonus = 8'000'000;
constexpr int CaptureBonus = 6'000'000;
constexpr int CheckBonus = 4'000'000;
constexpr int KillerBonus = 2'000'000;

int instinctScore(
    const chess::Position& position,
    const chess::Move& move,
    const EvaluationParameters& parameters,
    bool destinationIsAttacked) {
    const chess::Piece piece = position.pieceAt(move.from());
    if (piece.isEmpty()) {
        return 0;
    }

    const chess::PieceType destinationType =
        move.isPromotion() ? move.promotion() : piece.type();
    int score = 0;

    const bool white = piece.color() == chess::Color::White;
    const int backRank = white ? 7 : 0;
    // Sortir une pièce mineure de sa rangée initiale traduit un développement
    // généralement sain sans dépendre d’une case précise mémorisée.
    if ((piece.type() == chess::PieceType::Knight || piece.type() == chess::PieceType::Bishop)
        && move.from().row() == backRank
        && move.to().row() != backRank) {
        score += parameters.undevelopedMinorPenalty;
    }

    if (destinationIsAttacked) {
        // Cette pénalité approximative décourage les pièces coûteuses laissées
        // en prise, sans prétendre remplacer une analyse complète des échanges.
        score -= pieceValue(parameters, destinationType) / 2;
    }
    // La formule entière atteint son maximum au centre et évite une table de
    // cases figée qui deviendrait difficile à entraîner et à maintenir.
    const int centralization =
        6 - std::abs(2 * move.to().row() - 7)
        - std::abs(2 * move.to().col() - 7);
    score += centralization * parameters.mobilityBonus;
    return score;
}

} // namespace

bool isQuietMove(const chess::Move& move) {
    return !move.isCapture() && !move.isPromotion();
}

void MoveOrdering::clear() {
    killers_ = {};
    history_ = {};
}

void MoveOrdering::order(
    const chess::Position& position,
    std::vector<chess::Move>& moves,
    const std::optional<chess::Move>& preferredMove,
    int ply,
    const EvaluationParameters& parameters,
    bool useInstinct) const {
    struct ScoredMove {
        chess::Move move;
        int score = 0;
    };

    std::vector<ScoredMove> scored;
    scored.reserve(moves.size());
    for (const chess::Move& move : moves) {
        assert(move.hasOrderingMetadata());
        const chess::Piece attacker = position.pieceAt(move.from());
        const chess::Piece victim = move.isEnPassant()
            ? chess::Piece(chess::PieceType::Pawn, chess::opposite(attacker.color()))
            : position.pieceAt(move.to());
        const bool givesCheck = move.givesCheck();
        const bool destinationIsAttacked = move.destinationIsAttacked();

        int score = historyScore(move, position.sideToMove());
        // Les bonus sont séparés par ordres de grandeur : une préférence issue
        // de la recherche doit dominer les ajustements positionnels modestes.
        if (preferredMove.has_value() && move.sameUci(*preferredMove)) {
            score += PreferredMoveBonus;
        }
        if (move.isPromotion()) {
            score += PromotionBonus + pieceValue(parameters, move.promotion());
        }
        if (move.isCapture()) {
            // Approximation MVV-LVA : gagner une victime chère est favorable,
            // mais la valeur de l’attaquant est retirée si la cible est défendue.
            int exchange = pieceValue(parameters, victim.type());
            if (destinationIsAttacked) {
                const chess::PieceType placedType =
                    move.isPromotion() ? move.promotion() : attacker.type();
                exchange -= pieceValue(parameters, placedType);
            }
            score += CaptureBonus + 32 * exchange;
        }
        if (givesCheck) {
            score += CheckBonus;
        }
        if (isQuietMove(move) && isKiller(move, ply)) {
            score += KillerBonus;
        }
        if (useInstinct) {
            score += instinctScore(
                position,
                move,
                parameters,
                destinationIsAttacked);
        }
        scored.push_back(ScoredMove{move, score});
    }

    // La stabilité conserve l’ordre déterministe de génération en cas d’égalité,
    // propriété utile pour reproduire entraînements et tests.
    std::stable_sort(scored.begin(), scored.end(), [](const ScoredMove& left, const ScoredMove& right) {
        return left.score > right.score;
    });
    for (std::size_t index = 0; index < scored.size(); ++index) {
        moves[index] = scored[index].move;
    }
}

void MoveOrdering::recordQuietCutoff(
    const chess::Move& move,
    chess::Color color,
    int ply,
    int depth) {
    if (!isQuietMove(move)) {
        return;
    }
    if (ply >= 0 && ply < MaxPly && !killers_[static_cast<std::size_t>(ply)][0].sameUci(move)) {
        // Deux killers par niveau offrent une mémoire locale suffisante sans
        // laisser cette heuristique supplanter toutes les autres.
        killers_[static_cast<std::size_t>(ply)][1] = killers_[static_cast<std::size_t>(ply)][0];
        killers_[static_cast<std::size_t>(ply)][0] = move;
    }
    if (color == chess::Color::None || !move.from().isValid() || !move.to().isValid()) {
        return;
    }
    const std::size_t colorIndex = color == chess::Color::White ? 0u : 1u;
    int& value = history_[colorIndex]
                         [static_cast<std::size_t>(move.from().index())]
                         [static_cast<std::size_t>(move.to().index())];
    const int bonus = std::max(1, depth * depth);
    // Le carré de la profondeur récompense davantage une coupure obtenue haut
    // dans un sous-arbre coûteux. Le plafond prévient tout débordement progressif.
    value = std::min(value + bonus, 1'000'000);
}

bool MoveOrdering::isKiller(const chess::Move& move, int ply) const {
    if (ply < 0 || ply >= MaxPly) {
        return false;
    }
    const auto& killers = killers_[static_cast<std::size_t>(ply)];
    return killers[0].sameUci(move) || killers[1].sameUci(move);
}

int MoveOrdering::historyScore(const chess::Move& move, chess::Color color) const {
    if (color == chess::Color::None || !move.from().isValid() || !move.to().isValid()) {
        return 0;
    }
    const std::size_t colorIndex = color == chess::Color::White ? 0u : 1u;
    return history_[colorIndex]
                   [static_cast<std::size_t>(move.from().index())]
                   [static_cast<std::size_t>(move.to().index())];
}

} // namespace ai
