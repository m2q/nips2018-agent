#include "catch.hpp"
#include "bboard.hpp"
#include "from_json.hpp"

// some state and the corresponding observation
// note: the step count does not match because self._step_count is incremented AFTER the observations for the step are generated (bug in python env)
#define JSON_STATE "{\"board_size\": 11, \"step_count\": 30, \"board\": [[0, 0, 1, 1, 1, 1, 1, 1, 4, 4, 4], [0, 0, 0, 3, 2, 2, 2, 3, 13, 4, 7], [1, 0, 0, 1, 2, 1, 0, 2, 1, 0, 1], [1, 10, 1, 0, 0, 2, 2, 1, 0, 0, 1], [1, 6, 2, 0, 0, 0, 1, 0, 0, 2, 1], [1, 2, 1, 2, 0, 0, 0, 2, 0, 2, 2], [1, 2, 0, 2, 1, 0, 0, 2, 0, 2, 1], [1, 0, 2, 1, 0, 2, 2, 0, 1, 0, 1], [0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1], [3, 0, 0, 0, 11, 2, 2, 0, 0, 0, 6], [2, 0, 1, 1, 1, 2, 1, 1, 1, 6, 0]], \"agents\": [{\"agent_id\": 0, \"is_alive\": true, \"position\": [3, 1], \"ammo\": 0, \"blast_strength\": 2, \"can_kick\": false}, {\"agent_id\": 1, \"is_alive\": true, \"position\": [9, 4], \"ammo\": 0, \"blast_strength\": 2, \"can_kick\": true}, {\"agent_id\": 2, \"is_alive\": false, \"position\": [9, 8], \"ammo\": 1, \"blast_strength\": 2, \"can_kick\": false}, {\"agent_id\": 3, \"is_alive\": true, \"position\": [1, 8], \"ammo\": 0, \"blast_strength\": 2, \"can_kick\": true}], \"bombs\": [{\"position\": [9, 0], \"bomber_id\": 1, \"life\": 3, \"blast_strength\": 2, \"moving_direction\": null}, {\"position\": [1, 3], \"bomber_id\": 0, \"life\": 5, \"blast_strength\": 2, \"moving_direction\": null}, {\"position\": [1, 7], \"bomber_id\": 3, \"life\": 8, \"blast_strength\": 2, \"moving_direction\": null}], \"flames\": [{\"position\": [0, 8], \"life\": 0}, {\"position\": [0, 9], \"life\": 0}, {\"position\": [0, 10], \"life\": 0}, {\"position\": [1, 9], \"life\": 0}], \"items\": [[[6, 9], 6], [[10, 5], 6], [[7, 5], 7], [[6, 1], 7], [[1, 4], 8], [[1, 5], 6], [[9, 5], 7], [[3, 5], 6], [[1, 6], 8], [[5, 9], 6], [[6, 3], 8], [[2, 4], 7], [[9, 6], 8], [[6, 7], 8]], \"intended_actions\": [2, 0, 0, 4]}"
#define JSON_OBS "{\"alive\": [10, 11, 13], \"board\": [[0, 0, 1, 1, 1, 1, 1, 1, 4, 4, 4], [0, 0, 0, 3, 2, 2, 2, 3, 13, 4, 7], [1, 0, 0, 1, 2, 1, 0, 2, 1, 0, 1], [1, 10, 1, 0, 0, 2, 2, 1, 0, 0, 1], [1, 6, 2, 0, 0, 0, 1, 0, 0, 2, 1], [1, 2, 1, 2, 0, 0, 0, 2, 0, 2, 2], [1, 2, 0, 2, 1, 0, 0, 2, 0, 2, 1], [1, 0, 2, 1, 0, 2, 2, 0, 1, 0, 1], [0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1], [3, 0, 0, 0, 11, 2, 2, 0, 0, 0, 6], [2, 0, 1, 1, 1, 2, 1, 1, 1, 6, 0]], \"bomb_blast_strength\": [[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]], \"bomb_life\": [[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 5.0, 0.0, 0.0, 0.0, 8.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [3.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]], \"bomb_moving_direction\": [[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]], \"flame_life\": [[0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0], [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]], \"game_type\": 1, \"game_env\": \"pommerman.envs.v0:Pomme\", \"position\": [1, 8], \"blast_strength\": 2, \"can_kick\": true, \"teammate\": 9, \"ammo\": 0, \"enemies\": [10, 11, 12], \"step_count\": 29}"

TEST_CASE("Load State", "[json]")
{
    std::string jState = JSON_STATE;
    bboard::State s = StateFromJSON(jState, bboard::GameMode::FreeForAll);
    std::cout << std::endl << "Loaded state:" << std::endl;
    bboard::PrintState(&s);
}

TEST_CASE("Load Observation", "[json]")
{
    std::string jObs = JSON_OBS;
    bboard::Observation o = ObservationFromJSON(jObs, 0);
    std::cout << std::endl << "Loaded observation:" << std::endl;
    bboard::PrintObservation(&o);
}

TEST_CASE("Reconstruct State", "[json]")
{
    std::string jState = JSON_STATE;
    bboard::State realState = StateFromJSON(jState, bboard::GameMode::FreeForAll);

    std::string jObs = JSON_OBS;
    bboard::Observation o = ObservationFromJSON(jObs, 0);
    bboard::State reconstructedState;
    o.ToState(reconstructedState, GameMode::FreeForAll);
    std::cout << std::endl << "Reconstructed state:" << std::endl;
    bboard::PrintState(&reconstructedState);

    std::cout << realState.aliveAgents << std::endl;
    REQUIRE(reconstructedState.bombs.count == realState.bombs.count);
    REQUIRE(realState.aliveAgents == reconstructedState.aliveAgents);
    for(auto i = 0; i < bboard::AGENT_COUNT; i++)
    {
        REQUIRE(realState.agents[i].dead == reconstructedState.agents[i].dead);
    }
}