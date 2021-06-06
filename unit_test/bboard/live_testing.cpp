#include "catch.hpp"
#include "colors.hpp"
#include "bboard.hpp"
#include "agents.hpp"
#include "strategy.hpp"
#include <chrono>

#include "testing_utilities.hpp"

using namespace agents;

#define BASELINE_AGENT agents::SimpleAgent
#define IMPROVED_AGENT agents::SimpleUnbiasedAgent

#define BASELINE_NAME std::string(type_name<BASELINE_AGENT>())
#define IMPROVED_NAME std::string(type_name<IMPROVED_AGENT>())

template <int AGENT>
void PrintAgentInfo(const bboard::Environment& env)
{
    SimpleAgent& a = *static_cast<SimpleAgent*>(env.GetAgent(AGENT));
    std::cout << std::endl;
    // bboard::strategy::PrintMap(a.r);
    a.PrintDetailedInfo();
}

void REQUIRE_CORRECT_RESULT_FFA(bboard::Environment& env)
{
    if(env.IsDone())
    {
        // no winning team
        REQUIRE(env.GetWinningTeam() == 0);

        int winningAgent = env.GetWinningAgent();
        if(winningAgent != -1)
        {
            // either exactly one winner
            REQUIRE(env.GetState().agents[winningAgent].won == true);
            for(int i = 0; i < bboard::AGENT_COUNT; i++)
            {
                if(i == winningAgent) continue;

                REQUIRE(env.GetState().agents[i].won == false);
            }
        }
        else
        {
            // or no winners
            REQUIRE(env.IsDraw() == true);
        }
    }
    else
    {
        // at least two agents are still alive
        REQUIRE(env.GetState().aliveAgents >= 2);
    }
}

void REQUIRE_CORRECT_RESULT_TEAMS(bboard::Environment& env)
{
    if(env.IsDone())
    {
        // there is no single winning agent, we only consider teams
        REQUIRE(env.GetWinningAgent() == -1);

        int winningTeam = env.GetWinningTeam();
        if(winningTeam != 0)
        {
            for(int i = 0; i < bboard::AGENT_COUNT; i++)
            {
                bboard::AgentInfo info = env.GetState().agents[i];
                // an agent has won iff he is in the winning team
                REQUIRE(info.won == (info.team == winningTeam));
            }
        }
        else
        {
            // or no winners
            REQUIRE(env.IsDraw() == true);
        }
    }
    else
    {
        // at least two agents are still alive
        REQUIRE(env.GetState().aliveAgents >= 2);

        int team0 = 0;
        bool foundTwoTeams = false;
        // they have to be in two different teams
        for(int i = 0; i < bboard::AGENT_COUNT; i++)
        {
            bboard::AgentInfo info = env.GetState().agents[i];
            if(!info.dead && info.team != 0)
            {
                if (team0 == 0)
                {
                    team0 = info.team;
                    continue;
                }
                else if(team0 != info.team)
                {
                    foundTwoTeams = true;
                    break;
                }
            }
        }

        REQUIRE(foundTwoTeams);
    }
}

TEST_CASE("Test Baseline Agent", "[live testing]")
{
    int numGames = 100;

    int seed = 42;
    std::mt19937 rng(seed);

    std::string tst = "Testing " + std::to_string(numGames) + " games with " + BASELINE_NAME;
    std::cout << std::endl << FGRN(tst) << std::endl;

    SECTION("Free For All")
    {
        std::cout << "Mode \"FreeForAll\"... " << std::flush;
        for(int i = 0; i < numGames; i++)
        {
            auto agents = CreateAgents(rng);

            // create an environment
            bboard::Environment e;

            // initializes the game/board/agents
            e.MakeGame(ToPointerArray(agents), bboard::GameMode::FreeForAll, (int)rng());

            // starts the game with the specified params
            e.RunGame(800, false, false);

            REQUIRE_CORRECT_RESULT_FFA(e);
        }
        std::cout << "done." << std::endl;
    }
    SECTION("Teams")
    {
        std::cout << "Mode \"TwoTeams\"... " << std::flush;
        for(int i = 0; i < numGames; i++)
        {
            auto agents = CreateAgents(rng);

            // create an environment
            bboard::Environment e;

            // initializes the game/board/agents
            e.MakeGame(ToPointerArray(agents), bboard::GameMode::TwoTeams, (int)rng());

            // starts the game with the specified params
            e.RunGame(800, false, false);

            REQUIRE_CORRECT_RESULT_TEAMS(e);
        }
        std::cout << "done." << std::endl;
    }
}

void playGames(std::array<bboard::Agent*, 4> agents, bboard::GameMode gameMode, int numGames, std::mt19937 rng, bool useRandomAgentPositions = false, int steps = 800) 
{
    int wins[bboard::AGENT_COUNT];
    std::fill_n(wins, bboard::AGENT_COUNT, 0);
    int draws = 0;
    int notDone = 0;
    double averageSteps = 0;

    std::string startPosStr = useRandomAgentPositions ? "rand start" : "fixed start";
    std::string progressPrefix = "> Collecting stats for " + std::to_string(numGames) + " live games (" + startPosStr + ")... ";
    std::cout << progressPrefix << std::flush;

    auto tStart = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < numGames; i++)
    {
        bboard::Environment e;
        long randomAgentPositionSeed = useRandomAgentPositions ? rng() : -1;
        e.MakeGame(agents, gameMode, rng(), randomAgentPositionSeed);
        e.RunGame(steps, false, false);

        bboard::State finalState = e.GetState();

        if (i == 0) {
            averageSteps = finalState.timeStep;
        }
        else {
            averageSteps = (i * averageSteps + finalState.timeStep) / (i + 1);
        }

        int winner = finalState.winningAgent;
        if (winner != -1) {
            wins[winner]++;
        }
        notDone += finalState.finished ? 0 : 1;
        draws += finalState.isDraw ? 1 : 0;

#ifdef TEST_SHOW_GAME_PROGRESS
        std::cout  << "\r" << progressPrefix << (int)((float)(i+1) / numGames * 100) << "%" << std::flush;
#endif
    }
    std::cout << std::endl;
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<float>(tEnd - tStart).count();

    std::cout << "Episodes: " << numGames << std::endl;
    std::cout << "Average steps: " << averageSteps << std::endl;
    std::cout << "Wins:" << std::endl;
    for (int i = 0; i < bboard::AGENT_COUNT; i++) {
        std::cout << "> Agent " << i << ": " << wins[i] << " (" << (float)wins[i] / numGames * 100 << "%)" << std::endl;
    }
    std::cout << "Draws: " << draws <<  " (" << (float)draws / numGames * 100 << "%)" << std::endl;
    std::cout << "Not done: " << notDone <<  " (" << (float)notDone / numGames * 100 << "%)" << std::endl;
    std::cout << "Elapsed time: " << duration << "[s]" << std::endl;
}

TEST_CASE("Win Rate Stats", "[agent statistics]")
{
    int numGames = 10000;

    int seed = 42;
    std::mt19937 rng(seed);

    auto baseline = CreateAgents<BASELINE_AGENT>(rng);
    auto improved = CreateAgents<IMPROVED_AGENT>(rng);

    for (auto i : {0, 1})
    {
        std::array<bboard::Agent*, 4> agentPointers;
        std::string agentType;

        switch (i)
        {
        case 0:
            agentType = BASELINE_NAME;
            agentPointers = ToPointerArray(baseline);
            break;

        case 1:
            agentType = IMPROVED_NAME;
            agentPointers = ToPointerArray(improved);
            break;
        
        default:
            throw "Internal test error.";
            break;
        }

        for(bool useRandomAgentPos : {false, true})
        {
            std::string testName = "Free For All (" + agentType + ") - Random Start Positions = " + std::to_string(useRandomAgentPos);
            SECTION(testName)
            {
                std::cout << std::endl << FGRN(testName) << std::endl;
                playGames(agentPointers, bboard::GameMode::FreeForAll, numGames, rng, useRandomAgentPos);
            }
        }
    }
}

TEST_CASE("Baseline Agent vs. Improved Agent", "[agent statistics]")
{
    int seed = 7;
    std::mt19937 rng(seed);

    BASELINE_AGENT b1 = BASELINE_AGENT(rng());
    BASELINE_AGENT b2 = BASELINE_AGENT(rng());
    IMPROVED_AGENT i1 = IMPROVED_AGENT(rng());
    IMPROVED_AGENT i2 = IMPROVED_AGENT(rng());
    std::array<bboard::Agent*, 4> agents = {&b1, &b2, &i1, &i2};

    std::string txt = "Baseline Agent (" + BASELINE_NAME + ") {0, 1} vs. Improved Agent (" + IMPROVED_NAME + ") {2, 3}";
    std::cout << std::endl << FGRN(txt) << std::endl;
    playGames(agents, bboard::GameMode::FreeForAll, 10000, rng, true);
}
