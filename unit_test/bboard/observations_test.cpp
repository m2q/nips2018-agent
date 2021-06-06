#include <iostream>

#include "catch.hpp"
#include "bboard.hpp"

using namespace bboard;

void REQUIRE_CORRECT_FOG(const State& s, uint agentID, const Board& b, int viewRange)
{
    bool foundIncorrectCell = false;
    for(int y = 0; y < BOARD_SIZE; y++)
    {
        for(int x = 0; x < BOARD_SIZE; x++)
        {
            bool isFog = b.items[y][x] == Item::FOG;
            bool isInViewRange = InViewRange(s.agents[agentID].GetPos(), x, y, viewRange);

            if(isFog == isInViewRange)
            {
                foundIncorrectCell = true;
                break;
            }
        }

        if(foundIncorrectCell)
            break;
    }
    REQUIRE(foundIncorrectCell == false);
}

TEST_CASE("View Range", "[observation]")
{
    // initialize some state
    State s;
    s.Init(GameMode::FreeForAll, 4, true);

    SECTION("No fog in state")
    {
        bool foundFog = false;
        for(int y = 0; y < BOARD_SIZE; y++)
        {
            for(int x = 0; x < BOARD_SIZE; x++)
            {
                if(s.items[y][x] == Item::FOG)
                {
                    foundFog = true;
                    break;
                }
            }

            if(foundFog)
                break;
        }
        REQUIRE(foundFog == false);
    }

    SECTION("Fog in observation 0")
    {
        ObservationParameters params;
        params.agentPartialMapView = true;
        params.agentViewSize = 3;

        Observation obs;
        Observation::Get(s, 0, params, obs);

        REQUIRE_CORRECT_FOG(s, 0, obs, params.agentViewSize);
    }

    SECTION("Fog in observation 2")
    {
        ObservationParameters params;
        params.agentPartialMapView = true;
        params.agentViewSize = 5;

        Observation obs;
        Observation::Get(s, 2, params, obs);

        REQUIRE_CORRECT_FOG(s, 2, obs, params.agentViewSize);
    }
}

TEST_CASE("Round trip", "[observation]")
{
    // initialize state
    State s;
    s.Init(GameMode::FreeForAll, 1234, true);

    // restrict view
    ObservationParameters params;
    params.agentPartialMapView = true;
    params.agentViewSize = 4;

    // get observation
    Observation obs;
    Observation::Get(s, 2, params, obs);

    // check fog
    REQUIRE_CORRECT_FOG(s, 2, obs, params.agentViewSize);

    // convert to state
    State s2;
    obs.ToState(s2, GameMode::FreeForAll);

    // fog is still there
    REQUIRE_CORRECT_FOG(s, 2, s2, params.agentViewSize);
    REQUIRE(s.aliveAgents == s2.aliveAgents);
    REQUIRE(s.agents[2].x == s2.agents[2].x);
    REQUIRE(s.agents[2].y == s2.agents[2].y);
    REQUIRE(s.agents[2].team == s2.agents[2].team);
    REQUIRE(s2.agents[2].ignore == false);
}

TEST_CASE("Planning Step", "[observation]")
{
    ObservationParameters params;
    params.agentPartialMapView = true;
    params.agentViewSize = 4;

    State s, s2;
    Observation obs;

    s.Init(GameMode::FreeForAll, 1234, true);
    Observation::Get(s, 0, params, obs);
    obs.ToState(s2, GameMode::FreeForAll);

    Move m[4];
    m[0] = m[1] = m[2] = m[3] = Move::IDLE;

    AgentInfo& ownAgent = s2.agents[0];

    SECTION("Step")
    {
        // just check whether we can do a step
        bboard::Step(&s2, m);
    }
    SECTION("Bombs explode")
    {
        REQUIRE(!ownAgent.dead);
        m[0] = Move::BOMB;
        bboard::Step(&s2, m);
        m[0] = Move::IDLE;

        for(int _ = 0; _ < bboard::BOMB_LIFETIME; _++)
        {
            bboard::Step(&s2, m);
        }

        REQUIRE(ownAgent.dead);
    }
}

void REQUIRE_ITEM_IF(int item, bool condition, int trueItem, int falseItem)
{
    if(condition)
    {
        REQUIRE(item == trueItem);
    }
    else
    {
        REQUIRE(item == falseItem);
    }
}

TEST_CASE("Merge Observations", "[observation]")
{
    ObservationParameters params;
    params.agentPartialMapView = true;
    params.agentViewSize = 1;

    State s;
    s.Clear(Item::PASSAGE);

    // 0 1
    // 3 2
    s.PutAgentsInCorners(0, 1, 2, 3, 1);
    s.PutItem(0, 0, Item::RIGID);
    s.PutItem(0, 1, Item::WOOD);

    Observation obs;
    Observation::Get(s, 0, params, obs);

    // all the items are within our view range
    REQUIRE(obs.items[0][0] == Item::RIGID);
    REQUIRE(obs.items[1][0] == Item::WOOD);
    REQUIRE(obs.items[2][0] == Item::PASSAGE);

    Move m[AGENT_COUNT];
    std::fill_n(m, AGENT_COUNT, Move::IDLE);
    m[0] = Move::RIGHT;

    int itemAge[BOARD_SIZE][BOARD_SIZE];
    std::fill_n(&itemAge[0][0], BOARD_SIZE * BOARD_SIZE, 0);

    Observation newObs;
    // walk to the right until we see agent 1
    for(int i = 0; i < BOARD_SIZE - 4; i++)
    {
        bboard::Step(&s, m);

        // the items are out of range now
        Observation::Get(s, 0, params, newObs);
        REQUIRE(newObs.items[0][0] == Item::FOG);
        REQUIRE(newObs.items[1][0] == Item::FOG);
        REQUIRE(newObs.items[2][0] == Item::FOG);

        // but can be reconstructed
        newObs.Merge(obs, params, false, false, &itemAge);
        REQUIRE(newObs.items[0][0] == Item::RIGID);
        REQUIRE(newObs.items[1][0] == Item::WOOD);
        REQUIRE(newObs.items[2][0] == Item::PASSAGE);
        REQUIRE(itemAge[0][0] == (i + 1));
        REQUIRE(itemAge[1][1 + i] == 0);
        obs = newObs;
    }

    // place a bomb next to agent 1
    REQUIRE(s.items[1][BOARD_SIZE - 3] == Item::AGENT0);
    REQUIRE(s.items[1][BOARD_SIZE - 2] == Item::AGENT1);
    m[0] = Move::BOMB;

    bboard::Step(&s, m);
    REQUIRE(s.HasBomb(BOARD_SIZE - 3, 1));

    Observation::Get(s, 0, params, newObs);
    newObs.Merge(obs, params, false, false, &itemAge);
    obs = newObs;

    // run away downwards
    m[0] = Move::DOWN;

    // bomb and target agent are directly visible on the first step
    bboard::Step(&s, m);

    Observation::Get(s, 0, params, newObs);
    REQUIRE(newObs.items[1][BOARD_SIZE - 3] == Item::BOMB);
    REQUIRE(newObs.items[1][BOARD_SIZE - 2] == Item::AGENT1);

    newObs.Merge(obs, params, false, false, &itemAge);
    obs = newObs;

    for (bool trackBombs : {true, false})
    {
        for(bool trackAgents : {true, false})
        {
            SECTION("Track Bombs (" + std::to_string(trackBombs) + ") and Agents (" + std::to_string(trackAgents) + ")")
            {
                // ... but not in the following steps
                for(int i = 0; i < BOMB_LIFETIME - 2; i++)
                {
                    bboard::Step(&s, m);

                    Observation::Get(s, 0, params, newObs);
                    REQUIRE(newObs.items[1][BOARD_SIZE - 3] == Item::FOG);
                    REQUIRE(newObs.items[1][BOARD_SIZE - 2] == Item::FOG);

                    // however, they can be reconstructed
                    newObs.Merge(obs, params, trackAgents, trackBombs, &itemAge);
                    REQUIRE_ITEM_IF(newObs.items[1][BOARD_SIZE - 3], trackBombs, Item::BOMB, Item::PASSAGE);
                    REQUIRE_ITEM_IF(newObs.items[1][BOARD_SIZE - 2], trackAgents, Item::AGENT1, Item::PASSAGE);
                    obs = newObs;
                }

                // reconstructed bombs eventually explode, even out of view
                bboard::Step(&s, m);
                REQUIRE(s.items[1][BOARD_SIZE - 3] == Item::FLAME);
                REQUIRE(s.agents[1].dead == true);

                Observation::Get(s, 0, params, newObs);
                REQUIRE(newObs.items[1][BOARD_SIZE - 3] == Item::FOG);

                newObs.Merge(obs, params, trackAgents, trackBombs, &itemAge);
                REQUIRE_ITEM_IF(newObs.items[1][BOARD_SIZE - 3], trackBombs, Item::FLAME, Item::PASSAGE);
                REQUIRE(newObs.isAlive[1] == false);
                REQUIRE(itemAge[1][BOARD_SIZE - 3] == BOMB_LIFETIME - 1);
                obs = newObs;

                if(trackBombs)
                {
                    // ... and the spawned flames eventually vanish
                    for(int i = 0; i < FLAME_LIFETIME - 1; i++)
                    {
                        bboard::Step(&s, m);
                        Observation::Get(s, 0, params, newObs);
                        newObs.Merge(obs, params, true, true, &itemAge);
                        obs = newObs;
                    }

                    bboard::Step(&s, m);
                    REQUIRE(s.items[1][BOARD_SIZE - 3] == Item::PASSAGE);

                    Observation::Get(s, 0, params, newObs);
                    REQUIRE(newObs.items[1][BOARD_SIZE - 3] == Item::FOG);

                    newObs.Merge(obs, params, trackAgents, trackBombs, &itemAge);
                    REQUIRE(newObs.items[1][BOARD_SIZE - 3] == Item::PASSAGE);
                    obs = newObs;
                }
            }
        }
    }
}
