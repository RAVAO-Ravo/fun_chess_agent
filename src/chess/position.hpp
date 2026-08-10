/**
 * @file position.hpp
 * @brief Modéliser une position, son historique et ses états dérivés.
 *
 * La classe maintient incrémentalement les comptes de pièces, les cases des
 * rois et l’empreinte Zobrist afin d’éviter de rescanner le plateau pendant
 * la recherche.
 */

#pragma once

#include "chess/color.hpp"
#include "chess/game_state.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"
#include "chess/square.hpp"
#include "chess/termination.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chess {

/**
 * @class Position
 * @brief Porter l’état complet et réversible d’une partie d’échecs.
 *
 * Les données dérivées sont maintenues lors de chaque mutation. L’historique
 * contient les informations minimales nécessaires à ``undoMove`` ainsi que
 * les empreintes antérieures requises par la règle de triple répétition.
 */
class Position {
public:
    /** @brief Construire la position initiale réglementaire. */
    Position();

    /** @brief Restaurer la position initiale et vider l’historique. */
    void reset();
    /** @brief Construire un plateau vide aux compteurs cohérents. */
    void clear();

    /** @brief Lire une pièce, ou une pièce vide pour une case invalide. */
    Piece pieceAt(Square square) const;
    /** @brief Remplacer une pièce et actualiser tous les caches dérivés. */
    void setPiece(Square square, Piece piece);
    /** @brief Lire le nombre maintenu d’un type de pièce pour un camp. */
    int pieceCount(Color color, PieceType type) const;

    Color sideToMove() const;
    void setSideToMove(Color color);

    bool canCastleKingSide(Color color) const;
    bool canCastleQueenSide(Color color) const;
    void setCastlingRights(Color color, bool kingSide, bool queenSide);

    std::optional<Square> enPassantSquare() const;
    void setEnPassantSquare(std::optional<Square> square);

    int halfmoveClock() const;
    int fullmoveNumber() const;
    void setHalfmoveClock(int value);
    void setFullmoveNumber(int value);

    /**
     * @brief Appliquer un coup pseudo-légal et enregistrer son état antérieur.
     *
     * @param move (const Move&) : Coup déjà annoté par le générateur.
     *
     * @return bool : Vrai si l’origine et le camp au trait sont valides.
     */
    bool makeMove(const Move& move);
    /** @brief Restaurer exactement l’état précédant le dernier coup. */
    void undoMove();
    bool canUndo() const;
    std::size_t historySize() const;

    bool isInCheck(Color color) const;
    bool isCheckmate() const;
    bool isStalemate() const;
    bool isDraw() const;
    bool isRuleDraw() const;
    /** @brief Détecter trois empreintes identiques dans la séquence réversible. */
    bool isThreefoldRepetition() const;
    /** @brief Reconnaître roi contre roi ou roi et pièce mineure contre roi. */
    bool hasInsufficientMaterial() const;
    /** @brief Déterminer l’état terminal en évitant des tests contradictoires. */
    GameTermination termination() const;

    std::vector<Move> legalMoves();
    std::vector<Move> legalMoves() const;
    std::optional<Move> findLegalMove(const Move& requested) const;
    std::optional<Move> findLegalMove(const std::string& uci) const;
    /** @brief Copier l’état courant sans dupliquer sa séquence de coups. */
    Position copyWithoutHistory() const;

    std::uint64_t zobristHash() const;
    std::string toFen() const;
    static Position fromFen(const std::string& fen);

private:
    std::array<Piece, 64> squares_;
    std::array<std::array<int, 6>, 2> pieceCounts_{};
    Color sideToMove_ = Color::White;

    bool whiteCanCastleKingSide_ = true;
    bool whiteCanCastleQueenSide_ = true;
    bool blackCanCastleKingSide_ = true;
    bool blackCanCastleQueenSide_ = true;

    std::optional<Square> enPassantSquare_;
    int halfmoveClock_ = 0;
    int fullmoveNumber_ = 1;
    Square whiteKingSquare_;
    Square blackKingSquare_;
    std::uint64_t zobristHash_ = 0;

    std::vector<GameState> history_;
};

} // namespace chess
