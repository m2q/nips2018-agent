#include <random>
#include <chrono>
#include <thread>
#include <iostream>

#include "bboard.hpp"
#include "colors.hpp"

namespace bboard
{

/////////////////////////
// Auxiliary Functions //
/////////////////////////

/**
 * @brief _cleanFlameSpawnPosition Checks whether a flame can be spawned at the specified position (x, y).
 * If there exists a flame object at (x, y) with a different timeLeft, the existing flame is removed.
 *
 * @param flames The flames of the state
 * @param boardItem The board item at (x, y)
 * @param x The x coordinate
 * @param y The y coordinate
 * @param timeStep The current timestep
 * @param outSpawnFlame Returns whether we can spawn a new flame at (x, y)
 * @param outContinueFlameSpawn Returns whether the flame spawning can be continued (abort when encountering a destroyed wood block)
 */
template <int count>
void _cleanFlameSpawnPosition(FixedQueue<Flame, count>& flames, const int boardItem, const int x, const int y, const int timeStep, bool& outSpawnFlame, bool& outContinueFlameSpawn)
{
    if(IS_FLAME(boardItem))
    {
        // find the old flame object for this position
        int flameId = bboard::FLAME_ID(boardItem);
        // start from the back to save time finding the correct index
        for(int i = std::min(flames.count - 1, flameId); i >= 0; i--)
        {
            Flame& f = flames[i];
            if(f.position.x == x && f.position.y == y)
            {
                // we found the correct flame object
                if(timeStep == f.destroyedWoodAtTimeStep)
                {
                    // the existing flame destroyed some wood block at this timestep, stop here
                    outContinueFlameSpawn = false;
                    outSpawnFlame = false;
                    return;
                }

                if(f.timeLeft == FLAME_LIFETIME)
                {
                    // skip this flame, there already is a flame with the same lifetime
                    outContinueFlameSpawn = true;
                    outSpawnFlame = false;
                    return;
                }

                // the lifetime changes. Due to the ordering, we have to remove the old flame
                if(i == 0)
                {
                    flames.PopElem();
                    flames[0].timeLeft += f.timeLeft;
                }
                else
                {
                    flames[i - 1].timeLeft += f.timeLeft;
                    flames.RemoveAt(i);
                }

                outContinueFlameSpawn = true;
                outSpawnFlame = true;
                return;
            }
        }
    }

    // simply continue spawning flames
    outContinueFlameSpawn = true;
    outSpawnFlame = true;
}

/**
 * @brief IsOutOfBounds Checks wether a given position is out of bounds
 */
inline bool IsOutOfBounds(const int& x, const int& y)
{
    return x < 0 || y < 0 || x >= BOARD_SIZE || y >= BOARD_SIZE;
}

void SetTeams(AgentInfo agents[AGENT_COUNT], GameMode gameMode)
{
    switch(gameMode)
    {
        case GameMode::FreeForAll:
            for(int i = 0; i < AGENT_COUNT; i++)
            {
                agents[i].team = 0;
            }
            break;
        case GameMode::TwoTeams:
            for(int i = 0; i < AGENT_COUNT; i++)
            {
                agents[i].team = (i % 2 == 0) ? 1 : 2;
            }
            break;
        default:
            throw std::runtime_error("Unknown game mode "
                                     + std::to_string((int)gameMode));
    }
}

///////////////////
// Board Methods //
///////////////////

void Board::CopyFrom(const Board& board)
{
    std::copy_n(&board.items[0][0], BOARD_SIZE * BOARD_SIZE, &items[0][0]);
    bombs = board.bombs;
    flames = board.flames;
    timeStep = board.timeStep;
    currentFlameTime = board.currentFlameTime;
}

void Board::PlantBombModifiedLife(int x, int y, int id, int strength, int lifeTime, bool setItem)
{
    Bomb& b = bombs.NextPos();

    SetBombID(b, id);
    SetBombPosition(b, x, y);
    SetBombStrength(b, strength);
    SetBombDirection(b, Direction::IDLE);
    SetBombFlag(b, false);
    SetBombTime(b, lifeTime);

    if(setItem)
    {
        items[y][x] = Item::BOMB;
    }

    bombs.count++;
}

void Board::ExplodeBombAt(int i)
{
    Bomb b = bombs[i];

    // remove the bomb
    if (i == 0)
    {
        bombs.PopElem();
    }
    else
    {
        bombs.RemoveAt(i);
    }

    // spawn flames, this may trigger other explosions
    int x = BMB_POS_X(b);
    int y = BMB_POS_Y(b);
    SpawnFlames(x, y, BMB_STRENGTH(b));

    EventBombExploded(b);
}

bool Board::HasBomb(int x, int y) const
{
    for(int i = 0; i < bombs.count; i++)
    {
        if(BMB_POS_X(bombs[i]) == x && BMB_POS_Y(bombs[i]) == y)
        {
            return true;
        }
    }
    return false;
}

Bomb* Board::GetBomb(int x, int y)
{
    for(int i = 0; i < bombs.count; i++)
    {
        if(BMB_POS_X(bombs[i]) == x && BMB_POS_Y(bombs[i]) == y)
        {
            return &bombs[i];
        }
    }
    return nullptr;
}

int Board::GetBombIndex(int x, int y) const
{
    for(int i = 0; i < bombs.count; i++)
    {
        if(BMB_POS_X(bombs[i]) == x && BMB_POS_Y(bombs[i]) == y)
        {
            return i;
        }
    }
    return -1;
}

bool Board::SpawnFlameItem(int x, int y, bool isCenterFlame)
{
    int boardItem = items[y][x];

    // stop at rigid blocks
    if (boardItem == Item::RIGID)
        return false;

    if(boardItem >= Item::AGENT0)
    {
        Kill(boardItem - Item::AGENT0);
    }

    if(!isCenterFlame && (boardItem == Item::BOMB || boardItem >= Item::AGENT0))
    {
        // chain explosions (do not chain self)
        // note: bombs can also be "hidden" below agents
        for(int i = 0; i < bombs.count; i++)
        {
            Position bPos = BMB_POS(bombs[i]);
            if(bPos.x == x && bPos.y == y)
            {
                ExplodeBombAt(i);
                return true;
            }
        }
    }

    bool spawnFlame = false, continueSpawn = false;
    _cleanFlameSpawnPosition(flames, boardItem, x, y, timeStep, spawnFlame, continueSpawn);

    if(spawnFlame)
    {
        Flame& newFlame = flames.NextPos();
        newFlame.position.x = x;
        newFlame.position.y = y;

        // optimization: additive timeLeft in flame queue
        if(isCenterFlame)
        {
            if(flames.count == 0)
            {
                newFlame.timeLeft = FLAME_LIFETIME;
            }
            else
            {
                newFlame.timeLeft = FLAME_LIFETIME - currentFlameTime;
            }

            currentFlameTime = FLAME_LIFETIME;
        }
        else
        {
            newFlame.timeLeft = 0;
        }

        // update the board
        items[y][x] = Item::FLAME + (flames.count << 3);

        flames.count++;

        if(IS_WOOD(boardItem))
        {
            // set the powerup flag
            items[y][x] += WOOD_POWFLAG(boardItem);
            // remember that we destroyed wood here
            newFlame.destroyedWoodAtTimeStep = timeStep;
            // stop here, we found wood
            return false;
        }
        else
        {
            newFlame.destroyedWoodAtTimeStep = -1;
        }
    }

    return continueSpawn;
}

void Board::SpawnFlames(int x, int y, int strength)
{
    // spawn flame in center
    if(!SpawnFlameItem(x, y, true))
    {
        return;
    }

    // spawn subflames

    // right
    for(int i = 1; i <= strength; i++)
    {
        if(x + i >= BOARD_SIZE) break; // bounds

        if(!SpawnFlameItem(x + i, y, false))
        {
            break;
        }
    }

    // left
    for(int i = 1; i <= strength; i++)
    {
        if(x - i < 0) break; // bounds

        if(!SpawnFlameItem(x - i, y, false))
        {
            break;
        }
    }

    // top
    for(int i = 1; i <= strength; i++)
    {
        if(y + i >= BOARD_SIZE) break; // bounds

        if(!SpawnFlameItem(x, y + i, false))
        {
            break;
        }
    }

    // bottom
    for(int i = 1; i <= strength; i++)
    {
        if(y - i < 0) break; // bounds

        if(!SpawnFlameItem(x, y - i, false))
        {
            break;
        }
    }
}

void Board::PopFlames()
{
    while (flames.count > 0 && flames[0].timeLeft <= 0) {
        Flame& f = flames[0];
        // get the item behind the flame (can be 0 for passage)
        Item newItem = FlagItem(FLAME_POWFLAG(items[f.position.y][f.position.x]));
        // remove the flame
        items[f.position.y][f.position.x] = newItem;
        flames.PopElem();
    }
}

Item Board::FlagItem(int pwp)
{
    switch (pwp) {
        case 1: return Item::EXTRABOMB;
        case 2: return Item::INCRRANGE;
        case 3: return Item::KICK;
        default: return Item::PASSAGE;
    }
}

int Board::ItemFlag(Item item)
{
    switch (item) {
        case Item::EXTRABOMB: return 1;
        case Item::INCRRANGE: return 2;
        case Item::KICK: return 3;
        default: return 0;
    }
}

///////////////////
// State Methods //
///////////////////

int State::GetAgent(int x, int y)
{
    for(int i = 0; i < AGENT_COUNT; i++)
    {
        if(!agents[i].dead && agents[i].x == x && agents[i].y == y)
        {
            return i;
        }
    }
    return -1;
}

void State::PutAgent(int x, int y, int agentID)
{
    items[y][x] = Item::AGENT0 + agentID;

    agents[agentID].x = x;
    agents[agentID].y = y;
}

void State::PutAgentsInCorners(int a0, int a1, int a2, int a3, int padding)
{
    const int min = padding;
    const int max = BOARD_SIZE - (1 + padding);

    PutAgent(min, min, a0);
    PutAgent(max, min, a1);
    PutAgent(max, max, a2);
    PutAgent(min, max, a3);
}

inline int _invert(const int boardPos)
{
    return BOARD_SIZE - 1 - boardPos;
}

/**
 * Selects a random element arr[i] with i in range [0, count - 1]
 * and swaps arr[i] with arr[0]. This means arr[1:count-1] will
 * contain the elements which were not returned yet. Repeated use
 * with incremented arr start pointer allows for random and unique
 * in-place selection.
 *
 * @tparam preserveElements Can be disabled to save a copy operation,
 * arr[0] will not be set correctly. Only use this when you no longer
 * need the elements after they were selected.
 * @tparam T Type of the array.
 * @tparam Type of the random number generator.
 *
 * @param arr A pointer to the start of the array.
 * @param count The number of remaining elements.
 * @param rng A number generator. Has to return values >= 0.
 * @return A random element between arr[0] and arr[count - 1].
 */
template<bool preserveElements=true, typename T, typename RNG>
inline T _selectRandomInPlace(T* arr, int count, RNG& rng)
{
    // get a random index in [0, count - 1]
    const int index = rng() % count;

    // select value
    const T b = *(arr + index);

    // remember the value at arr[0] which we did not select
    *(arr + index) = *arr;
    if (preserveElements)
    {
        // do a regular swap if we want to preserve all elements in arr
        *arr = b;
    }

    // return the selected element
    return b;
}

void State::Init(GameMode gameMode, long boardSeed, long agentPositionSeed, int numRigid, int numWood, int numPowerUps, int padding, int breathingRoomSize)
{
    std::mt19937 rng;

    // initialize everything as passages
    std::fill_n(&items[0][0], BOARD_SIZE * BOARD_SIZE, (int)Item::PASSAGE);

    // insert agents at their respective positions
    std::array<int, 4> f = {0, 1, 2, 3};
    if(agentPositionSeed != -1)
    {
        rng.seed(agentPositionSeed);
        std::shuffle(f.begin(), f.end(), rng);
    }
    PutAgentsInCorners(f[0], f[1], f[2], f[3], padding);

    // init teams
    SetTeams(agents, gameMode);

    // create the board

    rng.seed(boardSeed);

    std::vector<Position> woodCoordinates;
    woodCoordinates.reserve(numWood);

    std::vector<Position> coordinates;
    coordinates.reserve(BOARD_SIZE * (BOARD_SIZE - 4) + 8 * padding + 4);

    // create a "breathing room" around agents of length freeSpaceUntil
    // and place wooden boxes to form passages between them.
    // The board will look like this (with padding to walls):
    //            |   padding    |
    // padding - [1][ ][x][x][ ][2] - padding
    //           [ ][?]      [?][ ] <- this is the breathing room
    //           [x]   [?][?]   [x]    (vertical and horizontal space)
    //           [x]   [?][?]   [x] <- these are the wooden boxes separating
    //           [ ][?]      [?][ ]    the players' breathing rooms
    // padding - [4][ ][x][x][ ][3] - padding
    //            |   padding    |

    int tmpNorm = -1;
    for(int i = 0; i < BOARD_SIZE; i++)
    {
        for(int  j = 0; j < BOARD_SIZE; j++)
        {
            if (i == padding || _invert(i) == padding) {
                tmpNorm = std::min(j, _invert(j));
            }
            else if (j == padding || _invert(j) == padding) {
                tmpNorm = std::min(i, _invert(i));
            }

            if(tmpNorm != -1) {
                // breathing room
                if (tmpNorm >= padding && tmpNorm <= breathingRoomSize) {
                    tmpNorm = -1;
                    continue;
                }
                // wooden boxes
                else if (tmpNorm > padding) {
                    tmpNorm = -1;
                    items[i][j] = Item::WOOD;
                    woodCoordinates.push_back((Position) {i, j});
                    numWood--;
                    continue;
                }
            }

            // remember this coordinate, we can randomly add stuff later
            coordinates.push_back((Position) {i, j});
        }
    }

    int i = 0;
    // create rigid walls
    while (numRigid > 0) {
        // select random coordinate
        Position coord = _selectRandomInPlace<false>(coordinates.data() + i, coordinates.size() - i, rng);
        i++;

        // create wall
        items[coord.y][coord.x] = Item::RIGID;
        numRigid--;
    }

    // create wooden blocks (keep index)
    while (numWood > 0) {
        Position coord = _selectRandomInPlace<false>(coordinates.data() + i, coordinates.size() - i, rng);
        i++;

        items[coord.y][coord.x] = Item::WOOD;
        woodCoordinates.push_back(coord);
        numWood--;
    }

    i = 0;
    std::uniform_int_distribution<int> choosePwp(1, 3);
    // insert items
    while (numPowerUps > 0) {
        Position coord = _selectRandomInPlace<false>(woodCoordinates.data() + i, woodCoordinates.size() - i, rng);
        i++;

        items[coord.y][coord.x] = Item::WOOD + choosePwp(rng);
        numPowerUps--;
    }

    // this is the initial state
    timeStep = 0;
    currentFlameTime = 0;
}

void State::Kill(int agentID)
{
    if(!agents[agentID].dead)
    {
        agents[agentID].dead = true;
        aliveAgents--;
    }
}

void State::Kill() {}

void State::EventBombExploded(Bomb b)
{
    agents[BMB_ID(b)].bombCount--;
}

//////////////////////
// bboard namespace //
//////////////////////

void bboard::Agent::reset()
{
    // default reset does nothing
}

void StartGame(State* state, Agent* agents[AGENT_COUNT], int timeSteps)
{
    Move moves[4];

    for(int i = 0; i < timeSteps; i++)
    {
        std::cout << "\033c"; // clear console on linux
        for(int j = 0; j < AGENT_COUNT; j++)
        {
            moves[j] = agents[j]->act(state);
        }

        Step(state, moves);
        PrintState(state);

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
}

void PrintState(const State* state, bool clearConsole)
{
    // clears console on linux
    if(clearConsole)
        std::cout << "\033c";

    for(int y = 0; y < BOARD_SIZE; y++)
    {
        for(int x = 0; x < BOARD_SIZE; x++)
        {
            int item = state->items[y][x];
            std::cout << PrintItem(item);
        }

        std::cout << "          ";

        // Print AgentInfo
        if(y < AGENT_COUNT)
        {
            std::cout << "Agent " << y << ": ";
            std::cout << PrintItem(Item::EXTRABOMB) << ": " << state->agents[y].maxBombCount << " ";
            std::cout << PrintItem(Item::INCRRANGE) << ": " << state->agents[y].bombStrength << " ";
            std::cout << PrintItem(Item::KICK) << ": " << state->agents[y].canKick << " ";
        }
        else if(y == AGENT_COUNT + 1)
        {
            std::cout << "Bombs:  [  ";
            for(int i = 0; i < state->bombs.count; i++)
            {
                std::cout << BMB_ID(state->bombs[i]) << "  ";
            }
            std::cout << "]";
        }
        else if(y == AGENT_COUNT + 2)
        {
            std::cout << "Flames: [  ";
            int cumulativeTime = 0;
            for(int i = 0; i < state->flames.count; i++)
            {
                if(state->flames[i].timeLeft != 0)
                {
                    cumulativeTime += state->flames[i].timeLeft;
                    std::cout << cumulativeTime << "  ";
                }
            }
            std::cout << "]";
        }
        std::cout << std::endl;
    }
}

void PrintBoard(const Board* board, bool clearConsole)
{
    // clears console on linux
    if(clearConsole)
        std::cout << "\033c";

    for(int y = 0; y < BOARD_SIZE; y++)
    {
        for(int x = 0; x < BOARD_SIZE; x++)
        {
            int item = board->items[y][x];
            std::cout << PrintItem(item);
        }
        std::cout << std::endl;
    }
}

template <typename T, int c>
void _printArray(const T arr[c])
{
    std::cout << "[";

    for(int i = 0; i < c - 1; i++)
    {
        std::cout << arr[i] << ", ";
    }

    if(c > 0)
    {
        std::cout << arr[c-1];
    }

    std::cout << "]";
}

void PrintObservation(const Observation* obs, bool clearConsole)
{
    PrintBoard(obs, clearConsole);

    std::cout << "IsAlive: ";
    _printArray<bool, AGENT_COUNT>(obs->isAlive);
    std::cout << std::endl;

    std::cout << "IsEnemy: ";
    _printArray<bool, AGENT_COUNT>(obs->isEnemy);
    std::cout << std::endl;
}

std::string PrintItem(int item)
{
    switch(item)
    {
        case Item::PASSAGE:
            return "   ";
        case Item::RIGID:
            return "[X]";
        case Item::BOMB:
            return " \u25CF ";
        case Item::EXTRABOMB:
            return " \u24B7 ";
        case Item::INCRRANGE:
            return " \u24C7 ";
        case Item::KICK:
            return " \u24C0 ";
        case Item::FOG:
            return "[@]";
    }

    if(IS_WOOD(item))
    {
        return FBLU((std::string)"[\u25A0]");
    }

    if(IS_FLAME(item))
    {
        return FRED((std::string)" \U0000263C ");
    }

    //agent number
    if(item >= Item::AGENT0)
    {
        return " "  +  std::to_string(item - Item::AGENT0) + " ";
    }
    else
    {
        // unknown item
        return "[?]";
    }
}

}

