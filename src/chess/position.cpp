/**
 * @file position.cpp
 * @brief Appliquer, annuler et qualifier les positions d’échecs.
 *
 * Les mutations du plateau mettent à jour simultanément les données mises en
 * cache. Chaque coup enregistre un GameState suffisant pour une annulation
 * exacte, opération centrale de la recherche arborescente.
 */

#include "chess/position.hpp"

#include "chess/fen.hpp"
#include "chess/move_generator.hpp"
#include "chess/zobrist.hpp"

#include <algorithm>
#include <cstdlib>

namespace chess {

Position::Position() {
    reset();
}

void Position::reset() {
    clear();
    setSideToMove(Color::White);
    setCastlingRights(Color::White, true, true);
    setCastlingRights(Color::Black, true, true);
    setHalfmoveClock(0);
    setFullmoveNumber(1);

    setPiece(Square(0, 0), Piece(PieceType::Rook, Color::Black));
    setPiece(Square(0, 1), Piece(PieceType::Knight, Color::Black));
    setPiece(Square(0, 2), Piece(PieceType::Bishop, Color::Black));
    setPiece(Square(0, 3), Piece(PieceType::Queen, Color::Black));
    setPiece(Square(0, 4), Piece(PieceType::King, Color::Black));
    setPiece(Square(0, 5), Piece(PieceType::Bishop, Color::Black));
    setPiece(Square(0, 6), Piece(PieceType::Knight, Color::Black));
    setPiece(Square(0, 7), Piece(PieceType::Rook, Color::Black));
    for (int col = 0; col < 8; ++col) {
        setPiece(Square(1, col), Piece(PieceType::Pawn, Color::Black));
    }

    for (int col = 0; col < 8; ++col) {
        setPiece(Square(6, col), Piece(PieceType::Pawn, Color::White));
    }
    setPiece(Square(7, 0), Piece(PieceType::Rook, Color::White));
    setPiece(Square(7, 1), Piece(PieceType::Knight, Color::White));
    setPiece(Square(7, 2), Piece(PieceType::Bishop, Color::White));
    setPiece(Square(7, 3), Piece(PieceType::Queen, Color::White));
    setPiece(Square(7, 4), Piece(PieceType::King, Color::White));
    setPiece(Square(7, 5), Piece(PieceType::Bishop, Color::White));
    setPiece(Square(7, 6), Piece(PieceType::Knight, Color::White));
    setPiece(Square(7, 7), Piece(PieceType::Rook, Color::White));
}

void Position::clear() {
    squares_.fill(Piece());
    pieceCounts_ = {};
    sideToMove_ = Color::White;
    whiteCanCastleKingSide_ = false;
    whiteCanCastleQueenSide_ = false;
    blackCanCastleKingSide_ = false;
    blackCanCastleQueenSide_ = false;
    enPassantSquare_.reset();
    halfmoveClock_ = 0;
    fullmoveNumber_ = 1;
    whiteKingSquare_ = Square();
    blackKingSquare_ = Square();
    zobristHash_ = 0;
    history_.clear();
}

Piece Position::pieceAt(Square square) const {
    if (!square.isValid()) {
        return Piece();
    }
    return squares_[square.index()];
}

void Position::setPiece(Square square, Piece piece) {
    if (square.isValid()) {
        Piece oldPiece = squares_[square.index()];
        // Les comptes de pièces alimentent l’évaluation et la détection du
        // matériel insuffisant ; les maintenir ici évite deux parcours répétés.
        if (!oldPiece.isEmpty()) {
            const std::size_t colorIndex =
                oldPiece.color() == Color::White ? 0u : 1u;
            --pieceCounts_[colorIndex][static_cast<std::size_t>(pieceTypeIndex(oldPiece.type()))];
        }
        if (!piece.isEmpty()) {
            const std::size_t colorIndex =
                piece.color() == Color::White ? 0u : 1u;
            ++pieceCounts_[colorIndex][static_cast<std::size_t>(pieceTypeIndex(piece.type()))];
        }
        // Le XOR retire naturellement l’ancienne clé puis ajoute la nouvelle.
        zobristHash_ ^= Zobrist::pieceKey(oldPiece, square);
        zobristHash_ ^= Zobrist::pieceKey(piece, square);
        squares_[square.index()] = piece;
        if (piece.type() == PieceType::King) {
            if (piece.color() == Color::White) {
                whiteKingSquare_ = square;
            } else if (piece.color() == Color::Black) {
                blackKingSquare_ = square;
            }
        } else {
            if (square == whiteKingSquare_) {
                whiteKingSquare_ = Square();
            }
            if (square == blackKingSquare_) {
                blackKingSquare_ = Square();
            }
        }
    }
}

int Position::pieceCount(Color color, PieceType type) const {
    const int typeIndex = pieceTypeIndex(type);
    if (color == Color::None || typeIndex < 0) {
        return 0;
    }
    const std::size_t colorIndex = color == Color::White ? 0u : 1u;
    return pieceCounts_[colorIndex][static_cast<std::size_t>(typeIndex)];
}

Color Position::sideToMove() const {
    return sideToMove_;
}

void Position::setSideToMove(Color color) {
    if (sideToMove_ != color) {
        // Les deux XOR rendent cette mutation correcte même pour Color::None,
        // utilisé pendant certaines constructions de position.
        zobristHash_ ^= Zobrist::sideToMoveKey(sideToMove_);
        zobristHash_ ^= Zobrist::sideToMoveKey(color);
    }
    sideToMove_ = color;
}

bool Position::canCastleKingSide(Color color) const {
    if (color == Color::White) {
        return whiteCanCastleKingSide_;
    }
    if (color == Color::Black) {
        return blackCanCastleKingSide_;
    }
    return false;
}

bool Position::canCastleQueenSide(Color color) const {
    if (color == Color::White) {
        return whiteCanCastleQueenSide_;
    }
    if (color == Color::Black) {
        return blackCanCastleQueenSide_;
    }
    return false;
}

void Position::setCastlingRights(Color color, bool kingSide, bool queenSide) {
    if (color == Color::White) {
        if (whiteCanCastleKingSide_ != kingSide) {
            zobristHash_ ^= Zobrist::castlingRightKey(Color::White, true);
        }
        if (whiteCanCastleQueenSide_ != queenSide) {
            zobristHash_ ^= Zobrist::castlingRightKey(Color::White, false);
        }
        whiteCanCastleKingSide_ = kingSide;
        whiteCanCastleQueenSide_ = queenSide;
    } else if (color == Color::Black) {
        if (blackCanCastleKingSide_ != kingSide) {
            zobristHash_ ^= Zobrist::castlingRightKey(Color::Black, true);
        }
        if (blackCanCastleQueenSide_ != queenSide) {
            zobristHash_ ^= Zobrist::castlingRightKey(Color::Black, false);
        }
        blackCanCastleKingSide_ = kingSide;
        blackCanCastleQueenSide_ = queenSide;
    }
}

std::optional<Square> Position::enPassantSquare() const {
    return enPassantSquare_;
}

void Position::setEnPassantSquare(std::optional<Square> square) {
    // L’empreinte ne conserve que la colonne, seule information pertinente
    // pour distinguer les possibilités de prise en passant.
    if (enPassantSquare_.has_value()) {
        zobristHash_ ^= Zobrist::enPassantFileKey(enPassantSquare_->col());
    }
    enPassantSquare_ = square;
    if (enPassantSquare_.has_value()) {
        zobristHash_ ^= Zobrist::enPassantFileKey(enPassantSquare_->col());
    }
}

int Position::halfmoveClock() const {
    return halfmoveClock_;
}

int Position::fullmoveNumber() const {
    return fullmoveNumber_;
}

void Position::setHalfmoveClock(int value) {
    halfmoveClock_ = value;
}

void Position::setFullmoveNumber(int value) {
    fullmoveNumber_ = value;
}

bool Position::makeMove(const Move& move) {
    if (!move.from().isValid() || !move.to().isValid()) {
        return false;
    }

    Piece movedPiece = pieceAt(move.from());
    if (movedPiece.isEmpty() || movedPiece.color() != sideToMove_) {
        return false;
    }

    // === Sauvegarde de l’état réversible ===

    // Toutes les données non déductibles du plateau après le coup sont
    // enregistrées avant la première mutation.
    GameState state;
    state.move = move;
    state.movedPiece = movedPiece;
    state.oldEnPassantSquare = enPassantSquare_;
    state.oldWhiteCanCastleKingSide = whiteCanCastleKingSide_;
    state.oldWhiteCanCastleQueenSide = whiteCanCastleQueenSide_;
    state.oldBlackCanCastleKingSide = blackCanCastleKingSide_;
    state.oldBlackCanCastleQueenSide = blackCanCastleQueenSide_;
    state.oldHalfmoveClock = halfmoveClock_;
    state.oldFullmoveNumber = fullmoveNumber_;
    state.oldSideToMove = sideToMove_;
    state.oldZobristHash = zobristHash_;

    Square capturedSquare = move.to();
    if (move.isEnPassant()) {
        // La destination d’une prise en passant est vide ; le pion capturé se
        // trouve sur la ligne de départ du pion attaquant.
        capturedSquare = Square(move.from().row(), move.to().col());
    }
    state.capturedSquare = capturedSquare;
    state.capturedPiece = pieceAt(capturedSquare);

    // === Mutation du plateau ===

    setPiece(move.from(), Piece());
    if (move.isEnPassant()) {
        setPiece(capturedSquare, Piece());
    }

    Piece placedPiece = movedPiece;
    if (move.isPromotion()) {
        placedPiece = Piece(move.promotion(), movedPiece.color());
    }
    setPiece(move.to(), placedPiece);

    if (move.isCastle()) {
        // Le déplacement du roi porte l’information de roque ; la tour est
        // déplacée ici pour que l’opération reste atomique.
        int row = move.from().row();
        if (move.to().col() == 6) {
            setPiece(Square(row, 5), pieceAt(Square(row, 7)));
            setPiece(Square(row, 7), Piece());
        } else if (move.to().col() == 2) {
            setPiece(Square(row, 3), pieceAt(Square(row, 0)));
            setPiece(Square(row, 0), Piece());
        }
    }

    // === Mise à jour des droits et compteurs ===

    if (movedPiece.type() == PieceType::King) {
        setCastlingRights(movedPiece.color(), false, false);
    }
    if (movedPiece.type() == PieceType::Rook) {
        if (move.from() == Square(7, 0)) {
            setCastlingRights(Color::White, whiteCanCastleKingSide_, false);
        } else if (move.from() == Square(7, 7)) {
            setCastlingRights(Color::White, false, whiteCanCastleQueenSide_);
        } else if (move.from() == Square(0, 0)) {
            setCastlingRights(Color::Black, blackCanCastleKingSide_, false);
        } else if (move.from() == Square(0, 7)) {
            setCastlingRights(Color::Black, false, blackCanCastleQueenSide_);
        }
    }
    if (state.capturedPiece.type() == PieceType::Rook) {
        // Capturer une tour sur sa case d’origine supprime aussi le droit,
        // même si cette tour n’a jamais eu l’occasion de bouger.
        if (capturedSquare == Square(7, 0)) {
            setCastlingRights(Color::White, whiteCanCastleKingSide_, false);
        } else if (capturedSquare == Square(7, 7)) {
            setCastlingRights(Color::White, false, whiteCanCastleQueenSide_);
        } else if (capturedSquare == Square(0, 0)) {
            setCastlingRights(Color::Black, blackCanCastleKingSide_, false);
        } else if (capturedSquare == Square(0, 7)) {
            setCastlingRights(Color::Black, false, blackCanCastleQueenSide_);
        }
    }

    setEnPassantSquare(std::nullopt);
    if (movedPiece.type() == PieceType::Pawn && std::abs(move.to().row() - move.from().row()) == 2) {
        // La case stockée est celle franchie par le pion, conformément à FEN.
        setEnPassantSquare(Square((move.to().row() + move.from().row()) / 2, move.from().col()));
    }

    if (movedPiece.type() == PieceType::Pawn || !state.capturedPiece.isEmpty()) {
        setHalfmoveClock(0);
    } else {
        setHalfmoveClock(halfmoveClock_ + 1);
    }

    if (sideToMove_ == Color::Black) {
        ++fullmoveNumber_;
    }
    setSideToMove(opposite(sideToMove_));
    history_.push_back(state);
    return true;
}

void Position::undoMove() {
    if (history_.empty()) {
        return;
    }

    GameState state = history_.back();
    history_.pop_back();

    // Les champs scalaires sont restaurés directement. L’empreinte Zobrist est
    // remise à la fin depuis sa valeur exacte, ce qui évite toute dérive.
    sideToMove_ = state.oldSideToMove;
    whiteCanCastleKingSide_ = state.oldWhiteCanCastleKingSide;
    whiteCanCastleQueenSide_ = state.oldWhiteCanCastleQueenSide;
    blackCanCastleKingSide_ = state.oldBlackCanCastleKingSide;
    blackCanCastleQueenSide_ = state.oldBlackCanCastleQueenSide;
    enPassantSquare_ = state.oldEnPassantSquare;
    halfmoveClock_ = state.oldHalfmoveClock;
    fullmoveNumber_ = state.oldFullmoveNumber;

    // Cette primitive interne met à jour les mêmes caches que setPiece, mais
    // laisse volontairement l’empreinte intacte jusqu’à sa restauration finale.
    auto restorePiece = [this](Square square, Piece piece) {
        if (!square.isValid()) {
            return;
        }
        const Piece oldPiece = squares_[static_cast<std::size_t>(square.index())];
        if (!oldPiece.isEmpty()) {
            const std::size_t colorIndex =
                oldPiece.color() == Color::White ? 0u : 1u;
            --pieceCounts_[colorIndex][static_cast<std::size_t>(pieceTypeIndex(oldPiece.type()))];
        }
        if (!piece.isEmpty()) {
            const std::size_t colorIndex =
                piece.color() == Color::White ? 0u : 1u;
            ++pieceCounts_[colorIndex][static_cast<std::size_t>(pieceTypeIndex(piece.type()))];
        }
        squares_[static_cast<std::size_t>(square.index())] = piece;
        if (oldPiece.type() == PieceType::King) {
            if (oldPiece.color() == Color::White && whiteKingSquare_ == square) {
                whiteKingSquare_ = Square();
            } else if (oldPiece.color() == Color::Black && blackKingSquare_ == square) {
                blackKingSquare_ = Square();
            }
        }
        if (piece.type() == PieceType::King) {
            if (piece.color() == Color::White) {
                whiteKingSquare_ = square;
            } else {
                blackKingSquare_ = square;
            }
        }
    };

    restorePiece(state.move.from(), state.movedPiece);
    restorePiece(state.move.to(), Piece());
    if (state.move.isEnPassant()) {
        if (state.capturedSquare.has_value()) {
            restorePiece(*state.capturedSquare, state.capturedPiece);
        }
    } else {
        restorePiece(state.move.to(), state.capturedPiece);
    }

    if (state.move.isCastle()) {
        // Le roi a déjà été restauré ; il reste à replacer la tour.
        int row = state.move.from().row();
        if (state.move.to().col() == 6) {
            restorePiece(Square(row, 7), pieceAt(Square(row, 5)));
            restorePiece(Square(row, 5), Piece());
        } else if (state.move.to().col() == 2) {
            restorePiece(Square(row, 0), pieceAt(Square(row, 3)));
            restorePiece(Square(row, 3), Piece());
        }
    }
    zobristHash_ = state.oldZobristHash;
}

bool Position::canUndo() const {
    return !history_.empty();
}

std::size_t Position::historySize() const {
    return history_.size();
}

bool Position::isInCheck(Color color) const {
    if (color == Color::None) {
        return false;
    }
    Square kingSquare = color == Color::White ? whiteKingSquare_ : blackKingSquare_;
    if (kingSquare.isValid()) {
        return MoveGenerator::isSquareAttacked(*this, kingSquare, opposite(color));
    }
    return false;
}

bool Position::isCheckmate() const {
    return termination() == GameTermination::Checkmate;
}

bool Position::isStalemate() const {
    return termination() == GameTermination::Stalemate;
}

bool Position::hasInsufficientMaterial() const {
    // La règle implémentée couvre les cas certains : rois seuls, ou un unique
    // fou/cavalier face à un roi. Les cas plus subtils de fous sont laissés
    // jouables afin d’éviter de déclarer abusivement une nulle.
    int nonKingPieces = 0;
    for (const Color color : {Color::White, Color::Black}) {
        nonKingPieces += pieceCount(color, PieceType::Queen);
        nonKingPieces += pieceCount(color, PieceType::Rook);
        nonKingPieces += pieceCount(color, PieceType::Bishop);
        nonKingPieces += pieceCount(color, PieceType::Knight);
        nonKingPieces += pieceCount(color, PieceType::Pawn);
    }

    if (nonKingPieces == 0) {
        return true;
    }
    if (nonKingPieces != 1) {
        return false;
    }
    const int minorPieces =
        pieceCount(Color::White, PieceType::Bishop)
        + pieceCount(Color::Black, PieceType::Bishop)
        + pieceCount(Color::White, PieceType::Knight)
        + pieceCount(Color::Black, PieceType::Knight);
    return minorPieces == 1;
}

bool Position::isDraw() const {
    const GameTermination state = termination();
    return state == GameTermination::Stalemate
        || state == GameTermination::RuleDraw;
}

bool Position::isRuleDraw() const {
    // Cent demi-coups correspondent aux cinquante coups complets prévus par la
    // règle, sans mouvement de pion ni capture.
    return halfmoveClock_ >= 100 || hasInsufficientMaterial() || isThreefoldRepetition();
}

bool Position::isThreefoldRepetition() const {
    int occurrences = 1;
    // Une capture ou un mouvement de pion rend toutes les positions plus
    // anciennes inaccessibles ; le compteur de demi-coups borne donc le scan.
    const std::size_t reversiblePlies = std::min(
        history_.size(),
        static_cast<std::size_t>(std::max(halfmoveClock_, 0)));
    for (std::size_t offset = 0; offset < reversiblePlies; ++offset) {
        const GameState& state = history_[history_.size() - 1 - offset];
        if (state.oldZobristHash == zobristHash_) {
            ++occurrences;
            if (occurrences >= 3) {
                return true;
            }
        }
    }
    return false;
}

GameTermination Position::termination() const {
    // Mat et pat doivent être testés avant les règles de nulle : une position
    // sans coup légal ne peut pas être qualifiée seulement par son compteur.
    const std::vector<Move> moves = legalMoves();
    if (moves.empty()) {
        return isInCheck(sideToMove_)
            ? GameTermination::Checkmate
            : GameTermination::Stalemate;
    }
    return isRuleDraw()
        ? GameTermination::RuleDraw
        : GameTermination::Ongoing;
}

std::vector<Move> Position::legalMoves() {
    return MoveGenerator::legalMoves(*this);
}

std::vector<Move> Position::legalMoves() const {
    return MoveGenerator::legalMoves(*this);
}

std::optional<Move> Position::findLegalMove(const Move& requested) const {
    for (const Move& move : legalMoves()) {
        if (move.sameUci(requested)) {
            return move;
        }
    }
    return std::nullopt;
}

std::optional<Move> Position::findLegalMove(const std::string& uci) const {
    return findLegalMove(Move::fromUci(uci));
}

Position Position::copyWithoutHistory() const {
    Position copy = *this;
    copy.history_.clear();
    return copy;
}

std::uint64_t Position::zobristHash() const {
    return zobristHash_;
}

std::string Position::toFen() const {
    return Fen::toFen(*this);
}

Position Position::fromFen(const std::string& fen) {
    return Fen::fromFen(fen);
}

} // namespace chess
