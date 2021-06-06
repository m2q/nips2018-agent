#include <thread>
#include <chrono>
#include <functional>
#include <algorithm>
#include <random>

#include "bboard.hpp"

namespace bboard
{

void Environment::PrintGameResult()
{
    std::cout << std::endl;

    if(IsDone())
    {
        if(IsDraw())
        {
            std::cout << "Draw! All agents are dead."
                      << std::endl;
        }
        else
        {
            std::cout << "Finished! ";
            int winningAgent = GetWinningAgent();
            int winningTeam = GetWinningTeam();

            if(winningAgent != -1)
            {
                std::cout << "Winning agent: " << winningAgent;
            }
            else if(winningTeam != 0)
            {
                std::cout << "Winning team: " << winningTeam << " (";

                // list agents in team
                bool first = true;
                for(int i = 0; i < AGENT_COUNT; i++)
                {
                    if(state->agents->team == winningTeam)
                    {
                        if(!first)
                        {
                            std::cout << ", ";
                        }

                        std::cout << i;
                        first = false;
                    }
                }

                std::cout << ")";
            }
            else
            {
                std::cout << "Undefined result!";
            }

            std::cout << std::endl;
        }
    }
    else
    {
        std::cout << "Not done!" << std::endl;
    }
}

Environment::Environment()
{
    state = std::make_unique<State>();
}

void Environment::MakeGame(std::array<Agent*, AGENT_COUNT> a, GameMode gameMode, long boardSeed, long agentPositionSeed)
{
    if(hasStarted)
    {
        // reset state
        *state.get() = State();
    }

    this->gameMode = gameMode;

    state->Init(gameMode, boardSeed, agentPositionSeed);

    SetAgents(a);
    hasStarted = true;
}

void Environment::RunGame(int steps, bool asyncAct, bool render, bool renderClear, bool renderInteractive, int renderWaitMs)
{
    int startSteps = state->timeStep;
    while(!IsDone() && (steps <= 0 || state->timeStep - startSteps < steps))
    {
        if(render)
        {   
            Print(renderClear);

            if(listener)
                listener(*this);

            if(renderInteractive)
                std::cin.get();

            if(renderWaitMs > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(renderWaitMs));

            if(!renderClear)
            {
                std::cout << std::endl;
            }
        }

        Step(asyncAct);
    }

    if(render)
    {
        Print(renderClear);
        PrintGameResult();
    }
}

void ProxyAct(Move& writeBack, Agent& agent, State& state)
{
    writeBack = agent.act(&state);
}

void CollectMovesAsync(Move m[AGENT_COUNT], Environment& e)
{
    std::thread threads[AGENT_COUNT];
    for(uint i = 0; i < AGENT_COUNT; i++)
    {
        if(!e.GetState().agents[i].dead)
        {
            threads[i] = std::thread(ProxyAct,
                                     std::ref(m[i]),
                                     std::ref(*e.GetAgent(i)),
                                     std::ref(e.GetState()));
        }
    }

    // join threads
    for(uint i = 0; i < AGENT_COUNT; i++)
    {
        if(!e.GetState().agents[i].dead)
        {
            threads[i].join();
        }
    }
}

Move Environment::GetLastMove(int agentID)
{
    return lastMoves[agentID];
}

bool Environment::HasActed(int agentID)
{
    return hasActed[agentID];
}

void Environment::Step(bool asyncAct)
{
    if(IsDone())
    {
        return;
    }

    Move m[AGENT_COUNT];

    if(asyncAct)
    {
        CollectMovesAsync(m, *this);
    }
    else
    {
        for(uint i = 0; i < AGENT_COUNT; i++)
        {
            if(!state->agents[i].dead)
            {
                m[i] = agents[i]->act(state.get());
                lastMoves[i] = m[i];
                hasActed[i] = true;
            }
            else
            {
                hasActed[i] = false;
            }
        }
    }

    bboard::Step(state.get(), m);
}

void Environment::Print(bool clear)
{
    std::cout << "Step " << state->timeStep << std::endl;
    PrintState(state.get(), clear);
}

State& Environment::GetState() const
{
    return *state.get();
}

Agent* Environment::GetAgent(uint agentID) const
{
    return agents[agentID];
}

void Environment::SetAgents(std::array<Agent*, AGENT_COUNT> agents)
{
    for(uint i = 0; i < AGENT_COUNT; i++)
    {
        agents[i]->id = int(i);
        agents[i]->reset();
    }

    this->agents = agents;
}

bool Environment::IsDone()
{
    return state->finished;
}

bool Environment::IsDraw()
{
    return state->isDraw;
}

int Environment::GetWinningAgent()
{
    return state->winningAgent;
}

int Environment::GetWinningTeam()
{
    return state->winningTeam;
}

void Environment::SetStepListener(const std::function<void(const Environment&)>& f)
{
    listener = f;
}

}
