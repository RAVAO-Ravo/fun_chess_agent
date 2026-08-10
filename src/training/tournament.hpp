/**
 * @file tournament.hpp
 * @brief Déclarer les confrontations qui mesurent la force des individus.
 */

#pragma once

#include "chess/position.hpp"
#include "training/individual.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace training {

enum class GameResult {
    WhiteWin,
    BlackWin,
    Draw
};

/**
 * @struct MatchResult
 * @brief Résumer l’issue et la longueur d’une partie.
 */
struct MatchResult {
    GameResult result = GameResult::Draw;
    int halfMovesPlayed = 0;
    int finalEvaluation = 0;
};

/**
 * @class Tournament
 * @brief Mesurer deux individus dans des conditions de jeu contrôlées.
 *
 * Les positions de départ diversifient les confrontations. La limite de
 * demi-coups borne le coût et l’évaluation finale départage les parties
 * interrompues sans inventer un résultat tactique.
 */
class Tournament {
public:
    /** @brief Configurer la longueur, les ouvertures et la quiescence. */
    explicit Tournament(
        int maxHalfMoves = 300,
        std::vector<chess::Position> startingPositions = {},
        int quiescenceMaxPly = 10);

    /** @brief Jouer une partie déterministe entre deux individus. */
    MatchResult playGame(
        const Individual& white,
        const Individual& black,
        std::size_t startingPositionIndex = 0) const;

private:
    chess::Position startingBoard(std::size_t startingPositionIndex) const;

    int maxHalfMoves_;
    int quiescenceMaxPly_;
    std::vector<chess::Position> startingPositions_;
};

/** @brief Charger des positions FEN utilisables comme débuts de tournoi. */
std::vector<chess::Position> loadTrainingPositionsFromFenFile(const std::string& path);

} // namespace training
