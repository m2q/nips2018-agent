#include <iostream>

#include "catch.hpp"
#include "bboard.hpp"

using namespace bboard;
/**
 * @brief REQUIRE_AGENT Proxy REQUIRE-Assertion to test
 * valid position on board AND agent arrays.
 */
void REQUIRE_AGENT(bboard::State* state, int agent, int x, int y)
{
    int o = bboard::Item::AGENT0 + agent;
    REQUIRE(state->agents[agent].GetPos() == (Position){x, y});
    REQUIRE(state->items[y][x] == o);
}

/**
 * @brief SeveralSteps Proxy for bboard::Step, useful for testing
 */
void SeveralSteps(int times, bboard::State* s, bboard::Move* m)
{
    for(int i = 0; i < times; i++)
    {
        bboard::Step(s, m);
    }
}

/**
 * @brief PlaceBombsDiagonally Lets an agent plant bombs along a
 * horizontal
 */
void PlaceBombsHorizontally(bboard::State* s, int agent, int bombs)
{
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[bboard::AGENT_COUNT] = {id, id, id, id};

    for(int i = 0; i < bombs; i++)
    {
        m[agent] = bboard::Move::BOMB;
        bboard::Step(s, m);
        m[agent] = bboard::Move::RIGHT;
        bboard::Step(s, m);
    }
}

bool IsAgentPos(bboard::State* state, int agent, int x, int y)
{
    int o = bboard::Item::AGENT0 + agent;
    return  state->agents[agent].x == x &&
            state->agents[agent].y == y && state->items[y][x] == o;
}

/**
 * @brief PlantBomb Emulate planting a bomb at some position.
 */
void PlantBomb(State* s, int x, int y, int id, bool setItem=false)
{
    // the agent first moves to this position
    AgentInfo& agent = s->agents[id];
    Position oldPosition = agent.GetPos();
    agent.x = x;
    agent.y = y;
    // then plants a bomb
    s->TryPlantBomb<false>(agent, id, setItem);
    // then moves back
    agent.x = oldPosition.x;
    agent.y = oldPosition.y;
}

TEST_CASE("Basic Non-Obstacle Movement", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    s->PutAgentsInCorners(0, 1, 2, 3, 0);

    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    m[0] = bboard::Move::RIGHT;
    bboard::Step(s.get(), m);
    REQUIRE_AGENT(s.get(), 0, 1, 0);

    m[0] = bboard::Move::DOWN;
    bboard::Step(s.get(), m);
    REQUIRE_AGENT(s.get(), 0, 1, 1);

    m[0] = bboard::Move::LEFT;
    bboard::Step(s.get(), m);
    REQUIRE_AGENT(s.get(), 0, 0, 1);

    m[0] = bboard::Move::UP;
    bboard::Step(s.get(), m);
    REQUIRE_AGENT(s.get(), 0, 0, 0);

    m[3] = bboard::Move::UP;
    bboard::Step(s.get(), m);
    REQUIRE_AGENT(s.get(), 3, 0, 9);
}

TEST_CASE("Snake Movement", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    s->PutAgent(0, 0, 0);
    s->PutAgent(1, 0, 1);
    s->PutAgent(2, 0, 2);
    s->PutAgent(3, 0, 3);

    bboard::Move r = Move::RIGHT;
    bboard::Move m[4] = {r, r, r, r};

    bboard::Step(s.get(), m);

    REQUIRE_AGENT(s.get(), 0, 1, 0);
    REQUIRE_AGENT(s.get(), 1, 2, 0);
    REQUIRE_AGENT(s.get(), 2, 3, 0);
    REQUIRE_AGENT(s.get(), 3, 4, 0);
}

TEST_CASE("Basic Obstacle Collision", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    s->PutAgentsInCorners(0, 1, 2, 3, 0);

    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    s->PutItem(1, 0, bboard::Item::RIGID);

    m[0] = bboard::Move::RIGHT;
    bboard::Step(s.get(), m);
    REQUIRE_AGENT(s.get(), 0, 0, 0);

    m[0] = bboard::Move::DOWN;
    bboard::Step(s.get(), m);
    REQUIRE_AGENT(s.get(), 0, 0, 1);
}

TEST_CASE("Movement Against Flames", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    s->PutAgentsInCorners(0, 1, 2, 3, 0);
    s->SpawnFlames(1,1,2);

    m[0] = bboard::Move::RIGHT;

    bboard::Step(s.get(), m);

    REQUIRE(s->agents[0].dead);
    REQUIRE(s->items[0][0] == bboard::Item::PASSAGE);
}

TEST_CASE("Destination Collision", "[step function]")
{
    auto sx = std::make_unique<bboard::State>();
    bboard::State* s = sx.get();

    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    s->PutAgent(0, 1, 0);
    s->PutAgent(2, 1, 1);

    s->Kill(2, 3);

    SECTION("Two Agent-Collision")
    {
        m[0] = bboard::Move::RIGHT;
        m[1] = bboard::Move::LEFT;

        bboard::Step(s, m);

        REQUIRE_AGENT(s, 0, 0, 1);
        REQUIRE_AGENT(s, 1, 2, 1);
    }
    SECTION("Dead Collision")
    {
        m[0] = bboard::Move::RIGHT;
        m[1] = bboard::Move::LEFT;
        s->Kill(1);

        bboard::Step(s, m);

        REQUIRE_AGENT(s, 0, 1, 1);
    }
    SECTION("Four Agent-Collision")
    {
        s->PutAgent(1, 0, 2);
        s->PutAgent(1, 2, 3);

        m[0] = bboard::Move::RIGHT;
        m[1] = bboard::Move::LEFT;
        m[2] = bboard::Move::DOWN;
        m[3] = bboard::Move::UP;

        bboard::Step(s, m);

        REQUIRE_AGENT(s, 0, 0, 1);
        REQUIRE_AGENT(s, 1, 2, 1);
        REQUIRE_AGENT(s, 2, 1, 0);
        REQUIRE_AGENT(s, 3, 1, 2);
    }
}

TEST_CASE("Movement Dependency Handling", "[step function]")
{
    auto sx = std::make_unique<bboard::State>();
    bboard::State* s = sx.get();

    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    SECTION("Move Chain Against Obstacle")
    {
        s->PutAgent(0, 0, 0);
        s->PutAgent(1, 0, 1);
        s->PutAgent(2, 0, 2);
        s->PutAgent(3, 0, 3);

        s->PutItem(4, 0, bboard::Item::RIGID);

        m[0] = m[1] = m[2] = m[3] = bboard::Move::RIGHT;

        bboard::Step(s, m);
        REQUIRE_AGENT(s, 0, 0, 0);
        REQUIRE_AGENT(s, 1, 1, 0);
        REQUIRE_AGENT(s, 2, 2, 0);
        REQUIRE_AGENT(s, 3, 3, 0);
    }
    SECTION("Two On One")
    {
        /* For clarity:
         * 0 -> 2 <- 1
         *      |
         *      3
         */

        s->PutAgent(0, 0, 0);
        s->PutAgent(2, 0, 1);
        s->PutAgent(1, 0, 2);
        s->PutAgent(1, 1, 3);

        m[0] = bboard::Move::RIGHT;
        m[1] = bboard::Move::LEFT;
        m[2] = m[3] = bboard::Move::DOWN;

        bboard::Step(s, m);
        REQUIRE_AGENT(s, 0, 0, 0);
        REQUIRE_AGENT(s, 1, 2, 0);
        REQUIRE_AGENT(s, 2, 1, 1);
        REQUIRE_AGENT(s, 3, 1, 2);
    }
}

void REQUIRE_OUROBOROS_MOVED(bboard::State* s, bool moved)
{
    if(moved)
    {
        REQUIRE_AGENT(s, 3, 0, 0);
        REQUIRE_AGENT(s, 0, 1, 0);
        REQUIRE_AGENT(s, 1, 1, 1);
        REQUIRE_AGENT(s, 2, 0, 1);
    }
    else
    {
        REQUIRE_AGENT(s, 0, 0, 0);
        REQUIRE_AGENT(s, 1, 1, 0);
        REQUIRE_AGENT(s, 2, 1, 1);
        REQUIRE_AGENT(s, 3, 0, 1);
    }
}

TEST_CASE("Ouroboros", "[step function]")
{
    auto sx = std::make_unique<bboard::State>();
    bboard::State* s = sx.get();

    s->PutAgent(0, 0, 0);
    s->PutAgent(1, 0, 1);
    s->PutAgent(1, 1, 2);
    s->PutAgent(0, 1, 3);

    bboard::Move m[4];
    m[0] = bboard::Move::RIGHT;
    m[1] = bboard::Move::DOWN;
    m[2] = bboard::Move::LEFT;
    m[3] = bboard::Move::UP;

    SECTION("Move Ouroboros")
    {
        bboard::Step(s, m);
        REQUIRE_OUROBOROS_MOVED(s, true);
    }
    SECTION("Ouroboros with bomb")
    {
        // when player 0 plants a bomb, no player can move
        s->TryPlantBomb<false>(s->agents[0], 0);
        bboard::Step(s, m);
        REQUIRE_OUROBOROS_MOVED(s, false);
    }
    SECTION("Ouroboros with bomb kick")
    {
        // when player 1 plants a bomb and player 0 can kick it
        // then we can move
        s->TryPlantBomb<false>(s->agents[1], 0);
        s->agents[0].canKick = true;
        bboard::Step(s, m);
        REQUIRE_OUROBOROS_MOVED(s, true);
    }
    for(Item i : {Item::WOOD, Item::RIGID, Item::EXTRABOMB, Item::INCRRANGE, Item::KICK}){
        SECTION("Ouroboros with bomb kick - 2 - " + std::to_string((int)i))
        {
            // does not work when the movement is blocked by something
            s->TryPlantBomb<false>(s->agents[1], 0);
            s->agents[0].canKick = true;
            s->PutItem(2, 0, i);
            bboard::Step(s, m);
            REQUIRE_OUROBOROS_MOVED(s, false);
        }
    }
    SECTION("Ouroboros with bomb kick - 3")
    {
        // also works vertically
        s->TryPlantBomb<false>(s->agents[2], 0);
        s->agents[1].canKick = true;
        bboard::Step(s, m);
        REQUIRE_OUROBOROS_MOVED(s, true);
    }
    SECTION("Ouroboros with bomb kick - 4")
    {
        // doesn't work for players 0 and 3 because the bomb cannot
        // be kicked out of bounds
        s->TryPlantBomb<false>(s->agents[0], 0);
        s->agents[3].canKick = true;
        bboard::Step(s, m);
        REQUIRE_OUROBOROS_MOVED(s, false);
    }
    SECTION("Ouroboros all bombs")
    {
        // when everybody plants bombs in the step function
        m[0] = m[1] = m[2] = m[3] = bboard::Move::BOMB;
        bboard::Step(s, m);

        // nobody can move
        m[0] = bboard::Move::RIGHT;
        m[1] = bboard::Move::DOWN;
        m[2] = bboard::Move::LEFT;
        m[3] = bboard::Move::UP;
        bboard::Step(s, m);

        REQUIRE_OUROBOROS_MOVED(s, false);
    }
}


TEST_CASE("Bomb Mechanics", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    SECTION("Standard Bomb Laying")
    {
        s->PutAgentsInCorners(0, 1, 2, 3, 0);
        m[0] = bboard::Move::BOMB;
        bboard::Step(s.get(), m);
        REQUIRE(s->items[0][0] == bboard::Item::AGENT0);

        m[0] = bboard::Move::DOWN;
        bboard::Step(s.get(), m);
        REQUIRE(s->items[0][0] == bboard::Item::BOMB);
    }
    SECTION("Bomb Movement Block Simple")
    {
        s->PutAgentsInCorners(0, 1, 2, 3, 0);
        PlantBomb(s.get(), 1, 0, 0);

        m[0] = bboard::Move::RIGHT;
        bboard::Step(s.get(), m);
        REQUIRE_AGENT(s.get(), 0, 0, 0);
    }
    SECTION("Bomb Movement Block Complex")
    {
        s->PutAgent(0, 0, 0);
        s->PutAgent(1, 0, 1);
        s->PutAgent(2, 0, 2);
        s->PutAgent(3, 0, 3);

        m[0] = m[1] = m[2] = bboard::Move::RIGHT;
        m[3] = bboard::Move::BOMB;
        bboard::Step(s.get(), m);
        REQUIRE_AGENT(s.get(), 0, 0, 0);
        REQUIRE_AGENT(s.get(), 1, 1, 0);
        REQUIRE_AGENT(s.get(), 2, 2, 0);

        m[0] = m[1] = m[2] = bboard::Move::IDLE;
        m[3] = bboard::Move::RIGHT;
        bboard::Step(s.get(), m);
        REQUIRE_AGENT(s.get(), 3, 4, 0);
    }
}


TEST_CASE("Bomb Explosion", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    s->Kill(2,3);
    s->PutAgent(5, 5, 0); //

    SECTION("Bomb Goes Off Correctly")
    {
        m[0] = bboard::Move::BOMB;
        bboard::Step(s.get(), m);

        m[0] = bboard::Move::UP;
        SeveralSteps(bboard::BOMB_LIFETIME - 1, s.get(), m);

        REQUIRE(s->items[5][5] == bboard::Item::BOMB);
        bboard::Step(s.get(), m);
        REQUIRE(IS_FLAME(s->items[5][5]));
    }
    SECTION("Destroy Objects and Agents")
    {
        s->PutItem(6, 5, bboard::Item::WOOD);
        s->PutAgent(4, 5, 1);

        m[0] = bboard::Move::BOMB;
        bboard::Step(s.get(), m);

        m[0] = bboard::Move::UP;
        SeveralSteps(bboard::BOMB_LIFETIME, s.get(), m);

        REQUIRE(s->agents[1].dead);
        REQUIRE(IS_FLAME(s->items[5][4]));
        REQUIRE(IS_FLAME(s->items[5][6]));
    }
    SECTION("Keep Rigid")
    {
        s->PutItem(6, 5, bboard::Item::RIGID);

        m[0] = bboard::Move::BOMB;
        bboard::Step(s.get(), m);

        m[0] = bboard::Move::UP;
        SeveralSteps(bboard::BOMB_LIFETIME, s.get(), m);

        REQUIRE(s->items[5][6] == bboard::Item::RIGID);
    }
    SECTION("Kill Only 1 Wood (1 Bomb)")
    {
        s->PutItem(7, 5, bboard::Item::WOOD);
        s->PutItem(8, 5, bboard::Item::WOOD);

        s->agents[0].bombStrength = 5;
        PlantBomb(s.get(), 6, 5, 0, true);
        SeveralSteps(bboard::BOMB_LIFETIME, s.get(), m);

        REQUIRE(IS_FLAME(s->items[5][7]));
        REQUIRE(!IS_FLAME(s->items[5][8]));
    }
    SECTION("Kill Only 1 Wood (2 Bombs)")
    {
        s->PutItem(9, 6, bboard::Item::WOOD);
        s->PutItem(8, 6, bboard::Item::WOOD);

        s->agents[0].maxBombCount = 2;
        s->agents[0].bombStrength = 5;
        PlantBomb(s.get(), 7, 6, 0, true);
        PlantBomb(s.get(), 6, 6, 0, true);

        SeveralSteps(bboard::BOMB_LIFETIME, s.get(), m);

        // first wood is destroyed
        REQUIRE(IS_FLAME(s->items[6][8]));
        // second wood stays
        REQUIRE(IS_WOOD(s->items[6][9]));
        // both bombs exploded
        REQUIRE(IS_FLAME(s->items[6][6]));
        REQUIRE(IS_FLAME(s->items[6][7]));
    }
    SECTION("Max Agent Bomb Limit")
    {
        s->agents[0].maxBombCount = 2;
        REQUIRE(s->agents[0].bombCount == 0);

        PlaceBombsHorizontally(s.get(), 0, 4); //place 1 over max
        REQUIRE(s->items[5][5] == bboard::Item::BOMB);
        REQUIRE(s->items[5][6] == bboard::Item::BOMB);
        REQUIRE(s->items[5][7] == bboard::Item::PASSAGE);

        REQUIRE(s->agents[0].bombCount == 2);
    }
}

TEST_CASE("Bomb Explosion - Special Cases", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    for(int order = 0; order < 2; order++)
    {
        SECTION("No chain explosion because of bomb movement - order=" + std::to_string(order))
        {
            s->Kill(2, 3);
            PlantBomb(s.get(), 0, 5, 0, true);
            s->PutAgent(1, 0, 0);
            s->PutAgent(2, 0, 1);
            s.get()->agents[0].maxBombCount = 2;
            s.get()->agents[0].canKick = true;
            s.get()->agents[1].canKick = true;

            for(int i = 0; i < BOMB_LIFETIME - 4; i++)
            {
                Step(s.get(), m);
            }

            // assume there are bombs in front of the agents
            // different planting order should not effect the outcome
            switch (order) {
                case 0:
                    PlantBomb(s.get(), 1, 1, 0, true);
                    PlantBomb(s.get(), 2, 1, 1, true);
                    break;
                case 1:
                    PlantBomb(s.get(), 2, 1, 0, true);
                    PlantBomb(s.get(), 1, 1, 1, true);
                    break;
            }

            // agent 1 kicks it
            m[1] = Move::DOWN;
            Step(s.get(), m);

            // then agent 0 kicks it
            m[0] = Move::DOWN;
            m[1] = Move::IDLE;
            Step(s.get(), m);

            m[0] = Move::IDLE;

            // the bomb should explode after two additional steps
            for(int i = 0; i < 2; i++)
            {
                Step(s.get(), m);
            }

            // and our bombs reach that explosion in the next step
            Step(s.get(), m);

            // but the first kicked bomb should pass the explosion
            REQUIRE(IS_FLAME(s.get()->items[5][0 + BOMB_DEFAULT_STRENGTH]));
            REQUIRE(IS_FLAME(s.get()->items[5][1 + BOMB_DEFAULT_STRENGTH]));
            REQUIRE(!IS_FLAME(s.get()->items[5][2 + BOMB_DEFAULT_STRENGTH]));
            REQUIRE(s.get()->GetBombIndex(2, 6) != -1);
        }
    }
}

TEST_CASE("Flame Mechanics", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};
    s->PutAgentsInCorners(0, 1, 2, 3, 0);


    SECTION("Correct Lifetime Calculation")
    {
        s->SpawnFlames(5,5,4);
        bboard::Step(s.get(), m);

        SeveralSteps(bboard::FLAME_LIFETIME - 2, s.get(), m);
        REQUIRE(IS_FLAME(s->items[5][5]));
        bboard::Step(s.get(), m);
        REQUIRE(!IS_FLAME(s->items[5][5]));
    }
    SECTION("Vanish Flame Completely")
    {
        s->SpawnFlames(5,5,4);
        bboard::Step(s.get(), m);

        for(int i = 0; i <= 4; i++)
        {
            REQUIRE(IS_FLAME(s->items[5][5 + i]));
            REQUIRE(IS_FLAME(s->items[5][5 - i]));
            REQUIRE(IS_FLAME(s->items[5 + i][5]));
            REQUIRE(IS_FLAME(s->items[5 - i][5]));
        }
    }
    SECTION("Only Vanish Your Own Flame")
    {
        s->SpawnFlames(5,5,4);
        bboard::Step(s.get(), m);

        s->SpawnFlames(6, 6, 4);
        SeveralSteps(bboard::FLAME_LIFETIME - 1, s.get(), m);

        REQUIRE(IS_FLAME(s->items[5][6]));
        REQUIRE(IS_FLAME(s->items[6][5]));
        REQUIRE(!IS_FLAME(s->items[5][5]));

        bboard::Step(s.get(), m);

        REQUIRE(!IS_FLAME(s->items[5][6]));
        REQUIRE(!IS_FLAME(s->items[6][5]));
    }

    SECTION("Only Vanish Your Own Flame II")
    {
        s->SpawnFlames(5, 5, 4);
        bboard::Step(s.get(), m);

        REQUIRE(IS_FLAME(s->items[1][5]));
        REQUIRE(IS_FLAME(s->items[2][5]));

        s->SpawnFlames(5, 6, 4);

        SeveralSteps(bboard::FLAME_LIFETIME - 1, s.get(), m);

        REQUIRE(!IS_FLAME(s->items[1][5]));
        REQUIRE(IS_FLAME(s->items[6][5]));
        REQUIRE(IS_FLAME(s->items[2][5]));
        REQUIRE(IS_FLAME(s->items[7][5]));

        bboard::Step(s.get(), m);

        REQUIRE(!IS_FLAME(s->items[2][5]));
    }
}

TEST_CASE("Chained Explosions", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};
    /*
     * Note two ways to plant bombs in this test
     * PlantBomb  =>  Bomb gets ticked
     * Set Move to Bomb  =>  Bomb doesn't get ticked
     */

    SECTION("Two Bombs")
    {
        s->PutAgentsInCorners(0, 1, 2, 3, 0);
        PlantBomb(s.get(), 5, 5, 0, true);
        bboard::Step(s.get(), m);
        PlantBomb(s.get(), 4, 5, 1, true);
        SeveralSteps(bboard::BOMB_LIFETIME - 1, s.get(), m);
        REQUIRE(s->bombs.count == 0);
        REQUIRE(IS_FLAME(s->items[5][6]));
    }
    SECTION("Two Bombs Covered By Agent")
    {
        s->PutAgent(5, 5, 0);
        s->PutAgent(4, 5, 1);
        s->Kill(2, 3);
        m[0] = bboard::Move::BOMB;
        bboard::Step(s.get(), m);

        m[1] = bboard::Move::BOMB;
        bboard::Step(s.get(), m);

        m[0] = m[1] = bboard::Move::DOWN;

        SeveralSteps(bboard::BOMB_LIFETIME - 2, s.get(), m);

        REQUIRE(s->bombs.count == 2);
        bboard::Step(s.get(), m);
        REQUIRE(s->bombs.count == 0);
        REQUIRE(s->flames.count == 8);
    }
}

TEST_CASE("Bomb Kick Mechanics", "[step function]")
{
    auto s = std::make_unique<bboard::State>();
    bboard::Move id = bboard::Move::IDLE;
    bboard::Move m[4] = {id, id, id, id};

    s->PutAgent(0, 1, 0);
    s->agents[0].canKick = true;
    PlantBomb(s.get(), 1, 1, 0, true);
    s->agents[0].maxBombCount = bboard::MAX_BOMBS_PER_AGENT;
    m[0] = bboard::Move::RIGHT;

    SECTION("One Agent - One Bomb")
    {
        s->Kill(1, 2, 3);
        bboard::Step(s.get(), m);

        REQUIRE_AGENT(s.get(), 0, 1, 1);
        REQUIRE(s->items[1][2] == bboard::Item::BOMB);

        for(int i = 0; i < 4; i++)
        {
            REQUIRE(s->items[1][2 + i] == bboard::Item::BOMB);
            bboard::Step(s.get(), m);
            m[0] = bboard::Move::IDLE;
        }
    }
    SECTION("Bomb kicked against Flame")
    {
        s->Kill(1, 2, 3);
        s->PutItem(5, 1, bboard::Item::FLAME);

        bboard::Step(s.get(), m);
        m[0] = bboard::Move::IDLE;

        SeveralSteps(3,  s.get(),  m);

        REQUIRE(IS_FLAME(s->items[1][5]));
        REQUIRE(s->bombs.count == 0);
        REQUIRE(s->flames.count == 5);
        REQUIRE(s->flames[0].position == bboard::Position({5,1}));
    }
    SECTION("Bomb - Bomb Collision")
    {
        s->Kill(1, 2, 3);
        PlantBomb(s.get(), 7, 7, 0, true);
        bboard::SetBombDirection(s->bombs[1], bboard::Direction::UP);

        for(int i = 0; i < 6; i++)
        {
            bboard::Step(s.get(), m);
            m[0] = bboard::Move::IDLE;
        }

        REQUIRE(BMB_POS_X(s->bombs[0]) == 6);
        REQUIRE(BMB_POS_X(s->bombs[1]) == 7);
        REQUIRE(BMB_POS_Y(s->bombs[1]) == 2);
    }

    SECTION("Bomb - Bomb - Static collision")
    {
        s->Kill(1, 2, 3);
        PlantBomb(s.get(), 7, 6, 0, true);
        s->PutItem(7, 0, bboard::Item::WOOD);
        bboard::SetBombDirection(s->bombs[1], bboard::Direction::UP);
        for(int i = 0; i < 7; i++)
        {
            bboard::Step(s.get(), m);
            m[0] = bboard::Move::IDLE;
        }

        REQUIRE(BMB_POS_X(s->bombs[0]) == 6);
        REQUIRE(BMB_POS_X(s->bombs[1]) == 7);
        REQUIRE(BMB_POS_Y(s->bombs[1]) == 1);
    }
    SECTION("Bounce Back Agent")
    {
        s->Kill(2, 3);
        s->PutAgent(0, 2, 1);
        m[1] = Move::UP;
        PlantBomb(s.get(), 2, 2, 0, true);
        bboard::SetBombDirection(s->bombs[1], bboard::Direction::UP);
        bboard::Step(s.get(), m);

        REQUIRE_AGENT(s.get(), 0, 0, 1);
        REQUIRE_AGENT(s.get(), 1, 0, 2);
        REQUIRE(BMB_POS_X(s->bombs[0]) == 1);
        REQUIRE(BMB_POS_X(s->bombs[1]) == 2);
    }
    SECTION("Bounce Back Complex Chain")
    {
        s->Kill(2, 3);
        s->PutAgent(0, 2, 1);
        m[1] = Move::UP;
        PlantBomb(s.get(), 2, 2, 0, true);
        PlantBomb(s.get(), 0, 3, 0, true);
        bboard::SetBombDirection(s->bombs[1], bboard::Direction::UP);
        bboard::SetBombDirection(s->bombs[2], bboard::Direction::UP);

        bboard::Step(s.get(), m);

        REQUIRE_AGENT(s.get(), 0, 0, 1);
        REQUIRE_AGENT(s.get(), 1, 0, 2);
        REQUIRE(s->items[3][0] == Item::BOMB);
        REQUIRE(s->items[1][1] == Item::BOMB);
        REQUIRE(s->items[2][2] == Item::BOMB);
    }
    SECTION("Bounce Back Super Complex Chain")
    {
        s->Kill(3);
        s->PutAgent(0, 2, 1);
        s->PutAgent(1, 3, 2);
        s->PutItem(2, 1, Item::RIGID);
        m[1] = Move::UP;
        m[2] = Move::BOMB;
        PlantBomb(s.get(), 0, 3, 0, true);
        bboard::SetBombDirection(s->bombs[1], bboard::Direction::UP);

        for(int i = 0; i < 3; i++)
        {
            // bboard::PrintState(s.get(), false);
            //std::cin.get();
            bboard::Step(s.get(), m);
            m[0] = m[1] = bboard::Move::IDLE;
            m[2] = Move::LEFT;
        }
    }
    SECTION("Bounce Back Wall")
    {
        s->Kill(1, 3);
        s->PutAgent(1, 3, 2);
        s->PutItem(2, 1, Item::RIGID);
        m[2] = Move::LEFT;
        s->agents[2].canKick = true;
        PlantBomb(s.get(), 0, 3, 0, true);
        bboard::Step(s.get(), m);

        REQUIRE_AGENT(s.get(), 2, 1, 3);
        REQUIRE(s->items[3][0] == Item::BOMB);
    }
    SECTION("Stepping on bombs") // Provided by M?rton G?r?g
    {
        s->PutAgent(6, 3, 0);
        s->PutAgent(6, 4, 1);
        s->PutAgent(6, 5, 2);
        m[0] = m[1] = m[2] = bboard::Move::IDLE;

        PlantBomb(s.get(), 5, 6, 3, true);
        PlantBomb(s.get(), 6, 6, 2, true);
        s->PutAgent(6, 6, 3);

        m[3] = bboard::Move::IDLE;
        bboard::Step(s.get(), m);
        REQUIRE_AGENT(s.get(), 3, 6, 6);

        m[3] = bboard::Move::LEFT;
        bboard::Step(s.get(), m);
        REQUIRE_AGENT(s.get(), 3, 6, 6);
    }
    for(bool b : {true, false})
    {
        SECTION("Undo Kick " + std::to_string(b))
        {
            s->Kill(2, 3);
            s->PutAgent(1, 1, 1);

            // agent planted bomb at its own position
            // and one space below
            s->agents[1].maxBombCount = 2;
            PlantBomb(s.get(), 1, 1, 1, true);
            PlantBomb(s.get(), 1, 2, 1, true);
            // 0 1 <- there is a bomb below agent 1
            //   b

            // agent 0 wants to move rightwards
            // agent 1 wants to move downwards
            m[1] = Move::DOWN;

            // agent 0 can kick in both cases
            if(b)
            {
                s->agents[1].canKick = false;
            }
            else
            {
                s->agents[1].canKick = true;
            }

            bboard::Step(s.get(), m);

            if(b)
            {
                // because agent 1 cannot kick, we have to undo
                // everything

                // bombs stay (especially the kicked bomb 0)
                REQUIRE(BMB_POS(s->bombs[0]) == (Position){1, 1});
                REQUIRE(BMB_POS(s->bombs[1]) == (Position){1, 2});
                // ..because agents did not move
                REQUIRE_AGENT(s.get(), 0, 0, 1);
                REQUIRE_AGENT(s.get(), 1, 1, 1);
            }
            else
            {
                // because agent 1 can kick, we can apply every move

                // bombs moved
                REQUIRE(BMB_POS(s->bombs[0]) == (Position){2, 1});
                REQUIRE(BMB_POS(s->bombs[1]) == (Position){1, 3});
                // ..because agents moved as well
                REQUIRE_AGENT(s.get(), 0, 1, 1);
                REQUIRE_AGENT(s.get(), 1, 1, 2);
            }
        }
    }
}
