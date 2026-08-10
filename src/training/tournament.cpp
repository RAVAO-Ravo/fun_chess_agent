/**
 * @file tournament.cpp
 * @brief Organiser les parties d’évaluation depuis des positions diversifiées.
 */

#include "training/tournament.hpp"

#include "search/chess_ai.hpp"
#include "chess/position.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace training {

namespace {

std::string trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string stripComment(const std::string& text) {
    const std::size_t comment = text.find('#');
    if (comment == std::string::npos) {
        return text;
    }
    return text.substr(0, comment);
}

int objectivePieceValue(chess::PieceType type) {
    switch (type) {
    case chess::PieceType::Pawn:
        return 100;
    case chess::PieceType::Knight:
        return 320;
    case chess::PieceType::Bishop:
        return 330;
    case chess::PieceType::Rook:
        return 500;
    case chess::PieceType::Queen:
        return 900;
    default:
        return 0;
    }
}

int objectiveEvaluation(const chess::Position& board, GameResult result) {
    if (result == GameResult::WhiteWin) {
        return 100000;
    }
    if (result == GameResult::BlackWin) {
        return -100000;
    }

    // Cette mesure utilise des valeurs fixes et non les poids des concurrents :
    // aucun individu ne peut favoriser artificiellement sa propre évaluation.
    int evaluation = 0;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            chess::Piece piece = board.pieceAt(chess::Square(row, col));
            if (piece.isEmpty()) {
                continue;
            }
            int value = objectivePieceValue(piece.type());
            evaluation += piece.color() == chess::Color::White ? value : -value;
        }
    }
    return evaluation;
}

} // namespace

Tournament::Tournament(
    int maxHalfMoves,
    std::vector<chess::Position> startingPositions,
    int quiescenceMaxPly)
    : maxHalfMoves_(maxHalfMoves)
    , quiescenceMaxPly_(quiescenceMaxPly)
    , startingPositions_(std::move(startingPositions)) {
    if (quiescenceMaxPly_ < 1) {
        throw std::invalid_argument("quiescence depth must be positive");
    }
}

MatchResult Tournament::playGame(
    const Individual& white,
    const Individual& black,
    std::size_t startingPositionIndex) const {
    chess::Position board = startingBoard(startingPositionIndex);
    ai::ChessAI whiteAI(white.parameters());
    ai::ChessAI blackAI(black.parameters());
    ai::SearchLimits whiteLimits;
    whiteLimits.maxDepth = white.parameters().searchDepth;
    whiteLimits.quiescenceMaxPly = quiescenceMaxPly_;
    ai::SearchLimits blackLimits;
    blackLimits.maxDepth = black.parameters().searchDepth;
    blackLimits.quiescenceMaxPly = quiescenceMaxPly_;
    MatchResult result;

    // La même profondeur de quiescence est imposée aux deux concurrents pour
    // que seule leur génétique distingue leurs décisions.
    for (int halfMove = 0; halfMove < maxHalfMoves_; ++halfMove) {
        const chess::GameTermination termination = board.termination();
        if (termination == chess::GameTermination::Checkmate) {
            result.result = board.sideToMove() == chess::Color::White ? GameResult::BlackWin : GameResult::WhiteWin;
            result.halfMovesPlayed = halfMove;
            result.finalEvaluation = objectiveEvaluation(board, result.result);
            return result;
        }
        if (termination == chess::GameTermination::Stalemate
            || termination == chess::GameTermination::RuleDraw) {
            result.result = GameResult::Draw;
            result.halfMovesPlayed = halfMove;
            result.finalEvaluation = objectiveEvaluation(board, result.result);
            return result;
        }

        chess::Move move = board.sideToMove() == chess::Color::White
            ? whiteAI.chooseMove(board, whiteLimits)
            : blackAI.chooseMove(board, blackLimits);
        if (!move.from().isValid()) {
            // Un coup invalide sert de garde-fou si un moteur ne trouve aucune
            // réponse malgré un état terminal non détecté en amont.
            result.result = board.isInCheck(board.sideToMove())
                ? (board.sideToMove() == chess::Color::White ? GameResult::BlackWin : GameResult::WhiteWin)
                : GameResult::Draw;
            result.halfMovesPlayed = halfMove;
            result.finalEvaluation = objectiveEvaluation(board, result.result);
            return result;
        }
        board.makeMove(move);
    }

    // La limite de longueur borne strictement le coût d’une confrontation ;
    // l’évaluation objective reste disponible pour départager ailleurs.
    result.result = GameResult::Draw;
    result.halfMovesPlayed = maxHalfMoves_;
    result.finalEvaluation = objectiveEvaluation(board, result.result);
    return result;
}

chess::Position Tournament::startingBoard(std::size_t startingPositionIndex) const {
    if (startingPositions_.empty()) {
        return chess::Position();
    }
    // Le modulo autorise un calendrier plus long que le corpus d’ouvertures.
    return startingPositions_[startingPositionIndex % startingPositions_.size()];
}

std::vector<chess::Position> loadTrainingPositionsFromFenFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot load training positions from " + path);
    }

    std::vector<chess::Position> positions;
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::string fen = trim(stripComment(line));
        if (fen.empty()) {
            continue;
        }

        try {
            positions.push_back(chess::Position::fromFen(fen));
        } catch (const std::exception& error) {
            std::ostringstream message;
            message << "invalid training FEN on line " << lineNumber << " in " << path << ": " << error.what();
            throw std::runtime_error(message.str());
        }
    }

    if (positions.empty()) {
        throw std::runtime_error("training positions file is empty: " + path);
    }
    return positions;
}

} // namespace training
