/**
 * @file opening_book.cpp
 * @brief Charger des lignes UCI et sélectionner un coup d’ouverture légal.
 */

#include "search/opening_book.hpp"

#include <algorithm>
#include <fstream>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace ai {

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

bool isMoveNumberToken(const std::string& token) {
    return token.find('.') != std::string::npos;
}

int maxWeight(const std::vector<BookMove>& moves) {
    int maximum = 0;
    for (const BookMove& move : moves) {
        maximum = std::max(maximum, move.weight);
    }
    return maximum;
}

} // namespace

OpeningBookMode parseOpeningBookMode(const std::string& text) {
    if (text == "chill" || text == "weighted" || text == "casual") {
        return OpeningBookMode::Chill;
    }
    if (text == "competition" || text == "competitive" || text == "best") {
        return OpeningBookMode::Competition;
    }
    throw std::invalid_argument("invalid opening book mode: " + text);
}

OpeningBook OpeningBook::loadFromFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot load opening book from " + path);
    }

    OpeningBook book;
    std::string line;
    int lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        book.addLine(line, lineNumber);
    }
    return book;
}

OpeningBook OpeningBook::fromLines(const std::vector<std::string>& lines) {
    OpeningBook book;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        book.addLine(lines[index], static_cast<int>(index + 1));
    }
    return book;
}

std::optional<chess::Move> OpeningBook::findMove(
    const chess::Position& board,
    OpeningBookMode mode,
    std::mt19937& rng) const {
    auto found = movesByPosition_.find(positionKey(board));
    if (found == movesByPosition_.end()) {
        return std::nullopt;
    }

    std::vector<BookMove> legalMoves;
    for (const BookMove& bookMove : found->second) {
        if (std::optional<chess::Move> legalMove = board.findLegalMove(bookMove.move)) {
            legalMoves.push_back(BookMove{*legalMove, bookMove.weight});
        }
    }
    if (legalMoves.empty()) {
        return std::nullopt;
    }

    if (mode == OpeningBookMode::Competition) {
        // Le mode compétition supprime l’aléa et reproduit toujours la ligne
        // la plus fréquente du corpus d’ouvertures.
        auto best = std::max_element(
            legalMoves.begin(),
            legalMoves.end(),
            [](const BookMove& a, const BookMove& b) {
                return a.weight < b.weight;
            });
        return best->move;
    }

    // En mode détendu, les variantes représentant moins de 5 % du meilleur
    // poids sont éliminées pour éviter des coups anecdotiques ou bruités.
    const int floor = std::max(1, maxWeight(legalMoves) / 20);
    legalMoves.erase(
        std::remove_if(
            legalMoves.begin(),
            legalMoves.end(),
            [floor](const BookMove& move) {
                return move.weight < floor;
            }),
        legalMoves.end());
    if (legalMoves.empty()) {
        return std::nullopt;
    }

    // Le tirage pondéré conserve la diversité tout en privilégiant les lignes
    // observées le plus souvent dans la bibliothèque.
    const int totalWeight = std::accumulate(
        legalMoves.begin(),
        legalMoves.end(),
        0,
        [](int total, const BookMove& move) {
            return total + std::max(1, move.weight);
        });
    std::uniform_int_distribution<int> distribution(1, totalWeight);
    int selected = distribution(rng);
    for (const BookMove& move : legalMoves) {
        selected -= std::max(1, move.weight);
        if (selected <= 0) {
            return move.move;
        }
    }
    return legalMoves.back().move;
}

void OpeningBook::addLine(const std::string& line, int lineNumber) {
    const std::string content = trim(stripComment(line));
    if (content.empty()) {
        return;
    }

    chess::Position board;
    std::istringstream stream(content);
    std::string token;
    while (stream >> token) {
        if (isMoveNumberToken(token)) {
            continue;
        }

        chess::Move requested;
        try {
            requested = chess::Move::fromUci(token);
        } catch (const std::exception&) {
            throw std::runtime_error("invalid opening book move on line " + std::to_string(lineNumber) + ": " + token);
        }

        // Rejouer la ligne valide chaque coup dans son contexte et construit
        // simultanément la clé de toutes les positions intermédiaires.
        std::optional<chess::Move> legalMove = board.findLegalMove(requested);
        if (!legalMove.has_value()) {
            throw std::runtime_error("illegal opening book move on line " + std::to_string(lineNumber) + ": " + token);
        }

        addMove(positionKey(board), *legalMove);
        board.makeMove(*legalMove);
    }
}

void OpeningBook::addMove(const std::string& key, const chess::Move& move) {
    std::vector<BookMove>& moves = movesByPosition_[key];
    auto duplicate = std::find_if(moves.begin(), moves.end(), [&move](const BookMove& existing) {
        return existing.move.sameUci(move);
    });
    if (duplicate == moves.end()) {
        moves.push_back(BookMove{move, 1});
    } else {
        // Le nombre d’occurrences devient le poids statistique du coup.
        ++duplicate->weight;
    }
}

std::string OpeningBook::positionKey(const chess::Position& board) {
    // Les compteurs de demi-coups et de coups complets ne changent pas les
    // coups disponibles ; les exclure regroupe les transpositions équivalentes.
    std::istringstream fen(board.toFen());
    std::string placement;
    std::string side;
    std::string castling;
    std::string enPassant;
    fen >> placement >> side >> castling >> enPassant;
    return placement + " " + side + " " + castling + " " + enPassant;
}

} // namespace ai
