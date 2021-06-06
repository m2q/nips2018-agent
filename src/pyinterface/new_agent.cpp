#include "pymethods.hpp"
#include "agents.hpp"

#include <iostream>
#include "string.h"

bboard::Agent* PyInterface::new_agent(std::string agentName, long seed)
{
    if(agentName == "SimpleAgent")
    {
        return new agents::SimpleAgent(seed);
    }
    else if(agentName == "SimpleUnbiasedAgent")
    {
        return new agents::SimpleUnbiasedAgent(seed);
    }
    else
    {
        return nullptr;
    }
}