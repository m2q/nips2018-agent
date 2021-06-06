#include <cmath>
#include <thread>
#include <future>
#include <chrono>
#include <utility>
#include <iostream>

#include "catch.hpp"
#include "testing_utilities.hpp"

#include "bboard.hpp"
#include "agents.hpp"
#include "colors.hpp"

using bboard::FixedQueue;

template<typename F, typename... Args>
double timeMethod(int times, F func, Args&&... args)
{
    std::chrono::duration<double, std::milli> total;
    auto t1 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < times; i++)
    {
        func(std::forward<Args>(args)...);
    }
    total = std::chrono::high_resolution_clock::now() - t1;
    return total.count();
}

void Step(bboard::Environment& s)
{
    s.Step();
}

#define TESTING_AGENT agents::SimpleAgent

void ProxyConcurrent(std::promise<int> && steps, int times)
{
    TESTING_AGENT a;
    bboard::Environment env;
    env.MakeGame({&a, &a, &a, &a});
    for(int i = 0; i < times && !env.IsDone(); i++)
    {
        env.Step();
    }
    steps.set_value(env.GetState().timeStep);
}

TEST_CASE("Step Function", "[performance]")
{
    TESTING_AGENT b;
    int times = 1000;
    double t = -1;
    int totalSteps = 0;

    for(int _ = 0; _ < 10; _++)
    {
        TESTING_AGENT a[4];
        bboard::Environment env;
        env.MakeGame({&a[0], &a[1], &a[2], &a[3]});
        if(!THREADING)
        {
            t += timeMethod(times, Step, env);
            totalSteps += env.GetState().timeStep; //update the amount
        }
        else
        {
            std::vector<std::thread> threads(THREAD_COUNT);

            std::chrono::duration<double, std::milli> total;
            auto t1 = std::chrono::high_resolution_clock::now();

            std::promise<int> p[THREAD_COUNT];
            std::future<int> f[THREAD_COUNT];

            for(uint i = 0; i < THREAD_COUNT; i++)
            {
                f[i] = p[i].get_future();
                threads[i] = std::thread(ProxyConcurrent, std::move(p[i]), times);
            }

            // join all
            for(uint i = 0; i < THREAD_COUNT; i++)
            {
                threads[i].join();
                totalSteps += f[i].get();
            }

            total = std::chrono::high_resolution_clock::now() - t1;
            t += total.count();
        }
    }
    t /= 10;
    totalSteps /= 10;

    std::string tst = "Test Results:\n";
    std::cout << std::endl
              << FGRN(tst)
              << "Iteration count (100ms):         ";

    RecursiveCommas(std::cout, uint(std::floor(totalSteps/(t/100.0))));

    std::cout << std::endl
              << "Tested with:                     "
              << type_name<decltype(b)>()
              << "\nTime: " << t/100.0 << "\n";

    REQUIRE(1);
}

TEST_CASE("Episode Steps", "[performance]")
{
    int numEpisodes = 2000;
    int totalSteps = 0;

    int seed = 42;
    std::mt19937 rng(seed);

    double elapsedMs = 0;

    for(int i = 0; i < numEpisodes; i++)
    {
        std::array<SimpleAgent, 4> r = CreateAgents<SimpleAgent>(rng);

        // create an environment
        bboard::Environment e;

        // initializes the game/board/agents
        e.MakeGame({&r[0], &r[1], &r[2], &r[3]}, bboard::GameMode::FreeForAll, (int)rng());

        auto t1 = std::chrono::high_resolution_clock::now();
        e.RunGame(800, false, false);
        totalSteps += e.GetState().timeStep;
        elapsedMs += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t1).count();
    }

    std::string tst = "Episode step performance results:\n";
    std::cout << std::endl
              << FGRN(tst)
              << "Episode count: " << numEpisodes
              << std::endl
              << "Step count: " << totalSteps
              << std::endl;

    std::cout << "Average step time: " << (elapsedMs / totalSteps) * 1000 << " us";

    TESTING_AGENT b;
    std::cout << std::endl
              << "Tested with:                     "
              << type_name<decltype(b)>()
              << "\nTime: " << elapsedMs << " ms\n";

    REQUIRE(1);
}
