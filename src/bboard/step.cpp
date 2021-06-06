#include <iostream>

#include "bboard.hpp"
#include "step_utility.hpp"

namespace bboard
{

void Step(State* state, Move* moves)
{
    // do not execute step on terminal states
    if(state->finished)
        return;

    int aliveAgentsBefore = state->aliveAgents;

    // tick flames (they might disappear)
    util::TickFlames(state);

    // resolve collisions in player movement

    Position oldPos[AGENT_COUNT];
    Position destPos[AGENT_COUNT];
    bool dead[AGENT_COUNT];

    util::FillPositions(state, oldPos);
    util::FillDestPos(state, moves, destPos);
    util::FillAgentDead(state, dead);

    util::FixDestPos<true>(oldPos, destPos, AGENT_COUNT, dead);

    // calculate dependencies in the player movement

    int dependency[AGENT_COUNT];
    std::fill_n(dependency, AGENT_COUNT, -1);
    int roots[AGENT_COUNT];
    std::fill_n(roots, AGENT_COUNT, -1);

    // the amount of chain roots
    const int rootNumber = util::ResolveDependencies(state, destPos, dependency, roots);

    int rootIdx = 0;
    int i = rootNumber == 0 ? 0 : roots[0]; // no roots -> start from 0

    // ouroboros: every agent wants to move to the current position of a different agent
    // A > B
    // ^   v
    // D < C
    bool ouroboros = rootNumber == 0;

    // first update the player positions

    // apply the moves in the correct order
    // iterates 4 times but the index i jumps around the dependencies
    for(int _ = 0; _ < AGENT_COUNT; _++, i = dependency[i])
    {
        if(i == -1)
        {
            rootIdx++;
            i = roots[rootIdx];
        }

        util::MoveAgent(state, i, moves[i], destPos[i], ouroboros);
    }

    // then update the bomb positions

    // resolve conflicting bomb destinations (and reset affected agents)
    util::ResolveBombMovement(state, oldPos);

    // move the bombs (bombs can also explode if they move into flames)
    util::MoveBombs(state);

    // let bombs explode
    util::TickBombs(state);
    util::ExplodeBombs(state);

    // advance timestep
    state->timeStep++;

    if(aliveAgentsBefore != state->aliveAgents)
    {
        // the number of agents has changed, check if some agent(s) won the game
        util::CheckTerminalState(*state);
    }
}

}
