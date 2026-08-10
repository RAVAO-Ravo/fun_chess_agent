/**
 * @file protocol.hpp
 * @brief Déclarer les conversions propres au protocole interactif.
 */

#pragma once

#include "chess/position.hpp"
#include "chess/color.hpp"

#include <string>

namespace app {

/** @brief Convertir un nom de couleur externe en valeur interne validée. */
chess::Color parseColor(const std::string& text);
/** @brief Décrire échec, mat, pat, nulle ou partie en cours. */
std::string protocolStatus(const chess::Position& board);

} // namespace app
