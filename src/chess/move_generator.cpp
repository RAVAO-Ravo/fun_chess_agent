/**
 * @file move_generator.cpp
 * @brief Générer les coups pseudo-légaux, tactiques et pleinement légaux.
 *
 * La légalité finale est vérifiée en jouant temporairement chaque candidat :
 * le roi du camp actif ne doit jamais rester attaqué.
 */

#include "chess/move_generator.hpp"

#include <algorithm>

namespace chess {

namespace {

bool inside(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

bool isEnemyTarget(const Position& board, Square square, Color ownColor) {
    Piece target = board.pieceAt(square);
    // Le roi n’est jamais capturé : la légalité est exprimée par l’échec et
    // l’absence de réponse, ce qui préserve un roi pour chaque camp.
    return !target.isEmpty() && target.color() != ownColor && target.type() != PieceType::King;
}

void addMoveOrPromotion(Square from, Square to, bool capture, std::vector<Move>& moves) {
    const PieceType promotions[] = {
        PieceType::Queen,
        PieceType::Rook,
        PieceType::Bishop,
        PieceType::Knight,
    };
    if (to.row() == 0 || to.row() == 7) {
        // Chaque promotion est un coup UCI distinct et doit donc apparaître
        // séparément dans la liste légale.
        for (PieceType type : promotions) {
            Move move(from, to);
            move.setPromotion(type);
            move.setCapture(capture);
            moves.push_back(move);
        }
        return;
    }

    Move move(from, to);
    move.setCapture(capture);
    moves.push_back(move);
}

} // namespace

std::vector<Move> MoveGenerator::pseudoLegalMoves(
    const Position& board,
    MoveGenerationMode mode) {
    std::vector<Move> moves;
    moves.reserve(64);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            Square from(row, col);
            Piece piece = board.pieceAt(from);
            if (piece.isEmpty() || piece.color() != board.sideToMove()) {
                continue;
            }
            addMovesForPiece(board, from, mode, moves);
        }
    }
    return moves;
}

std::vector<Move> MoveGenerator::legalMoves(
    Position& board,
    MoveGenerationMode mode,
    MoveAnnotationMode annotations) {
    std::vector<Move> legal;
    Color movingColor = board.sideToMove();
    std::vector<Move> pseudoMoves = pseudoLegalMoves(board, mode);
    legal.reserve(pseudoMoves.size());
    for (Move move : pseudoMoves) {
        // Jouer puis annuler évite de dupliquer la logique des pièces clouées,
        // des échecs découverts et des prises en passant exposant le roi.
        if (!board.makeMove(move)) {
            continue;
        }
        if (!board.isInCheck(movingColor)) {
            if (annotations == MoveAnnotationMode::Ordering) {
                // Ces métadonnées coûtent des tests d’attaque supplémentaires ;
                // elles ne sont produites que lorsque la recherche les exploite.
                move.setOrderingMetadata(
                    board.isInCheck(board.sideToMove()),
                    isSquareAttacked(
                        board,
                        move.to(),
                        board.sideToMove()));
            }
            legal.push_back(move);
        }
        board.undoMove();
    }
    return legal;
}

std::vector<Move> MoveGenerator::legalMoves(
    const Position& board,
    MoveGenerationMode mode,
    MoveAnnotationMode annotations) {
    Position copy = board.copyWithoutHistory();
    return legalMoves(copy, mode, annotations);
}

bool MoveGenerator::hasAnyLegalMove(Position& board) {
    const Color movingColor = board.sideToMove();
    // Le tampon par thread supprime les allocations répétées de ce test très
    // fréquent sans partager de mémoire mutable entre recherches parallèles.
    thread_local std::vector<Move> candidates;
    candidates.clear();
    if (candidates.capacity() < 32) {
        candidates.reserve(32);
    }
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            const Square from(row, col);
            const Piece piece = board.pieceAt(from);
            if (piece.isEmpty() || piece.color() != movingColor) {
                continue;
            }
            candidates.clear();
            addMovesForPiece(
                board,
                from,
                MoveGenerationMode::All,
                candidates);
            for (const Move& move : candidates) {
                if (!board.makeMove(move)) {
                    continue;
                }
                const bool legal = !board.isInCheck(movingColor);
                board.undoMove();
                if (legal) {
                    // Mat, pat et quiescence n’ont besoin que d’une réponse
                    // booléenne : arrêter ici évite de construire la liste entière.
                    return true;
                }
            }
        }
    }
    return false;
}

bool MoveGenerator::hasAnyLegalMove(const Position& board) {
    Position copy = board.copyWithoutHistory();
    return hasAnyLegalMove(copy);
}

bool MoveGenerator::isSquareAttacked(const Position& board, Square square, Color byColor) {
    if (!square.isValid() || byColor == Color::None) {
        return false;
    }

    // On cherche les sources potentielles depuis la case cible ; cette méthode
    // évite la récursion qu’entraînerait une génération de coups adverse.
    int pawnSourceRow = square.row() + (byColor == Color::White ? 1 : -1);
    for (int dc : {-1, 1}) {
        Square source(pawnSourceRow, square.col() + dc);
        if (source.isValid()) {
            Piece piece = board.pieceAt(source);
            if (piece.type() == PieceType::Pawn && piece.color() == byColor) {
                return true;
            }
        }
    }

    const int knightOffsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1},
    };
    for (const auto& offset : knightOffsets) {
        Square source(square.row() + offset[0], square.col() + offset[1]);
        if (source.isValid()) {
            Piece piece = board.pieceAt(source);
            if (piece.type() == PieceType::Knight && piece.color() == byColor) {
                return true;
            }
        }
    }

    const int diagonalDirections[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    for (const auto& direction : diagonalDirections) {
        int row = square.row() + direction[0];
        int col = square.col() + direction[1];
        while (inside(row, col)) {
            // La première pièce rencontrée masque toutes les suivantes sur ce rayon.
            Piece piece = board.pieceAt(Square(row, col));
            if (!piece.isEmpty()) {
                if (piece.color() == byColor
                    && (piece.type() == PieceType::Bishop
                        || piece.type() == PieceType::Queen)) {
                    return true;
                }
                break;
            }
            row += direction[0];
            col += direction[1];
        }
    }

    const int orthogonalDirections[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& direction : orthogonalDirections) {
        int row = square.row() + direction[0];
        int col = square.col() + direction[1];
        while (inside(row, col)) {
            Piece piece = board.pieceAt(Square(row, col));
            if (!piece.isEmpty()) {
                if (piece.color() == byColor && (piece.type() == PieceType::Rook || piece.type() == PieceType::Queen)) {
                    return true;
                }
                break;
            }
            row += direction[0];
            col += direction[1];
        }
    }

    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            Square source(square.row() + dr, square.col() + dc);
            if (source.isValid()) {
                Piece piece = board.pieceAt(source);
                if (piece.type() == PieceType::King && piece.color() == byColor) {
                    return true;
                }
            }
        }
    }

    return false;
}

void MoveGenerator::addMovesForPiece(
    const Position& board,
    Square from,
    MoveGenerationMode mode,
    std::vector<Move>& moves) {
    const Piece piece = board.pieceAt(from);
    switch (piece.type()) {
    case PieceType::Pawn:
        addPawnMoves(board, from, mode, moves);
        break;
    case PieceType::Knight:
        addKnightMoves(board, from, mode, moves);
        break;
    case PieceType::Bishop: {
        const int directions[4][2] = {
            {-1, -1},
            {-1, 1},
            {1, -1},
            {1, 1},
        };
        addSlidingMoves(board, from, directions, 4, mode, moves);
        break;
    }
    case PieceType::Rook: {
        const int directions[4][2] = {
            {-1, 0},
            {1, 0},
            {0, -1},
            {0, 1},
        };
        addSlidingMoves(board, from, directions, 4, mode, moves);
        break;
    }
    case PieceType::Queen: {
        const int directions[8][2] = {
            {-1, -1},
            {-1, 1},
            {1, -1},
            {1, 1},
            {-1, 0},
            {1, 0},
            {0, -1},
            {0, 1},
        };
        addSlidingMoves(board, from, directions, 8, mode, moves);
        break;
    }
    case PieceType::King:
        addKingMoves(board, from, mode, moves);
        break;
    case PieceType::None:
        break;
    }
}

void MoveGenerator::addPawnMoves(
    const Position& board,
    Square from,
    MoveGenerationMode mode,
    std::vector<Move>& moves) {
    Piece pawn = board.pieceAt(from);
    Color color = pawn.color();
    int direction = color == Color::White ? -1 : 1;
    int startRow = color == Color::White ? 6 : 1;

    Square oneStep(from.row() + direction, from.col());
    if (oneStep.isValid() && board.pieceAt(oneStep).isEmpty()) {
        const bool promotion = oneStep.row() == 0 || oneStep.row() == 7;
        if (promotion || mode == MoveGenerationMode::All) {
            addMoveOrPromotion(from, oneStep, false, moves);
        }
        if (mode == MoveGenerationMode::All && !promotion) {
            Square twoStep(from.row() + 2 * direction, from.col());
            // La case intermédiaire est déjà libre puisque ``oneStep`` a été
            // validée avant ce bloc.
            if (from.row() == startRow && twoStep.isValid() && board.pieceAt(twoStep).isEmpty()) {
                moves.emplace_back(from, twoStep);
            }
        }
    }

    for (int dc : {-1, 1}) {
        Square target(from.row() + direction, from.col() + dc);
        if (!target.isValid()) {
            continue;
        }
        if (isEnemyTarget(board, target, color)) {
            addMoveOrPromotion(from, target, true, moves);
        }
        if (board.enPassantSquare().has_value() && target == *board.enPassantSquare()) {
            // La cible FEN suffit à marquer le coup ; makeMove retirera le pion
            // situé à côté de la case de départ.
            Move move(from, target);
            move.setCapture(true);
            move.setEnPassant(true);
            moves.push_back(move);
        }
    }
}

void MoveGenerator::addKnightMoves(
    const Position& board,
    Square from,
    MoveGenerationMode mode,
    std::vector<Move>& moves) {
    Piece knight = board.pieceAt(from);
    const int offsets[8][2] = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1},
    };

    for (const auto& offset : offsets) {
        Square target(from.row() + offset[0], from.col() + offset[1]);
        if (!target.isValid()) {
            continue;
        }
        Piece piece = board.pieceAt(target);
        if (piece.isEmpty() && mode == MoveGenerationMode::All) {
            moves.emplace_back(from, target);
        } else if (isEnemyTarget(board, target, knight.color())) {
            Move move(from, target);
            move.setCapture(true);
            moves.push_back(move);
        }
    }
}

void MoveGenerator::addSlidingMoves(
    const Position& board,
    Square from,
    const int directions[][2],
    int directionCount,
    MoveGenerationMode mode,
    std::vector<Move>& moves) {
    Piece slider = board.pieceAt(from);
    for (int i = 0; i < directionCount; ++i) {
        int row = from.row() + directions[i][0];
        int col = from.col() + directions[i][1];
        while (inside(row, col)) {
            Square target(row, col);
            Piece piece = board.pieceAt(target);
            if (piece.isEmpty() && mode == MoveGenerationMode::All) {
                moves.emplace_back(from, target);
            } else if (!piece.isEmpty()) {
                if (isEnemyTarget(board, target, slider.color())) {
                    Move move(from, target);
                    move.setCapture(true);
                    moves.push_back(move);
                }
                break;
            }
            row += directions[i][0];
            col += directions[i][1];
        }
    }
}

void MoveGenerator::addKingMoves(
    const Position& board,
    Square from,
    MoveGenerationMode mode,
    std::vector<Move>& moves) {
    Piece king = board.pieceAt(from);
    Color color = king.color();
    Color enemy = opposite(color);

    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            Square target(from.row() + dr, from.col() + dc);
            if (!target.isValid()) {
                continue;
            }
            Piece piece = board.pieceAt(target);
            if (piece.isEmpty() && mode == MoveGenerationMode::All) {
                moves.emplace_back(from, target);
            } else if (isEnemyTarget(board, target, color)) {
                Move move(from, target);
                move.setCapture(true);
                moves.push_back(move);
            }
        }
    }

    if (mode == MoveGenerationMode::Tactical) {
        // Les déplacements calmes et les roques ne stabilisent pas une feuille
        // de quiescence, ils sont donc exclus de cette génération.
        return;
    }

    int homeRow = color == Color::White ? 7 : 0;
    if (from != Square(homeRow, 4) || board.isInCheck(color)) {
        return;
    }

    // La génération vérifie les cases traversées, tandis que le filtrage légal
    // vérifiera une dernière fois que la position finale protège le roi.
    Piece kingRook = board.pieceAt(Square(homeRow, 7));
    if (board.canCastleKingSide(color)
        && kingRook.type() == PieceType::Rook
        && kingRook.color() == color
        && board.pieceAt(Square(homeRow, 5)).isEmpty()
        && board.pieceAt(Square(homeRow, 6)).isEmpty()
        && !isSquareAttacked(board, Square(homeRow, 5), enemy)
        && !isSquareAttacked(board, Square(homeRow, 6), enemy)) {
        Move move(from, Square(homeRow, 6));
        move.setCastle(true);
        moves.push_back(move);
    }

    Piece queenRook = board.pieceAt(Square(homeRow, 0));
    if (board.canCastleQueenSide(color)
        && queenRook.type() == PieceType::Rook
        && queenRook.color() == color
        && board.pieceAt(Square(homeRow, 1)).isEmpty()
        && board.pieceAt(Square(homeRow, 2)).isEmpty()
        && board.pieceAt(Square(homeRow, 3)).isEmpty()
        && !isSquareAttacked(board, Square(homeRow, 3), enemy)
        && !isSquareAttacked(board, Square(homeRow, 2), enemy)) {
        Move move(from, Square(homeRow, 2));
        move.setCastle(true);
        moves.push_back(move);
    }
}

} // namespace chess
