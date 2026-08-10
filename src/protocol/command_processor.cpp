/**
 * @file command_processor.cpp
 * @brief Valider les commandes externes et produire des réponses structurées.
 */

#include "protocol/command_processor.hpp"

#include "protocol/protocol.hpp"

#include <exception>
#include <sstream>

namespace app {

CommandProcessor::CommandProcessor(
    GameSession& session,
    std::ostream& output,
    bool diagnostics)
    : session_(session)
    , output_(output)
    , diagnostics_(diagnostics) {
}

bool CommandProcessor::processLine(const std::string& line) {
    std::istringstream input(line);
    std::string command;
    input >> command;
    if (command.empty()) {
        return true;
    }

    // Une exception de validation devient une ligne d’erreur ; elle ne doit pas
    // interrompre la boucle interactive ni désynchroniser le client.
    try {
        if (command == "quit") {
            output_ << "ok\n";
            return false;
        }
        if (command == "new_game") {
            std::string colorText;
            input >> colorText;
            session_.newGame(parseColor(colorText.empty() ? "white" : colorText));
            // Si l’humain choisit les Noirs, la même commande inclut le premier
            // coup de l’IA afin de retourner une position immédiatement jouable.
            if (session_.board().sideToMove() != session_.humanColor()) {
                const std::optional<chess::Move> aiMove = session_.playAIMove();
                if (aiMove.has_value()) {
                    output_ << "ai_move " << aiMove->toUci() << '\n';
                    printSearchStats();
                }
            }
            printPosition();
            return true;
        }
        if (command == "human_move") {
            std::string moveText;
            input >> moveText;
            if (!session_.playHumanMove(chess::Move::fromUci(moveText))) {
                output_ << "illegal_move\n";
                return true;
            }
            output_ << "ok\n";
            printPosition();
            return true;
        }
        if (command == "free_move") {
            std::string moveText;
            input >> moveText;
            if (!playFreeMove(moveText)) {
                output_ << "illegal_move\n";
                return true;
            }
            output_ << "ok\n";
            printPosition();
            return true;
        }
        if (command == "ai_move") {
            if (!session_.isGameOver()) {
                const std::optional<chess::Move> aiMove = session_.playAIMove();
                if (aiMove.has_value()) {
                    output_ << "ai_move " << aiMove->toUci() << '\n';
                    printSearchStats();
                }
            }
            printPosition();
            return true;
        }
        if (command == "undo_turn") {
            if (!session_.undoTurn()) {
                output_ << "cannot_undo\n";
                return true;
            }
            output_ << "ok\n";
            printPosition();
            return true;
        }
        if (command == "undo_move") {
            if (!session_.board().canUndo()) {
                output_ << "cannot_undo\n";
                return true;
            }
            session_.board().undoMove();
            output_ << "ok\n";
            printPosition();
            return true;
        }
        if (command == "get_fen") {
            output_ << "fen " << session_.board().toFen() << '\n';
            return true;
        }
        if (command == "get_legal_moves") {
            printLegalMoves();
            return true;
        }
        if (command == "status") {
            output_ << session_.gameStatus() << '\n';
            return true;
        }
        if (command == "diagnostics") {
            std::string setting;
            input >> setting;
            if (setting == "on") {
                diagnostics_ = true;
                output_ << "ok\n";
            } else if (setting == "off") {
                diagnostics_ = false;
                output_ << "ok\n";
            } else {
                output_ << session_.lastSearchStats().toProtocolLine() << '\n';
            }
            return true;
        }
        output_ << "error unknown command: " << command << '\n';
    } catch (const std::exception& error) {
        output_ << "error " << error.what() << '\n';
    }
    return true;
}

void CommandProcessor::printPosition() const {
    // L’ordre FEN puis status est le contrat attendu par EngineClient.
    output_ << "board_fen " << session_.board().toFen() << '\n';
    output_ << session_.gameStatus() << '\n';
}

void CommandProcessor::printLegalMoves() const {
    output_ << "legal_moves";
    for (const chess::Move& move : session_.board().legalMoves()) {
        output_ << ' ' << move.toUci();
    }
    output_ << '\n';
}

void CommandProcessor::printSearchStats() const {
    // Les diagnostics sont optionnels pour ne pas ajouter une ligne inconnue
    // aux clients utilisant le protocole minimal.
    if (diagnostics_) {
        output_ << session_.lastSearchStats().toProtocolLine() << '\n';
    }
}

bool CommandProcessor::playFreeMove(const std::string& moveText) {
    if (session_.isGameOver()) {
        return false;
    }
    const std::optional<chess::Move> move = session_.board().findLegalMove(moveText);
    return move.has_value() && session_.board().makeMove(*move);
}

} // namespace app
