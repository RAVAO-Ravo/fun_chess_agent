/**
 * @file main.cpp
 * @brief Convertir le corpus lichess-org/chess-openings en ressources internes.
 *
 * Les lignes SAN sont rejouées sur un vrai plateau afin de lever les
 * ambiguïtés, produire une bibliothèque UCI vérifiée et extraire des positions
 * FEN diversifiées pour l’entraînement.
 */

#include "chess/position.hpp"
#include "chess/move.hpp"
#include "chess/piece.hpp"
#include "util/cli_options.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct ConvertResult {
    std::vector<std::string> moves;
    chess::Position finalBoard;
};

std::string trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::vector<std::string> splitTsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (start <= line.size()) {
        std::size_t tab = line.find('\t', start);
        if (tab == std::string::npos) {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
    return fields;
}

bool isResultToken(const std::string& token) {
    return token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*";
}

std::string pgnMoveToken(std::string token) {
    if (isResultToken(token)) {
        return "";
    }
    // Un token peut contenir son numéro de coup, par exemple ``12...Nf6`` ;
    // seule la partie située après le dernier point décrit le déplacement.
    const std::size_t dot = token.find_last_of('.');
    if (dot != std::string::npos) {
        token = token.substr(dot + 1);
    }
    return trim(token);
}

chess::PieceType pieceTypeFromSan(char c) {
    switch (c) {
    case 'K':
        return chess::PieceType::King;
    case 'Q':
        return chess::PieceType::Queen;
    case 'R':
        return chess::PieceType::Rook;
    case 'B':
        return chess::PieceType::Bishop;
    case 'N':
        return chess::PieceType::Knight;
    default:
        return chess::PieceType::Pawn;
    }
}

bool isSanPieceLetter(char c) {
    return c == 'K' || c == 'Q' || c == 'R' || c == 'B' || c == 'N';
}

std::string stripSanSuffixes(std::string san) {
    san = trim(san);
    // Échec, mat et annotations n’influencent pas l’identification du coup,
    // puisque sa légalité sera vérifiée directement sur le plateau.
    while (!san.empty()) {
        char last = san.back();
        if (last == '+' || last == '#' || last == '!' || last == '?') {
            san.pop_back();
            continue;
        }
        break;
    }
    return san;
}

std::optional<chess::Move> findCastleMove(const chess::Position& board, bool kingSide) {
    for (const chess::Move& move : board.legalMoves()) {
        if (move.isCastle() && ((kingSide && move.to().col() == 6) || (!kingSide && move.to().col() == 2))) {
            return move;
        }
    }
    return std::nullopt;
}

bool matchesDisambiguation(chess::Square from, const std::string& disambiguation) {
    for (char c : disambiguation) {
        if (c >= 'a' && c <= 'h' && from.col() != c - 'a') {
            return false;
        }
        if (c >= '1' && c <= '8' && from.row() != 8 - (c - '0')) {
            return false;
        }
    }
    return true;
}

std::optional<chess::Move> parseSanMove(const chess::Position& board, std::string san) {
    san = stripSanSuffixes(san);
    if (san == "O-O" || san == "0-0") {
        return findCastleMove(board, true);
    }
    if (san == "O-O-O" || san == "0-0-0") {
        return findCastleMove(board, false);
    }

    // === Décomposition de la notation SAN ===

    chess::PieceType promotion = chess::PieceType::None;
    const std::size_t promotionMarker = san.find('=');
    if (promotionMarker != std::string::npos) {
        if (promotionMarker + 1 >= san.size()) {
            return std::nullopt;
        }
        promotion = chess::promotionTypeFromChar(san[promotionMarker + 1]);
        san = san.substr(0, promotionMarker);
    }

    san.erase(std::remove(san.begin(), san.end(), 'x'), san.end());
    if (san.size() < 2) {
        return std::nullopt;
    }

    const std::string targetText = san.substr(san.size() - 2);
    chess::Square target;
    try {
        target = chess::Square::fromAlgebraic(targetText);
    } catch (const std::exception&) {
        return std::nullopt;
    }

    chess::PieceType pieceType = chess::PieceType::Pawn;
    std::size_t start = 0;
    if (isSanPieceLetter(san.front())) {
        pieceType = pieceTypeFromSan(san.front());
        start = 1;
    }
    const std::string disambiguation = san.substr(start, san.size() - start - 2);

    // === Résolution par la liste légale ===

    // Cette stratégie délègue rois en échec, clouages et prises en passant au
    // moteur au lieu de reproduire partiellement les règles dans le convertisseur.
    std::optional<chess::Move> match;
    for (const chess::Move& move : board.legalMoves()) {
        chess::Piece piece = board.pieceAt(move.from());
        if (piece.type() != pieceType || move.to() != target) {
            continue;
        }
        if (move.promotion() != promotion) {
            continue;
        }
        if (!matchesDisambiguation(move.from(), disambiguation)) {
            continue;
        }
        if (match.has_value()) {
            // Deux candidats signifient que la désambiguïsation SAN reçue est
            // insuffisante ou invalide ; choisir arbitrairement corromprait la ligne.
            return std::nullopt;
        }
        match = move;
    }
    return match;
}

ConvertResult convertPgnToUci(const std::string& pgn) {
    chess::Position board;
    ConvertResult result;
    std::istringstream stream(pgn);
    std::string rawToken;
    while (stream >> rawToken) {
        std::string token = pgnMoveToken(rawToken);
        if (token.empty()) {
            continue;
        }
        std::optional<chess::Move> move = parseSanMove(board, token);
        if (!move.has_value()) {
            throw std::runtime_error("cannot parse SAN move: " + token + " in " + pgn);
        }
        // Rejouer immédiatement le coup fournit le contexte exact du token suivant.
        result.moves.push_back(move->toUci());
        board.makeMove(*move);
    }
    result.finalBoard = board;
    return result;
}

std::vector<std::filesystem::path> inputFiles(const std::filesystem::path& inputDir) {
    std::vector<std::filesystem::path> files;
    for (const char name : {'a', 'b', 'c', 'd', 'e'}) {
        files.push_back(inputDir / (std::string(1, name) + ".tsv"));
    }
    return files;
}

void ensureParentDirectory(const std::filesystem::path& path) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        // === Préparation des entrées et sorties ===

        const util::CliOptions options(argc, argv);
        const std::filesystem::path inputDir =
            options.value("--input-dir", "data/openings/source");
        const std::filesystem::path bookPath =
            options.value("--output", "data/openings/generated/book.txt");
        const std::filesystem::path trainingPath = options.value(
            "--training-output",
            "data/openings/generated/training_positions.fen");
        const int trainingMinPlies = options.integer("--training-min-plies", 6);
        const bool strict = options.has("--strict");

        ensureParentDirectory(bookPath);
        ensureParentDirectory(trainingPath);
        std::ofstream bookOutput(bookPath);
        std::ofstream trainingOutput(trainingPath);
        if (!bookOutput) {
            throw std::runtime_error("cannot write opening book to " + bookPath.string());
        }
        if (!trainingOutput) {
            throw std::runtime_error("cannot write training positions to " + trainingPath.string());
        }

        bookOutput << "# Generated from lichess-org/chess-openings source TSV files (CC0-1.0).\n";
        bookOutput << "# Each non-comment line is one theoretical line in UCI notation.\n\n";
        trainingOutput << "# Generated from lichess-org/chess-openings source TSV files (CC0-1.0).\n";
        trainingOutput << "# Final positions from lines with at least " << trainingMinPlies << " plies.\n\n";

        int totalLines = 0;
        int convertedLines = 0;
        int skippedLines = 0;
        std::unordered_set<std::string> trainingPositions;

        // === Conversion des cinq volumes du corpus ===

        for (const std::filesystem::path& file : inputFiles(inputDir)) {
            std::ifstream input(file);
            if (!input) {
                throw std::runtime_error("cannot read " + file.string());
            }

            std::string line;
            bool header = true;
            int lineNumber = 0;
            while (std::getline(input, line)) {
                ++lineNumber;
                if (header) {
                    header = false;
                    continue;
                }
                if (trim(line).empty()) {
                    continue;
                }
                ++totalLines;

                std::vector<std::string> fields = splitTsvLine(line);
                if (fields.size() < 3) {
                    ++skippedLines;
                    continue;
                }

                try {
                    ConvertResult converted = convertPgnToUci(fields[2]);
                    if (converted.moves.empty()) {
                        ++skippedLines;
                        continue;
                    }

                    for (std::size_t index = 0; index < converted.moves.size(); ++index) {
                        if (index != 0) {
                            bookOutput << ' ';
                        }
                        bookOutput << converted.moves[index];
                    }
                    bookOutput << '\n';
                    ++convertedLines;

                    // Les positions trop précoces apportent peu de diversité.
                    // Les états terminaux ne constituent pas des débuts jouables.
                    if (static_cast<int>(converted.moves.size()) >= trainingMinPlies
                        && !converted.finalBoard.isCheckmate()
                        && !converted.finalBoard.isDraw()) {
                        const std::string fen = converted.finalBoard.toFen();
                        if (trainingPositions.insert(fen).second) {
                            trainingOutput << fen << '\n';
                        }
                    }
                } catch (const std::exception& error) {
                    // Le mode normal privilégie le rendement sur un grand corpus ;
                    // le mode strict sert à l’audit et s’arrête au premier défaut.
                    ++skippedLines;
                    std::cerr << file << ':' << lineNumber << ": " << error.what() << '\n';
                    if (strict) {
                        throw;
                    }
                }
            }
        }

        std::cout << "source_lines " << totalLines << '\n';
        std::cout << "book_lines " << convertedLines << '\n';
        std::cout << "training_positions " << trainingPositions.size() << '\n';
        std::cout << "skipped_lines " << skippedLines << '\n';
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
