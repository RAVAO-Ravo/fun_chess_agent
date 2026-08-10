/**
 * @file move_generator.hpp
 * @brief Déclarer la génération et la validation des coups d’échecs.
 */

#pragma once

#include "chess/position.hpp"
#include "chess/move.hpp"
#include "chess/square.hpp"

#include <vector>

namespace chess {

enum class MoveGenerationMode {
    All,
    Tactical,
};

enum class MoveAnnotationMode {
    None,
    Ordering,
};

/**
 * @class MoveGenerator
 * @brief Produire des coups candidats puis éliminer ceux exposant le roi.
 *
 * Le mode tactique génère uniquement captures, promotions et réponses à
 * considérer en quiescence. Les surcharges mutables évitent une copie du
 * plateau dans les chemins critiques de la recherche.
 */
class MoveGenerator {
public:
    /** @brief Générer les déplacements respectant la géométrie des pièces. */
    static std::vector<Move> pseudoLegalMoves(
        const Position& board,
        MoveGenerationMode mode = MoveGenerationMode::All);
    /** @brief Filtrer les candidats en jouant et annulant chaque coup. */
    static std::vector<Move> legalMoves(
        Position& board,
        MoveGenerationMode mode = MoveGenerationMode::All,
        MoveAnnotationMode annotations = MoveAnnotationMode::None);
    /** @brief Générer les coups légaux depuis une copie sans historique. */
    static std::vector<Move> legalMoves(
        const Position& board,
        MoveGenerationMode mode = MoveGenerationMode::All,
        MoveAnnotationMode annotations = MoveAnnotationMode::None);
    /** @brief Arrêter la génération dès qu’un premier coup légal est trouvé. */
    static bool hasAnyLegalMove(Position& board);
    /** @brief Tester l’existence d’un coup légal sans modifier la position. */
    static bool hasAnyLegalMove(const Position& board);
    /** @brief Détecter une attaque sans générer l’ensemble des coups adverses. */
    static bool isSquareAttacked(const Position& board, Square square, Color byColor);

private:
    static void addMovesForPiece(
        const Position& board,
        Square from,
        MoveGenerationMode mode,
        std::vector<Move>& moves);
    static void addPawnMoves(
        const Position& board,
        Square from,
        MoveGenerationMode mode,
        std::vector<Move>& moves);
    static void addKnightMoves(
        const Position& board,
        Square from,
        MoveGenerationMode mode,
        std::vector<Move>& moves);
    static void addSlidingMoves(
        const Position& board,
        Square from,
        const int directions[][2],
        int directionCount,
        MoveGenerationMode mode,
        std::vector<Move>& moves);
    static void addKingMoves(
        const Position& board,
        Square from,
        MoveGenerationMode mode,
        std::vector<Move>& moves);
};

} // namespace chess
