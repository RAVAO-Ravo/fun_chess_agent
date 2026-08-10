/**
 * @file game_session.hpp
 * @brief Regrouper une partie interactive, son moteur et ses limites.
 */

#pragma once

#include "search/chess_ai.hpp"
#include "chess/position.hpp"
#include "chess/color.hpp"
#include "chess/move.hpp"

#include <optional>
#include <string>

namespace app {

/**
 * @class GameSession
 * @brief Maintenir la position et le rôle du joueur entre les commandes.
 *
 * La session délègue les règles à Position et les décisions à ChessAI. Elle
 * définit uniquement les actions cohérentes pour une partie humain contre IA.
 */
class GameSession {
public:
    /** @brief Construire une session autour d’un moteur déjà configuré. */
    explicit GameSession(ai::ChessAI ai, chess::Color humanColor = chess::Color::White);

    /** @brief Réinitialiser le plateau et définir le camp humain. */
    void newGame(chess::Color humanColor);
    /** @brief Jouer un coup uniquement lorsque le trait appartient à l’humain. */
    bool playHumanMove(const chess::Move& move);
    /** @brief Calculer et appliquer le coup du moteur si la partie continue. */
    std::optional<chess::Move> playAIMove();
    /** @brief Annuler jusqu’à rendre le trait au joueur humain. */
    bool undoTurn();
    /** @brief Remplacer les limites appliquées aux prochains calculs. */
    void setSearchLimits(ai::SearchLimits limits);

    chess::Position& board();

    chess::Color humanColor() const;
    bool isGameOver() const;
    /** @brief Produire l’état textuel stable attendu par le protocole. */
    std::string gameStatus() const;
    const ai::SearchStats& lastSearchStats() const;

private:
    chess::Position board_;
    ai::ChessAI ai_;
    chess::Color humanColor_;
    std::optional<ai::SearchLimits> searchLimits_;
};

} // namespace app
