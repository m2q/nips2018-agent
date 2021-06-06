#ifndef FROM_JSON_H
#define FROM_JSON_H

#include "bboard.hpp"

using namespace bboard;

/**
 * @brief StateFromJSON Converts a state string from python to a state.
 * @param state The state reference used to save the state
 * @param json The state string (in json)
 * @param gameMode The current gamemode (not included in json)
 */
void StateFromJSON(State& state, const std::string& json, GameMode gameMode);

/**
 * @brief StateFromJSON Converts a state string from python to a state.
 * @param json The state string (in json)
 * @param gameMode The current gamemode (not included in json)
 * @return The state created from the string
 */
State StateFromJSON(const std::string& json, GameMode gameMode);

/**
 * @brief ObservationFromJSON Converts an observation string from python to an observation.
 * @param obs The observation reference used to save the observation
 * @param json The observation string (in json)
 * @param agentId The id of the agent which received this observation
 */
void ObservationFromJSON(Observation& obs, const std::string& json, int agentId);

/**
 * @brief ObservationFromJSON Converts an observation string from python to an observation.
 * @param json The observation string (in json)
 * @param agentId The id of the agent which received this observation
 * @return The observation created from the string
 */
Observation ObservationFromJSON(const std::string& json, int agentId);

#endif // FROM_JSON_H
