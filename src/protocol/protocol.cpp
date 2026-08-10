/**
 * @file protocol.cpp
 * @brief Traduire les couleurs et états de partie en messages du protocole.
 */

#include "protocol/protocol.hpp"

#include <stdexcept>

namespace app {

chess::Color parseColor(const std::string& text) {
    if (text == "white" || text == "w") {
        return chess::Color::White;
    }
    if (text == "black" || text == "b") {
        return chess::Color::Black;
    }
    throw std::invalid_argument("invalid color: " + text);
}

std::string protocolStatus(const chess::Position& board) {
    const chess::GameTermination termination = board.termination();
    if (termination == chess::GameTermination::Checkmate) {
        chess::Color winner = chess::opposite(board.sideToMove());
        return std::string("status checkmate ") + chess::colorName(winner);
    }
    if (termination == chess::GameTermination::Stalemate) {
        return "status stalemate";
    }
    if (termination == chess::GameTermination::RuleDraw) {
        return "status draw";
    }
    if (board.isInCheck(chess::Color::White)) {
        return "status check white";
    }
    if (board.isInCheck(chess::Color::Black)) {
        return "status check black";
    }
    return "status playing";
}

} // namespace app
