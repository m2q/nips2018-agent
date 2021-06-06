#include "pymethods.hpp"

#include <iostream>
#include "from_json.hpp"

// init interface state

bboard::State PyInterface::state;
bboard::Observation PyInterface::observation;
bool PyInterface::agentHasId = false;
std::unique_ptr<bboard::Agent> PyInterface::agent(nullptr);

// interface methods

void _reset_state_and_obs()
{
    PyInterface::state = bboard::State();
    PyInterface::observation = bboard::Observation();
}

bool agent_create(char* agentName, long seed)
{
    auto createdAgent = PyInterface::new_agent(agentName, seed);
    if(createdAgent == nullptr)
    {
        return false;
    }

    PyInterface::agent.reset(createdAgent);
    PyInterface::agentHasId = false;

    return true;
}

void agent_reset(int id)
{
    auto agent = PyInterface::agent.get();
    if(agent)
    {
        agent->id = id;
        agent->reset();
        _reset_state_and_obs();
        PyInterface::agentHasId = true;
    }
}

int agent_act(char* json, bool jsonIsState)
{
    auto agent = PyInterface::agent.get();
    if(!agent)
    {
        std::cout << "Agent does not exist!" << std::endl;
        return -1;
    }
    if(!PyInterface::agentHasId)
    {
        std::cout << "Agent has no id!" << std::endl;
        return -1;
    }

    std::string jsonString(json);
    if(jsonIsState)
    {
        // std::cout << "json > state" << std::endl;
        StateFromJSON(PyInterface::state, jsonString, GameMode::FreeForAll);
    }
    else
    {
        // std::cout << "json > obs" << std::endl;
        ObservationFromJSON(PyInterface::observation, jsonString, agent->id);
        PyInterface::observation.ToState(PyInterface::state, GameMode::FreeForAll);
    }

    bboard::Move move = agent->act(&PyInterface::state);
    // bboard::PrintState(&PyInterface::state);
    // std::cout << "Agent " << agent->id << " selects action " << (int)move << std::endl;
    return (int)move;
}