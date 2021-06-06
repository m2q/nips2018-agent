#include <iostream>
#include <chrono>
#include <thread>

#include "bboard.hpp"
#include "agents.hpp"

int main()
{
    agents::SimpleAgent a[4];
    std::array<bboard::Agent*, 4> agents = {&a[0], &a[1], &a[2], &a[3]};

    bboard::Environment env;
    env.MakeGame(agents, bboard::GameMode::FreeForAll);

    for(bboard::AgentInfo& agentInfo : env.GetState().agents)
    {
        agentInfo.canKick = true;
    }

    env.RunGame(500, false, true);
}
