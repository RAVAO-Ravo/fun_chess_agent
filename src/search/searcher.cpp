/**
 * @file searcher.cpp
 * @brief Explorer l’arbre de jeu avec plusieurs optimisations complémentaires.
 *
 * Le pipeline associe approfondissement itératif, fenêtres d’aspiration,
 * Principal Variation Search, table de transposition, réductions tardives et
 * quiescence. Chaque heuristique réduit le travail sans modifier les règles.
 */

#include "search/searcher.hpp"

#include "chess/move_generator.hpp"
#include "chess/zobrist.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <optional>
#include <utility>

namespace ai {

namespace {

constexpr int Infinity = 1'000'000'000;
constexpr int MateScore = 100'000'000;
constexpr int MateThreshold = MateScore - MoveOrdering::MaxPly;
constexpr int InitialAspirationWindow = 50;

EvaluationParameters clampedParameters(EvaluationParameters parameters) {
    clampParameters(parameters);
    return parameters;
}

int scoreToTable(int score, int ply) {
    // Un mat doit conserver sa distance depuis la racine, même lorsqu’une
    // entrée est réutilisée à un autre niveau de l’arbre.
    if (score >= MateThreshold) {
        return score + ply;
    }
    if (score <= -MateThreshold) {
        return score - ply;
    }
    return score;
}

int scoreFromTable(int score, int ply) {
    // Rétablit le score relatif au niveau courant après lecture du cache.
    if (score >= MateThreshold) {
        return score - ply;
    }
    if (score <= -MateThreshold) {
        return score + ply;
    }
    return score;
}

std::vector<chess::Move> generateSearchMoves(
    chess::Position& position,
    chess::MoveGenerationMode mode = chess::MoveGenerationMode::All) {
    return chess::MoveGenerator::legalMoves(
        position,
        mode,
        chess::MoveAnnotationMode::Ordering);
}

} // namespace

Searcher::Searcher(EvaluationParameters parameters)
    : parameters_(clampedParameters(std::move(parameters)))
    , evaluator_(parameters_) {
}

SearchResult Searcher::search(const chess::Position& position, SearchLimits limits) {
    // === Initialisation de la recherche ===

    limits_ = limits;
    limits_.maxDepth = std::max(1, limits_.maxDepth);
    limits_.quiescenceMaxPly = std::max(1, limits_.quiescenceMaxPly);
    stats_ = {};
    stopped_ = false;
    startedAt_ = std::chrono::steady_clock::now();
    deadline_ = limits_.timeLimit.count() > 0
        ? startedAt_ + limits_.timeLimit
        : std::chrono::steady_clock::time_point::max();
    transpositionTable_.newSearch();
    moveOrdering_.clear();

    // La position fournie reste immuable pour l’appelant ; toute l’exploration
    // repose ensuite sur des paires makeMove/undoMove appliquées à cette copie.
    chess::Position board = position;
    std::vector<chess::Move> legalMoves = generateSearchMoves(board);
    SearchResult result;
    if (legalMoves.empty()) {
        result.score = board.isInCheck(board.sideToMove()) ? -MateScore : 0;
        stats_.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - startedAt_);
        result.stats = stats_;
        return result;
    }

    moveOrdering_.order(
        board,
        legalMoves,
        std::nullopt,
        0,
        parameters_,
        usesInstinctOrdering());
    result.bestMove = legalMoves.front();

    chess::Move previousBest = result.bestMove;

    // === Approfondissement itératif ===

    // Chaque profondeur réutilise le meilleur coup précédent. Ce surcoût
    // améliore suffisamment l’ordre des coups pour réduire l’arbre total.
    for (int depth = 1; depth <= limits_.maxDepth; ++depth) {
        int alpha = -Infinity;
        int beta = Infinity;
        int aspirationWindow = InitialAspirationWindow;
        // Une fenêtre étroite autour du score précédent déclenche davantage
        // de coupures. Elle est désactivée avec LMR, combinaison trop instable
        // pour garantir ici un gain reproductible.
        if (limits_.useAspirationWindows
            && !usesLateMoveReductions()
            && stats_.completedDepth > 0
            && std::abs(result.score) < MateThreshold) {
            alpha = std::max(-Infinity, result.score - aspirationWindow);
            beta = std::min(Infinity, result.score + aspirationWindow);
        }

        std::pair<chess::Move, int> iterationResult;
        chess::Move iterationPreferred = previousBest;
        while (true) {
            iterationResult = searchRoot(
                board,
                depth,
                iterationPreferred,
                alpha,
                beta);
            if (stopped_) {
                break;
            }
            iterationPreferred = iterationResult.first;
            if (iterationResult.second <= alpha && alpha > -Infinity) {
                // Un échec bas signifie que la vraie valeur se trouve sous la
                // fenêtre : on l’élargit progressivement sans tout abandonner.
                alpha = std::max(-Infinity, alpha - aspirationWindow);
                aspirationWindow = std::min(
                    Infinity / 2,
                    aspirationWindow * 2);
                continue;
            }
            if (iterationResult.second >= beta && beta < Infinity) {
                // Symétriquement, un échec haut impose d’ouvrir la borne bêta.
                beta = std::min(Infinity, beta + aspirationWindow);
                aspirationWindow = std::min(
                    Infinity / 2,
                    aspirationWindow * 2);
                continue;
            }
            break;
        }
        if (stopped_) {
            break;
        }

        result.bestMove = iterationResult.first;
        result.score = iterationResult.second;
        previousBest = result.bestMove;
        stats_.completedDepth = depth;
    }

    // Seule la dernière itération complète est publiée : un arrêt au milieu
    // d’une profondeur ne doit pas remplacer un résultat entièrement vérifié.
    stats_.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - startedAt_);
    stats_.transpositionEntries =
        static_cast<std::uint64_t>(transpositionTable_.size());
    result.stats = stats_;
    result.stoppedByTime = stopped_;
    return result;
}

std::pair<chess::Move, int> Searcher::searchRoot(
    chess::Position& position,
    int depth,
    const chess::Move& previousBest,
    int alpha,
    int beta) {
    std::vector<chess::Move> moves = generateSearchMoves(position);
    std::optional<chess::Move> preferredMove;
    if (previousBest.from().isValid()) {
        preferredMove = previousBest;
    }
    moveOrdering_.order(
        position,
        moves,
        preferredMove,
        0,
        parameters_,
        usesInstinctOrdering());

    chess::Move bestMove = moves.front();
    int bestScore = -Infinity;
    const int originalAlpha = alpha;
    const int originalBeta = beta;
    for (std::size_t moveIndex = 0; moveIndex < moves.size(); ++moveIndex) {
        const chess::Move& move = moves[moveIndex];
        if (shouldStop()) {
            break;
        }
        position.makeMove(move);
        int score;
        if (!limits_.usePrincipalVariationSearch || moveIndex == 0) {
            // Le premier coup, supposé meilleur grâce au classement, établit
            // une borne fiable avec une fenêtre complète.
            score = -negamax(position, depth - 1, -beta, -alpha, 1);
        } else {
            // PVS teste les autres coups avec une fenêtre nulle. Seul un coup
            // qui améliore réellement alpha mérite une recherche complète.
            score = -negamax(position, depth - 1, -alpha - 1, -alpha, 1);
            if (!stopped_ && score > alpha && score < beta) {
                score = -negamax(position, depth - 1, -beta, -alpha, 1);
            }
        }
        position.undoMove();
        if (stopped_) {
            break;
        }
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            ++stats_.betaCutoffs;
            break;
        }
    }
    if (!stopped_) {
        // La nature de la borne permet une réutilisation correcte même lorsque
        // la fenêtre alpha-bêta n’a pas produit de score exact.
        BoundType bound = BoundType::Exact;
        if (bestScore <= originalAlpha) {
            bound = BoundType::Upper;
        } else if (bestScore >= originalBeta) {
            bound = BoundType::Lower;
        }
        transpositionTable_.store(
            position.zobristHash()
                ^ chess::Zobrist::halfmoveClockKey(position.halfmoveClock()),
            TranspositionEntry{depth, scoreToTable(bestScore, 0), bound, bestMove});
    }
    return {bestMove, bestScore};
}

int Searcher::negamax(
    chess::Position& position,
    int depth,
    int alpha,
    int beta,
    int ply) {
    if (shouldStop()) {
        return 0;
    }
    const bool inCheck = position.isInCheck(position.sideToMove());
    if (depth <= 0) {
        return quiescence(position, alpha, beta, ply, 0);
    }
    if (position.isRuleDraw()) {
        if (inCheck && !chess::MoveGenerator::hasAnyLegalMove(position)) {
            return -MateScore + ply;
        }
        return 0;
    }
    ++stats_.nodes;

    const std::uint64_t key =
        position.zobristHash()
        ^ chess::Zobrist::halfmoveClockKey(position.halfmoveClock());
    const int originalAlpha = alpha;
    const int originalBeta = beta;
    std::optional<chess::Move> preferredMove;
    ++stats_.transpositionProbes;
    if (const auto entry = transpositionTable_.lookup(key); entry.has_value()) {
        ++stats_.transpositionHits;
        preferredMove = entry->bestMove;
        if (entry->depth >= depth) {
            // Une entrée moins profonde reste utile pour ordonner son meilleur
            // coup, mais pas pour borner la valeur de cette recherche.
            const int tableScore = scoreFromTable(entry->score, ply);
            if (entry->bound == BoundType::Exact) {
                return tableScore;
            }
            if (entry->bound == BoundType::Lower) {
                alpha = std::max(alpha, tableScore);
            } else if (entry->bound == BoundType::Upper) {
                beta = std::min(beta, tableScore);
            }
            if (alpha >= beta) {
                return tableScore;
            }
        }
    }

    std::vector<chess::Move> moves = generateSearchMoves(position);
    if (moves.empty()) {
        return inCheck ? -MateScore + ply : 0;
    }
    moveOrdering_.order(
        position,
        moves,
        preferredMove,
        ply,
        parameters_,
        usesInstinctOrdering());

    int best = -Infinity;
    chess::Move bestMove = moves.front();
    for (std::size_t moveIndex = 0; moveIndex < moves.size(); ++moveIndex) {
        const chess::Move& move = moves[moveIndex];
        const chess::Color movingColor = position.sideToMove();
        const bool quiet = isQuietMove(move);
        const bool reductionCandidate =
            usesLateMoveReductions()
            && depth >= 3
            && ply >= parameters_.lmrMinPly
            && moveIndex >= static_cast<std::size_t>(parameters_.lmrFullDepthMoves)
            && quiet
            && !inCheck
            && !move.givesCheck();

        position.makeMove(move);
        const bool reduce = reductionCandidate;
        int score;
        if (!limits_.usePrincipalVariationSearch) {
            if (reduce) {
                // LMR réduit d’un niveau les coups calmes classés tard. Une
                // amélioration d’alpha annule ce pari et force la profondeur
                // normale afin de ne pas perdre un coup surprenant.
                score = -negamax(
                    position,
                    depth - 2,
                    -alpha - 1,
                    -alpha,
                    ply + 1);
                if (!stopped_ && score > alpha) {
                    score = -negamax(
                        position,
                        depth - 1,
                        -beta,
                        -alpha,
                        ply + 1);
                }
            } else {
                score = -negamax(
                    position,
                    depth - 1,
                    -beta,
                    -alpha,
                    ply + 1);
            }
        } else if (moveIndex == 0) {
            score = -negamax(position, depth - 1, -beta, -alpha, ply + 1);
        } else {
            if (reduce) {
                // Avec PVS, le coup tardif est d’abord réduit et testé dans une
                // fenêtre nulle, puis réexaminé par étapes s’il résiste.
                score = -negamax(
                    position,
                    depth - 2,
                    -alpha - 1,
                    -alpha,
                    ply + 1);
                if (!stopped_ && score > alpha) {
                    score = -negamax(
                        position,
                        depth - 1,
                        -alpha - 1,
                        -alpha,
                        ply + 1);
                }
            } else {
                score = -negamax(
                    position,
                    depth - 1,
                    -alpha - 1,
                    -alpha,
                    ply + 1);
            }
            if (!stopped_ && score > alpha && score < beta) {
                score = -negamax(
                    position,
                    depth - 1,
                    -beta,
                    -alpha,
                    ply + 1);
            }
        }
        position.undoMove();
        if (stopped_) {
            return 0;
        }

        if (score > best) {
            best = score;
            bestMove = move;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            ++stats_.betaCutoffs;
            if (quiet) {
                // Les coups calmes responsables d’une coupure deviennent des
                // candidats prioritaires dans les positions voisines.
                moveOrdering_.recordQuietCutoff(move, movingColor, ply, depth);
            }
            break;
        }
    }

    BoundType bound = BoundType::Exact;
    if (best <= originalAlpha) {
        bound = BoundType::Upper;
    } else if (best >= originalBeta) {
        bound = BoundType::Lower;
    }
    transpositionTable_.store(
        key,
        TranspositionEntry{depth, scoreToTable(best, ply), bound, bestMove});
    return best;
}

int Searcher::quiescence(
    chess::Position& position,
    int alpha,
    int beta,
    int ply,
    int quiescencePly) {
    if (shouldStop()) {
        return 0;
    }
    ++stats_.quiescenceNodes;

    const bool inCheck = position.isInCheck(position.sideToMove());
    if (position.isRuleDraw()) {
        if (inCheck && !chess::MoveGenerator::hasAnyLegalMove(position)) {
            return -MateScore + ply;
        }
        return 0;
    }

    int best = -Infinity;
    std::vector<chess::Move> legalMoves;
    if (!inCheck) {
        if (!chess::MoveGenerator::hasAnyLegalMove(position)) {
            return 0;
        }
        const int standPat = evaluateForSideToMove(position);
        // Stand pat représente le choix implicite de ne pas poursuivre un
        // échange. Il est interdit en échec, où une réponse légale est forcée.
        best = standPat;
        if (standPat >= beta) {
            return standPat;
        }
        alpha = std::max(alpha, standPat);
    } else {
        legalMoves = generateSearchMoves(position);
        if (legalMoves.empty()) {
            return -MateScore + ply;
        }
    }
    if (quiescencePly >= limits_.quiescenceMaxPly) {
        // La borne évite qu’une suite artificielle de captures ne monopolise
        // indéfiniment la recherche dans une position pathologique.
        return best == -Infinity ? evaluateForSideToMove(position) : best;
    }

    if (!inCheck) {
        // Hors échec, seuls les coups tactiques peuvent invalider brutalement
        // l’évaluation statique située à la frontière de profondeur.
        legalMoves = generateSearchMoves(
            position,
            chess::MoveGenerationMode::Tactical);
    }
    if (legalMoves.empty()) {
        return best;
    }
    moveOrdering_.order(
        position,
        legalMoves,
        std::nullopt,
        ply,
        parameters_,
        usesInstinctOrdering());

    for (const chess::Move& move : legalMoves) {
        position.makeMove(move);
        const int score = -quiescence(
            position,
            -beta,
            -alpha,
            ply + 1,
            quiescencePly + 1);
        position.undoMove();
        if (stopped_) {
            return 0;
        }
        best = std::max(best, score);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            ++stats_.betaCutoffs;
            break;
        }
    }
    return best;
}

int Searcher::evaluateForSideToMove(const chess::Position& position) const {
    const int score = evaluator_.evaluate(position);
    return position.sideToMove() == chess::Color::White ? score : -score;
}

bool Searcher::shouldStop() {
    if (stopped_) {
        return true;
    }
    if (limits_.timeLimit.count() > 0 && std::chrono::steady_clock::now() >= deadline_) {
        // Le drapeau rend les appels suivants constants et propage rapidement
        // l’arrêt à travers toutes les couches récursives.
        stopped_ = true;
    }
    return stopped_;
}

bool Searcher::usesInstinctOrdering() const {
    return parameters_.searchMode != SearchMode::Classic;
}

bool Searcher::usesLateMoveReductions() const {
    return parameters_.searchMode == SearchMode::InstinctLmr;
}

} // namespace ai
