#ifndef RANDOM_AGENT_H
#define RANDOM_AGENT_H

#include <random>

#include "bboard.hpp"
#include "strategy.hpp"

namespace agents
{

/**
 * Use this as an example to implement more sophisticated
 * agents.
 *
 * @brief Randomly selects actions
 */
struct RandomAgent : bboard::Agent
{
    std::mt19937_64 rng;
    std::uniform_int_distribution<int> intDist;

    RandomAgent();

    bboard::Move act(const bboard::State* state) override;
};


/**
 * @brief Randomly selects actions that are not laying bombs
 */
struct HarmlessAgent : bboard::Agent
{
    std::mt19937_64 rng;
    std::uniform_int_distribution<int> intDist;

    HarmlessAgent();

    bboard::Move act(const bboard::State* state) override;
};

/**
 * @brief Selects Idle for every action
 */
struct LazyAgent : bboard::Agent
{
    bboard::Move act(const bboard::State* state) override;
};



/**
 * @brief Handcrafted agent by m2q
 */
struct SimpleAgent : bboard::Agent
{
    std::mt19937_64 rng;

    SimpleAgent();
    SimpleAgent(long seed);

    //////////////
    // Specific //
    //////////////
    int danger = 0;
    bboard::strategy::RMap r;
    bboard::FixedQueue<bboard::Move, bboard::MOVE_COUNT> moveQueue;

    // capacity of recent positions
    static const int rpCapacity = 4;
    bboard::FixedQueue<bboard::Position, rpCapacity> recentPositions;

    virtual bboard::Move decide(const bboard::State* state);
    bboard::Move act(const bboard::State* state) override;

    void reset() override;

    void PrintDetailedInfo();
};

/**
 * @brief An improved version of SimpleAgent. Adds more randomization to get equal win rates and collects powerups.
 * Also includes adjustments of parameters to (hopefully) result in better gameplay.
 */
struct SimpleUnbiasedAgent : SimpleAgent
{
    std::array<int, 4> agentAxis;
    std::array<bboard::Move, bboard::MOVE_COUNT> dirAxis;
    std::array<int, bboard::BOARD_SIZE> boardAxisX;
    std::array<int, bboard::BOARD_SIZE> boardAxisY;

    SimpleUnbiasedAgent();
    SimpleUnbiasedAgent(long seed);

    bboard::Move decide(const bboard::State* state) override;
    void reset() override;
};

}

#endif
