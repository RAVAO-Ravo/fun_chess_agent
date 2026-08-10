/**
 * @file evaluator.cpp
 * @brief Mesurer le matériel, la structure et l’activité des deux camps.
 *
 * Le score final est exprimé du point de vue des Blancs. La recherche le
 * retourne ensuite selon le camp au trait pour conserver l’invariant negamax.
 */

#include "search/evaluation/evaluator.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <utility>
#include <vector>

namespace ai {

namespace {

int sign(chess::Color color) {
    return color == chess::Color::White ? 1 : -1;
}

bool isPawn(const chess::Position& board, chess::Square square, chess::Color color) {
    chess::Piece piece = board.pieceAt(square);
    return piece.type() == chess::PieceType::Pawn && piece.color() == color;
}

int colorIndex(chess::Color color) {
    return color == chess::Color::White ? 0 : 1;
}

bool inside(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

bool hasFriendlyPawnOnAdjacentFile(const std::array<int, 8>& fileCounts, int file) {
    return (file > 0 && fileCounts[file - 1] > 0)
        || (file < 7 && fileCounts[file + 1] > 0);
}

bool isProtectedByPawn(
    const std::array<std::array<bool, 64>, 2>& pawnsBySquare,
    chess::Square square,
    chess::Color color) {
    const int sourceRow = square.row() + (color == chess::Color::White ? 1 : -1);
    for (int dc : {-1, 1}) {
        chess::Square source(sourceRow, square.col() + dc);
        if (source.isValid() && pawnsBySquare[colorIndex(color)][source.index()]) {
            return true;
        }
    }
    return false;
}

bool isPassedPawn(
    const std::vector<chess::Square>& enemyPawns,
    chess::Square square,
    chess::Color color) {
    const int direction = color == chess::Color::White ? -1 : 1;
    for (chess::Square enemyPawn : enemyPawns) {
        // Seuls la même colonne et les colonnes voisines peuvent empêcher le
        // pion d’avancer sans rencontrer de pion adverse.
        if (std::abs(enemyPawn.col() - square.col()) > 1) {
            continue;
        }
        if ((enemyPawn.row() - square.row()) * direction > 0) {
            return false;
        }
    }
    return true;
}

int pawnAdvancement(chess::Square square, chess::Color color) {
    if (color == chess::Color::White) {
        return std::max(0, 6 - square.row());
    }
    return std::max(0, square.row() - 1);
}

int countPawnMobility(const chess::Position& board, chess::Square from, chess::Color color) {
    const int direction = color == chess::Color::White ? -1 : 1;
    const int startRow = color == chess::Color::White ? 6 : 1;
    int mobility = 0;

    chess::Square oneStep(from.row() + direction, from.col());
    if (oneStep.isValid() && board.pieceAt(oneStep).isEmpty()) {
        ++mobility;
        chess::Square twoStep(from.row() + 2 * direction, from.col());
        if (from.row() == startRow
            && twoStep.isValid()
            && board.pieceAt(twoStep).isEmpty()) {
            ++mobility;
        }
    }

    for (int dc : {-1, 1}) {
        chess::Square target(from.row() + direction, from.col() + dc);
        if (!target.isValid()) {
            continue;
        }
        chess::Piece piece = board.pieceAt(target);
        if (!piece.isEmpty()
            && piece.color() != color
            && piece.type() != chess::PieceType::King) {
            ++mobility;
        }
    }
    return mobility;
}

int countStepMobility(
    const chess::Position& board,
    chess::Square from,
    chess::Color color,
    const int offsets[][2],
    int offsetCount) {
    int mobility = 0;
    for (int i = 0; i < offsetCount; ++i) {
        chess::Square target(from.row() + offsets[i][0], from.col() + offsets[i][1]);
        if (!target.isValid()) {
            continue;
        }
        chess::Piece piece = board.pieceAt(target);
        if (piece.isEmpty()
            || (piece.color() != color
                && piece.type() != chess::PieceType::King)) {
            ++mobility;
        }
    }
    return mobility;
}

int countSlidingMobility(
    const chess::Position& board,
    chess::Square from,
    chess::Color color,
    const int directions[][2],
    int directionCount) {
    int mobility = 0;
    for (int i = 0; i < directionCount; ++i) {
        int row = from.row() + directions[i][0];
        int col = from.col() + directions[i][1];
        while (inside(row, col)) {
            chess::Piece piece = board.pieceAt(chess::Square(row, col));
            if (piece.isEmpty()) {
                ++mobility;
            } else {
                if (piece.color() != color && piece.type() != chess::PieceType::King) {
                    ++mobility;
                }
                break;
            }
            row += directions[i][0];
            col += directions[i][1];
        }
    }
    return mobility;
}

int pseudoMobility(const chess::Position& board, chess::Square square, chess::Piece piece) {
    // La mobilité est volontairement pseudo-légale : tester les échecs pour
    // chaque pièce transformerait l’évaluation statique en génération coûteuse.
    static constexpr int knightOffsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1},
    };
    static constexpr int bishopDirections[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    static constexpr int rookDirections[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    static constexpr int queenDirections[8][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    static constexpr int kingOffsets[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},           {0, 1},
        {1, -1},  {1, 0},  {1, 1},
    };

    switch (piece.type()) {
    case chess::PieceType::Pawn:
        return countPawnMobility(board, square, piece.color());
    case chess::PieceType::Knight:
        return countStepMobility(board, square, piece.color(), knightOffsets, 8);
    case chess::PieceType::Bishop:
        return countSlidingMobility(board, square, piece.color(), bishopDirections, 4);
    case chess::PieceType::Rook:
        return countSlidingMobility(board, square, piece.color(), rookDirections, 4);
    case chess::PieceType::Queen:
        return countSlidingMobility(board, square, piece.color(), queenDirections, 8);
    case chess::PieceType::King:
        return countStepMobility(board, square, piece.color(), kingOffsets, 8);
    case chess::PieceType::None:
        return 0;
    }
    return 0;
}

struct SideFeatures {
    std::array<int, 8> pawnsByFile{};
    std::vector<chess::Square> pawns;
    chess::Square king;
    int mobility = 0;
    int bishops = 0;
    int undevelopedMinors = 0;
};

struct PositionFeatures {
    std::array<SideFeatures, 2> sides;
    std::array<std::array<bool, 64>, 2> pawnsBySquare{};
};

int evaluatePawnStructure(
    const PositionFeatures& features,
    const EvaluationParameters& parameters,
    chess::Color color) {
    const SideFeatures& side = features.sides[colorIndex(color)];
    int score = 0;
    for (int count : side.pawnsByFile) {
        // Chaque pion au-delà du premier sur une colonne subit la pénalité.
        if (count > 1) {
            score -= parameters.doubledPawnPenalty * (count - 1);
        }
    }

    for (chess::Square pawn : side.pawns) {
        if (!hasFriendlyPawnOnAdjacentFile(side.pawnsByFile, pawn.col())) {
            score -= parameters.isolatedPawnPenalty;
        }
        if (isProtectedByPawn(features.pawnsBySquare, pawn, color)) {
            score += parameters.protectedPawnBonus;
        }
        if (isPassedPawn(features.sides[colorIndex(chess::opposite(color))].pawns, pawn, color)) {
            const int advancement = pawnAdvancement(pawn, color);
            // Plus un pion passé approche de la promotion, plus sa valeur
            // stratégique dépasse le bonus de base appris.
            score += parameters.passedPawnBonus + (parameters.passedPawnBonus * advancement) / 3;
        }
    }
    return score;
}

int evaluateKingShield(
    const chess::Position& board,
    const EvaluationParameters& parameters,
    chess::Square king,
    chess::Color color) {
    if (!king.isValid()) {
        return 0;
    }

    int shield = 0;
    int shieldRow = king.row() + (color == chess::Color::White ? -1 : 1);
    for (int file = std::max(0, king.col() - 1); file <= std::min(7, king.col() + 1); ++file) {
        chess::Square square(shieldRow, file);
        if (square.isValid() && isPawn(board, square, color)) {
            ++shield;
        }
    }
    return shield * parameters.kingShieldBonus;
}

int evaluateSideFeatures(
    const chess::Position& board,
    const PositionFeatures& features,
    const EvaluationParameters& parameters,
    chess::Color color) {
    const SideFeatures& side = features.sides[colorIndex(color)];
    return evaluatePawnStructure(features, parameters, color)
        + side.mobility * parameters.mobilityBonus
        + (side.bishops >= 2 ? parameters.bishopPairBonus : 0)
        + evaluateKingShield(board, parameters, side.king, color)
        - side.undevelopedMinors * parameters.undevelopedMinorPenalty;
}

} // namespace

Evaluator::Evaluator(EvaluationParameters parameters)
    : parameters_(std::move(parameters)) {
}

int Evaluator::evaluate(const chess::Position& board) const {
    // === Matériel ===

    // Les comptes maintenus par Position rendent cette étape constante pour
    // chaque type, indépendamment du nombre de cases occupées.
    int score = 0;
    for (const chess::PieceType type : {
             chess::PieceType::King,
             chess::PieceType::Queen,
             chess::PieceType::Rook,
             chess::PieceType::Bishop,
             chess::PieceType::Knight,
             chess::PieceType::Pawn}) {
        score +=
            (board.pieceCount(chess::Color::White, type)
             - board.pieceCount(chess::Color::Black, type))
            * pieceValue(parameters_, type);
    }
    // === Extraction unique des caractéristiques positionnelles ===

    // Un seul parcours alimente toutes les composantes afin de ne pas rescanner
    // le plateau séparément pour la mobilité, les pions et la sécurité du roi.
    PositionFeatures features;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            chess::Square square(row, col);
            chess::Piece piece = board.pieceAt(square);
            if (piece.isEmpty()) {
                continue;
            }

            SideFeatures& side = features.sides[colorIndex(piece.color())];
            side.mobility += pseudoMobility(board, square, piece);
            switch (piece.type()) {
            case chess::PieceType::Pawn:
                ++side.pawnsByFile[col];
                side.pawns.push_back(square);
                features.pawnsBySquare[colorIndex(piece.color())][square.index()] = true;
                break;
            case chess::PieceType::Bishop:
                ++side.bishops;
                if ((piece.color() == chess::Color::White && row == 7 && (col == 2 || col == 5))
                    || (piece.color() == chess::Color::Black && row == 0 && (col == 2 || col == 5))) {
                    ++side.undevelopedMinors;
                }
                break;
            case chess::PieceType::Knight:
                if ((piece.color() == chess::Color::White && row == 7 && (col == 1 || col == 6))
                    || (piece.color() == chess::Color::Black && row == 0 && (col == 1 || col == 6))) {
                    ++side.undevelopedMinors;
                }
                break;
            case chess::PieceType::King:
                side.king = square;
                break;
            case chess::PieceType::Queen:
            case chess::PieceType::Rook:
            case chess::PieceType::None:
                break;
            }
        }
    }
    // === Agrégation symétrique ===

    score += sign(chess::Color::White) * evaluateSideFeatures(board, features, parameters_, chess::Color::White);
    score += sign(chess::Color::Black) * evaluateSideFeatures(board, features, parameters_, chess::Color::Black);
    return score;
}

} // namespace ai
