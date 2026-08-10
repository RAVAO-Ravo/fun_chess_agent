/**
 * @file game_session.cpp
 * @brief Appliquer les actions d’une session et exposer son état au protocole.
 */

#include "protocol/game_session.hpp"

#include "protocol/protocol.hpp"

#include <utility>

namespace app {

GameSession::GameSession(ai::ChessAI ai, chess::Color humanColor)
    : ai_(std::move(ai))
    , humanColor_(humanColor) {
}

void GameSession::newGame(chess::Color humanColor) {
    board_.reset();
    humanColor_ = humanColor;
}

bool GameSession::playHumanMove(const chess::Move& move) {
    if (isGameOver() || board_.sideToMove() != humanColor_) {
        return false;
    }
    if (std::optional<chess::Move> legalMove = board_.findLegalMove(move)) {
        return board_.makeMove(*legalMove);
    }
    return false;
}

std::optional<chess::Move> GameSession::playAIMove() {
    if (isGameOver() || board_.sideToMove() == humanColor_) {
        return std::nullopt;
    }
    chess::Move move = searchLimits_.has_value()
        ? ai_.chooseMove(board_, *searchLimits_)
        : ai_.chooseMove(board_);
    if (!move.from().isValid()) {
        return std::nullopt;
    }
    board_.makeMove(move);
    return move;
}

bool GameSession::undoTurn() {
    if (!board_.canUndo()) {
        return false;
    }

    if (board_.sideToMove() == humanColor_ && board_.historySize() < 2) {
        return false;
    }

    board_.undoMove();
    if (board_.canUndo() && board_.sideToMove() != humanColor_) {
        board_.undoMove();
    }
    return true;
}

void GameSession::setSearchLimits(ai::SearchLimits limits) {
    searchLimits_ = limits;
}

chess::Position& GameSession::board() {
    return board_;
}

chess::Color GameSession::humanColor() const {
    return humanColor_;
}

bool GameSession::isGameOver() const {
    return board_.termination() != chess::GameTermination::Ongoing;
}

std::string GameSession::gameStatus() const {
    return protocolStatus(board_);
}

const ai::SearchStats& GameSession::lastSearchStats() const {
    return ai_.lastSearchStats();
}

} // namespace app
