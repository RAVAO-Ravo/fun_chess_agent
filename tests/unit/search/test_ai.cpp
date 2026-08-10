/**
 * @file test_ai.cpp
 * @brief Vérifier la légalité et la stabilité des choix du moteur.
 */

#include "test_util.hpp"

#include "search/chess_ai.hpp"
#include "search/evaluation/parameters.hpp"
#include "search/evaluation/evaluator.hpp"
#include "search/searcher.hpp"
#include "chess/position.hpp"

#include <filesystem>
#include <chrono>

namespace {

bool isLegalMove(const chess::Position& board, const chess::Move& move) {
    return board.findLegalMove(move).has_value();
}

} // namespace

void test_ai(TestSuite& suite) {
    chess::Position board;
    ai::EvaluationParameters parameters = ai::defaultEvaluationParameters();
    parameters.searchDepth = 1;
    ai::ChessAI ai(parameters);
    chess::Move move = ai.chooseMove(board);
    REQUIRE(suite, isLegalMove(board, move));

    chess::Position mateInOne = chess::Position::fromFen("7k/5K2/6Q1/8/8/8/8/8 w - - 0 1");
    parameters.searchDepth = 2;
    ai::ChessAI mateAi(parameters);
    chess::Move mateMove = mateAi.chooseMove(mateInOne);
    REQUIRE(suite, isLegalMove(mateInOne, mateMove));
    for (const chess::Move& legalMove : mateInOne.legalMoves()) {
        if (legalMove.sameUci(mateMove)) {
            mateInOne.makeMove(legalMove);
            break;
        }
    }
    REQUIRE(suite, mateInOne.isCheckmate());

    chess::Position aggressiveInstinct = chess::Position::fromFen("7k/5K2/6Q1/8/8/8/8/8 w - - 0 1");
    parameters.searchMode = ai::SearchMode::Instinct;
    parameters.searchDepth = 2;
    ai::ChessAI instinctAi(parameters);
    chess::Move aggressiveInstinctMove =
        instinctAi.chooseMove(aggressiveInstinct);
    REQUIRE(suite, isLegalMove(aggressiveInstinct, aggressiveInstinctMove));

    chess::Position lmrMate =
        chess::Position::fromFen("7k/5K2/6Q1/8/8/8/8/8 w - - 0 1");
    parameters.searchMode = ai::SearchMode::InstinctLmr;
    parameters.searchDepth = 5;
    parameters.lmrMinPly = 0;
    parameters.lmrFullDepthMoves = 3;
    ai::ChessAI lmrAi(parameters);
    const chess::Move lmrMateMove = lmrAi.chooseMove(lmrMate);
    const std::optional<chess::Move> legalLmrMate =
        lmrMate.findLegalMove(lmrMateMove);
    REQUIRE(suite, legalLmrMate.has_value());
    lmrMate.makeMove(*legalLmrMate);
    REQUIRE(suite, lmrMate.isCheckmate());

    chess::Position material = chess::Position::fromFen("4k3/8/8/8/8/8/8/3QK3 w - - 0 1");
    REQUIRE(suite, ai::Evaluator(parameters).evaluate(material) > 800);

    std::filesystem::create_directories("/tmp/probcomp_eval_smoke");
    std::string path = "/tmp/probcomp_eval_smoke/params.json";
    parameters.queenValue = 950;
    parameters.mobilityBonus = 6;
    parameters.passedPawnBonus = 77;
    parameters.searchMode = ai::SearchMode::InstinctLmr;
    parameters.lmrMinPly = 12;
    parameters.lmrFullDepthMoves = 0;
    parameters.searchDepth = 12;
    ai::saveParametersToJson(parameters, path);
    ai::EvaluationParameters loaded = ai::loadParametersFromJson(path);
    REQUIRE_EQ(suite, loaded.queenValue, 950);
    REQUIRE_EQ(suite, loaded.mobilityBonus, 6);
    REQUIRE_EQ(suite, loaded.passedPawnBonus, 77);
    REQUIRE_EQ(suite, loaded.searchMode, ai::SearchMode::InstinctLmr);
    REQUIRE_EQ(suite, loaded.lmrMinPly, ai::MaxLmrMinPly);
    REQUIRE_EQ(suite, loaded.lmrFullDepthMoves, ai::MinLmrFullDepthMoves);
    REQUIRE_EQ(suite, loaded.searchDepth, ai::MaxSearchDepth);

    ai::EvaluationParameters searchParameters = ai::defaultEvaluationParameters();
    searchParameters.searchDepth = 3;
    ai::Searcher searcher(searchParameters);
    ai::SearchLimits limits;
    limits.maxDepth = 3;
    const ai::SearchResult iterative = searcher.search(chess::Position{}, limits);
    REQUIRE(suite, isLegalMove(chess::Position{}, iterative.bestMove));
    REQUIRE_EQ(suite, iterative.stats.completedDepth, 3);
    REQUIRE(suite, iterative.stats.nodes > 0);
    REQUIRE(suite, iterative.stats.quiescenceNodes > 0);
    REQUIRE(suite, iterative.stats.transpositionProbes > 0);
    REQUIRE(suite, iterative.stats.transpositionHits > 0);
    REQUIRE(suite, iterative.stats.elapsed.count() > 0);
    REQUIRE(suite, iterative.stats.nodesPerSecond() > 0.0);

    ai::SearchLimits timedLimits;
    timedLimits.maxDepth = 10;
    timedLimits.timeLimit = std::chrono::milliseconds(1);
    const ai::SearchResult timed = searcher.search(chess::Position{}, timedLimits);
    REQUIRE(suite, isLegalMove(chess::Position{}, timed.bestMove));
    REQUIRE(suite, timed.stoppedByTime);
    REQUIRE(suite, timed.stats.completedDepth < timedLimits.maxDepth);

    const chess::Position stalemate =
        chess::Position::fromFen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    limits.maxDepth = 1;
    const ai::SearchResult stalemateResult = searcher.search(stalemate, limits);
    REQUIRE_EQ(suite, stalemateResult.score, 0);
    REQUIRE(suite, !stalemateResult.bestMove.from().isValid());

    const chess::Position poisonedPawn =
        chess::Position::fromFen("3rk3/8/8/3p4/8/8/8/3QK3 w - - 0 1");
    const ai::SearchResult horizonResult = searcher.search(poisonedPawn, limits);
    REQUIRE(suite, isLegalMove(poisonedPawn, horizonResult.bestMove));
    REQUIRE(suite, horizonResult.bestMove.toUci() != "d1d5");

    ai::EvaluationParameters exactParameters = ai::defaultEvaluationParameters();
    exactParameters.searchMode = ai::SearchMode::Instinct;
    ai::SearchLimits referenceLimits;
    referenceLimits.maxDepth = 4;
    referenceLimits.usePrincipalVariationSearch = false;
    referenceLimits.useAspirationWindows = false;
    ai::SearchLimits optimizedLimits = referenceLimits;
    optimizedLimits.usePrincipalVariationSearch = true;
    optimizedLimits.useAspirationWindows = true;
    for (const chess::Position& exactPosition : {
             chess::Position{},
             poisonedPawn,
             chess::Position::fromFen(
                 "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/"
                 "2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"),
         }) {
        ai::Searcher referenceSearcher(exactParameters);
        ai::Searcher optimizedSearcher(exactParameters);
        const ai::SearchResult reference =
            referenceSearcher.search(exactPosition, referenceLimits);
        const ai::SearchResult optimized =
            optimizedSearcher.search(exactPosition, optimizedLimits);
        REQUIRE_EQ(suite, optimized.score, reference.score);
        REQUIRE_EQ(
            suite,
            optimized.bestMove.toUci(),
            reference.bestMove.toUci());
        REQUIRE_EQ(
            suite,
            optimized.stats.completedDepth,
            referenceLimits.maxDepth);
    }

    ai::EvaluationParameters lmrParameters = exactParameters;
    lmrParameters.searchMode = ai::SearchMode::InstinctLmr;
    lmrParameters.lmrMinPly = 3;
    lmrParameters.lmrFullDepthMoves = 4;
    ai::SearchLimits lmrControlLimits;
    lmrControlLimits.maxDepth = 6;
    for (const chess::Position& controlPosition : {
             chess::Position{},
             poisonedPawn,
             chess::Position::fromFen(
                 "4k3/8/8/8/8/8/4r3/4K3 w - - 0 1"),
             chess::Position::fromFen(
                 "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/"
                 "2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"),
             chess::Position::fromFen(
                 "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"),
         }) {
        ai::Searcher fullSearcher(exactParameters);
        ai::Searcher lmrSearcher(lmrParameters);
        const ai::SearchResult full =
            fullSearcher.search(controlPosition, lmrControlLimits);
        const ai::SearchResult reduced =
            lmrSearcher.search(controlPosition, lmrControlLimits);
        REQUIRE_EQ(
            suite,
            reduced.bestMove.toUci(),
            full.bestMove.toUci());
        REQUIRE_EQ(
            suite,
            reduced.stats.completedDepth,
            lmrControlLimits.maxDepth);
    }
}
