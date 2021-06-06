#ifndef PYMETHODS_H
#define PYMETHODS_H

#include <memory>
#include "bboard.hpp"

namespace PyInterface 
{
    extern bboard::State state;
    extern bboard::Observation observation;
    extern std::unique_ptr<bboard::Agent> agent; 

    extern bool agentHasId;

    /**
     * @brief Create an agent with the type specified by the given name.
     * @param agentName A name representing the desired agent type
     * @param seed Use this seed to initialize the agent if it includes randomness
     * @return A pointer to the created agent (or nullptr)
     */
    bboard::Agent* new_agent(std::string agentName, long seed);
}

extern "C" {
    /**
     * @brief Create an agent with the type specified by the given name.
     * @param agentName A name representing the desired agent type
     * @param seed Use this seed to initialize the agent if it includes randomness
     * @return Whether the agent has been created 
     * (only if there is an agent type for the given name)
     */
    bool agent_create(char* agentName, long seed);

    /**
     * @brief Reset the state of the agent.
     * @param id The id of the agent
     */
    void agent_reset(int id);

    /**
     * @brief Get the action from the current agent for the given state/observation.
     * @param json The current state/observation
     * @param jsonIsState Whether json contains a state (true) or observation (false)
     * @return The action of the agent. -1 if there has been an error.
     */
    int agent_act(char* json, bool jsonIsState);
}

#endif // PYMETHODS_H
