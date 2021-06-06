#include "catch.hpp"
#include "bboard.hpp"

using namespace bboard;

void REQUIRE_SAME_ITEMS_EXCEPT_AGENT(bboard::State* a, bboard::State* b)
{
    for (int y = 0; y < bboard::AGENT_COUNT; y++) {
        for (int x = 0; x < bboard::AGENT_COUNT; x++) {
            int itemA = a->items[y][x];
            int itemB = b->items[y][x];

            if (bboard::IS_AGENT(itemA)) {
                REQUIRE(bboard::IS_AGENT(itemB));
            }
            else {
                REQUIRE(itemA == itemB);
            }
        }
    }
}

TEST_CASE( "Board Agent Placement", "[board creation]" )
{
    for(int envSeed : {7, 9, 42, 100})
    {
        SECTION("Env Seed (" + std::to_string(envSeed) + ")")
        {
            // generate an environment with default agent placement
            State s;
            s.Init(GameMode::FreeForAll, envSeed, -1);

            // generate environments with seed-based agent placement
            State s2;
            std::mt19937 rng(envSeed);
            for (int i = 0; i < 10; i++) {
                s2.Init(GameMode::FreeForAll, envSeed, rng());

                // the board should look exactly the same, except for the agents
                REQUIRE_SAME_ITEMS_EXCEPT_AGENT(&s, &s2);
            }
        }
    }
}
