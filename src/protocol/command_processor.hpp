/**
 * @file command_processor.hpp
 * @brief Déclarer l’interpréteur du protocole textuel interactif.
 */

#pragma once

#include "protocol/game_session.hpp"

#include <iosfwd>
#include <string>

namespace app {

/**
 * @class CommandProcessor
 * @brief Traduire une ligne externe en action atomique sur la session.
 *
 * Les réponses restent volontairement simples et stables afin que la GUI ou
 * un script puisse les lire ligne par ligne sans dépendance particulière.
 */
class CommandProcessor {
public:
    /** @brief Relier une session, un flux de sortie et le mode diagnostic. */
    CommandProcessor(GameSession& session, std::ostream& output, bool diagnostics = false);

    /** @brief Traiter une commande et indiquer si la boucle doit continuer. */
    bool processLine(const std::string& line);

private:
    void printPosition() const;
    void printLegalMoves() const;
    void printSearchStats() const;
    bool playFreeMove(const std::string& moveText);

    GameSession& session_;
    std::ostream& output_;
    bool diagnostics_ = false;
};

} // namespace app
