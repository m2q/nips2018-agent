#include <thread>
#include <chrono>
#include <functional>
#include <algorithm>
#include <random>

#include <bits/stdc++.h>

#include "bboard.hpp"
#include "step_utility.hpp"

namespace bboard
{

inline void copyAgentInfosTo(const State& state, Observation& observation)
{
    observation.agentInfos.count = 0;
    for(int i = 0; i < AGENT_COUNT; i++)
    {
        observation.agentIDMapping[i] = i;
        observation.agentInfos.AddElem(state.agents[i]);
    }
}

inline void copyTo(const State& state, Observation& observation)
{
    observation.CopyFrom(state);
    copyAgentInfosTo(state, observation);
}

inline void setAgentArrays(const State& state, const uint ownAgentID, Observation& observation)
{
    AgentInfo self = state.agents[ownAgentID];
    observation.isAlive[ownAgentID] = !self.dead;
    observation.isEnemy[ownAgentID] = false;

    for(uint i = 0; i < AGENT_COUNT; i++)
    {
        if(i == ownAgentID) continue;

        AgentInfo info = state.agents[i];
        observation.isAlive[i] = !info.dead;
        observation.isEnemy[i] = info.team == 0 || info.team != self.team;
    }
}

void Observation::Get(const State& state, const uint agentID, const ObservationParameters obsParams, Observation& observation)
{
    observation.agentID = agentID;
    setAgentArrays(state, agentID, observation);

    // fully observable environment
    if(obsParams.exposePowerUps && !obsParams.agentPartialMapView && obsParams.agentInfoVisibility == AgentInfoVisibility::All)
    {
        copyTo(state, observation);
        return;
    }

    // board observations

    if(obsParams.agentPartialMapView)
    {
        // hide invisible area
        AgentInfo info = state.agents[agentID];

        int leftFogCount = std::max(0, info.x - obsParams.agentViewSize);
        int rightFogBegin = std::min(BOARD_SIZE, info.x + obsParams.agentViewSize + 1);
        int rightFogCount = BOARD_SIZE - rightFogBegin;
        int viewRowLength = rightFogBegin - leftFogCount;

        for(int y = 0; y < BOARD_SIZE; y++)
        {
            if(std::abs(y - info.y) > obsParams.agentViewSize)
            {
                // fill the whole row with fog
                std::fill_n(&observation.items[y][0], BOARD_SIZE, bboard::Item::FOG);
            }
            else
            {
                // row is inside the agent's view, fill the row partially with fog

                // cells on the left
                std::fill_n(&observation.items[y][0], leftFogCount, bboard::Item::FOG);

                // copy board items
                if(obsParams.exposePowerUps)
                {
                    std::copy_n(&state.items[y][leftFogCount], viewRowLength, &observation.items[y][leftFogCount]);
                }
                else
                {
                    for(int x = leftFogCount; x < rightFogBegin; x++)
                    {
                        int item = state.items[y][x];
                        if(IS_WOOD(item))
                        {
                            // erase the powerup information
                            observation.items[y][x] = Item::WOOD;
                        }
                        else
                        {
                            observation.items[y][x] = item;
                        }
                    }
                }

                // cells on the right
                std::fill_n(&observation.items[y][rightFogBegin], rightFogCount, bboard::Item::FOG);
            }
        }

        // filter bomb objects
        observation.bombs.count = 0;
        for(int i = 0; i < state.bombs.count; i++)
        {
            int b = state.bombs[i];

            if(InViewRange(info.x, info.y, BMB_POS_X(b), BMB_POS_Y(b), obsParams.agentViewSize))
            {
                observation.bombs.AddElem(b);
            }
        }

        // and flames
        observation.flames.count = 0;
        for(int i = 0; i < state.flames.count; i++)
        {
            Flame f = state.flames[i];

            if(InViewRange(f.position, info.x, info.y, obsParams.agentViewSize))
            {
                observation.flames.AddElem(f);
            }
        }
    }
    else
    {
        // full view on the arena
        observation.CopyFrom(state);

        if(!obsParams.exposePowerUps)
        {
            for(int y = 0; y < BOARD_SIZE; y++)
            {
                for(int x = 0; x < BOARD_SIZE; x++)
                {
                    int item = observation.items[y][x];
                    if(IS_WOOD(item) && item != Item::WOOD)
                    {
                        // erase the powerup information
                        observation.items[y][x] = Item::WOOD;
                    }
                }
            }
        }
    }

    // agent observations

    switch (obsParams.agentInfoVisibility) {
        case bboard::AgentInfoVisibility::All:
            copyAgentInfosTo(state, observation);
            break;

        case bboard::AgentInfoVisibility::OnlySelf:
            observation.agentInfos.AddElem(state.agents[agentID]);
            std::fill_n(observation.agentIDMapping, AGENT_COUNT, -1);
            observation.agentIDMapping[agentID] = 0;
            break;

        case bboard::AgentInfoVisibility::InView:
            AgentInfo self = state.agents[agentID];
            // add self
            observation.agentIDMapping[agentID] = 0;
            observation.agentInfos.AddElem(self);
            // add others if visible
            for(uint i = 0; i < AGENT_COUNT; i++)
            {
                if(i == agentID) continue;

                AgentInfo other = state.agents[i];
                if(InViewRange(self.x, self.y, other.x, other.y, obsParams.agentViewSize))
                {
                    // visible
                    observation.agentIDMapping[i] = observation.agentInfos.count;
                    observation.agentInfos.AddElem(other);
                }
                else
                {
                    // not visible
                    observation.agentIDMapping[i] = -1;
                }
            }
            break;
    }
}

void Observation::ToState(State& state, GameMode gameMode) const
{
    // initialize the board of the state
    state.CopyFrom(*this);

    // optimize flame queue
    state.currentFlameTime = util::OptimizeFlameQueue(state);

    // set the correct teams
    SetTeams(state.agents, gameMode);

    int aliveAgents = 0;
    for(int i = 0; i < AGENT_COUNT; i++)
    {
        AgentInfo& info = state.agents[i];

        int index = agentIDMapping[i];
        if(index > -1)
        {
            // copy the available info
            info = agentInfos[index];
            info.ignore = false;
        }
        else
        {
            // we don't know much about this agent and want to ignore it
            // use unique positions out of bounds to be compatible with the destination checks
            info.x = -i;
            info.y = -1;
            info.won = false;
            info.dead = !isAlive[i];
            info.ignore = true;
            // unknown: canKick, bombCount, maxBombCount, bombStrength
        }

        if(isAlive[i])
        {
            aliveAgents += 1;
        }
    }

    state.aliveAgents = aliveAgents;

    // search for agents and set their correct positions if available
    // TODO: Is this necessary?
    for(int y = 0; y < BOARD_SIZE; y++)
    {
        for(int x = 0; x < BOARD_SIZE; x++)
        {
            int item = items[y][x];
            if(item >= Item::AGENT0)
            {
                int id = item - Item::AGENT0;
                // std::cout << "Set agent " << id << " pos to " << y << ", " << x << std::endl;
                AgentInfo& info = state.agents[id];
                info.x = x;
                info.y = y;
                info.ignore = false;
            }
        }
    }

    util::CheckTerminalState(state);
}

void _addBomb(Board& board, Bomb bomb)
{
    for(int i = 0; i < board.bombs.count; i++)
    {
        // add it at the correct position (sorted list)
        if(BMB_TIME(bomb) > BMB_TIME(board.bombs[i]))
        {
            if(i == 0)
            {
                break;
            }
            else
            {
                board.bombs.AddElem(bomb, i - 1);
                return;
            }
        }
    }

    // add it at the end
    board.bombs.AddElem(bomb);
}

void _addBombsFromLastStep(const Observation& last, Observation& current, const ObservationParameters& params)
{
    if(!params.agentPartialMapView)
        return;

    Position center = current.agentInfos[current.agentIDMapping[current.agentID]].GetPos();

    std::unordered_set<Position> positions(current.bombs.count);

    // remember the positions of all existing bombs
    for(int i = 0; i < current.bombs.count; i++)
    {
        positions.insert(BMB_POS(current.bombs[i]));
    }

    // try to add old bombs from last step
    for(int i = 0; i < last.bombs.count; i++)
    {
        Bomb b = last.bombs[i];
        Position bPos = BMB_POS(b);

        if(InViewRange(center, bPos, params.agentViewSize))
        {
            // bombs in view range are already handled correctly
            continue;
        }

        auto res = positions.find(bPos);
        if(res != positions.end())
        {
            // there is already a bomb at this position
            continue;
        }

        // otherwise: reduce timer and add the bomb at the right index (sorted list)
        ReduceBombTimer(b);
        // TODO: Handle bomb movement (?)
        SetBombDirection(b, Direction::IDLE);

        _addBomb(current, b);
    }
}

void _addFlamesFromLastStep(const Observation& last, Observation& current, const ObservationParameters& params)
{
    if(!params.agentPartialMapView)
        return;

    Position center = current.agentInfos[current.agentIDMapping[current.agentID]].GetPos();

    std::unordered_set<Position> positions(current.flames.count);

    // remember the positions of all existing flames
    for(int i = 0; i < current.flames.count; i++)
    {
        positions.insert(current.flames[i].position);
    }

    for(int i = 0; i < last.flames.count; i++)
    {
        Flame f = last.flames[i];

        // only update flames which are outside of our view
        // and not already in our flames list
        if(!InViewRange(center, f.position, params.agentViewSize)
                && positions.find(f.position) == positions.end())
        {
            if(f.timeLeft <= 1)
            {
                // disappearing flames leave passages behind (last item is flame)
                current.items[f.position.y][f.position.x] = Item::PASSAGE;
            }
            else
            {
                // add the new flame to the obs (item is already set)
                f.timeLeft -= 1;
                current.flames.AddElem(f);
            }
        }
    }
}

std::array<bool, AGENT_COUNT> _getAgentVisibility(const Board &board)
{
    std::array<bool, AGENT_COUNT> agentVisibility;
    std::fill(agentVisibility.begin(), agentVisibility.end(), false);

    for(int y = 0; y < BOARD_SIZE; y++)
    {
        for(int x = 0; x < BOARD_SIZE; x++)
        {
            int item = board.items[y][x];
            if(item >= Item::AGENT0)
            {
                const int id = item - Item::AGENT0;
                agentVisibility[id] = true;
            }
        }
    }

    return agentVisibility;
}

void Observation::Merge(const Observation& last, const ObservationParameters& params, bool agents, bool bombs, int (*itemAge)[BOARD_SIZE][BOARD_SIZE])
{
    std::array<bool, AGENT_COUNT> agentVisibility = _getAgentVisibility(*this);

    // reconstruct fogged items based on the last observation
    for(int y = 0; y < BOARD_SIZE; y++)
    {
        for(int x = 0; x < BOARD_SIZE; x++)
        {
            int item = items[y][x];

            // try to reconstruct fog
            if(item == Item::FOG)
            {
                int oldItem = last.items[y][x];

                if(oldItem == Item::FOG)
                {
                    // no old item here
                    continue;
                }

                // no fog here -> reconstruct

                if(oldItem >= Item::AGENT0)
                {
                    const int id = oldItem - Item::AGENT0;
                    if(!agents || agentVisibility[id])
                    {
                        // skip agents when we ignore them or when they
                        // are already visible in our current observation
                        // (-> they moved)
                        oldItem = Item::PASSAGE;
                    }
                }

                if(oldItem == Item::BOMB)
                {
                    if(bombs)
                    {
                        // add bombs (may explode later)
                        oldItem = Item::BOMB;
                    }
                    else
                    {
                        // treat the bomb as a passage
                        oldItem = Item::PASSAGE;
                    }
                }

                items[y][x] = oldItem;

                if(itemAge != nullptr)
                {
                    // increase the age of that reconstructed item
                    (*itemAge)[y][x] += 1;
                }

                continue;
            }

            // new obs is not fog

            if(itemAge != nullptr)
            {
                (*itemAge)[y][x] = 0;
            }
        }
    }

    if(bombs)
    {
        _addBombsFromLastStep(last, *this, params);
        // after adding bombs, let them explode (if necessary)
        util::ExplodeBombs(this);
    }

    _addFlamesFromLastStep(last, *this, params);
}

// methods from board

void Observation::Kill(int agentID)
{
    if(agentIDMapping[agentID] != -1)
    {
        agentInfos[agentIDMapping[agentID]].dead = true;
    }

    isAlive[agentID] = false;
}

void Observation::EventBombExploded(__attribute__((unused)) Bomb b) {}

}
