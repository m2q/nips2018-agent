#ifndef STRATEGY_H
#define STRATEGY_H

#include "bboard.hpp"
#include "step_utility.hpp"

// integer with less than 4 bytes will have bugs
static_assert (sizeof(int) == 4 || sizeof (int) == 8, "32/64 bit integer");

namespace bboard::strategy
{

/////////////////////////
// Reachable Map & BFS //
/////////////////////////

/**
 * @brief RMapInfo Filling an RMap lets you collect additional
 * information. These are encoded into RMapInfo
 */
typedef uint RMapInfo;
const int chalf = 0xFFFF;

/**
 * @brief The ReachableMap struct is a bitflag array
 * that describes which positions on the board can be
 * reached.
 */
struct RMap
{

    int map[BOARD_SIZE][BOARD_SIZE] = {};
    RMapInfo info;
    Position source;
    /**
     * @brief GetDistance Returns the shortest walking
     * distance from the point the RMap was initialized
     * to the given point (x, y).
     */
    int  GetDistance(int x, int y) const;
    void SetDistance(int x, int y, int distance);
    /**
     * @brief GetPredecessor Returns the index i = x' + 11 * y' of the predecessor
     * of the position (x, y)
     */
    int  GetPredecessor(int x, int y) const;
    void SetPredecessor(int x, int y, int xPredecessor, int yPredecessor);
};

/**
 * @brief FillRMap Fills a given RMap. Uses BFS. Additional
 * info is in map.info
 */
void FillRMap(const State& s, RMap& r, int agentID);

/**
 * @brief IsReachable Returns true if the given position is reachable
 * on the given RMap
 */
inline bool IsReachable(RMap& r, int x, int y)
{
    return r.GetDistance(x, y) != 0;
}

//////////////
// Movement //
//////////////

/**
 * @brief IsAdjacentEnemy returns true if the agent is within a
 * given manhattan-distance from an enemey
 */
bool IsAdjacentEnemy(const State& state, int agentID, int distance);

/**
 * @brief IsAdjacentEnemy returns true if the agent is within a
 * given manhattan-distance from the specified item
 */
bool IsAdjacentItem(const State& state, int agentID, int distance, Item item);

/**
 * @brief MoveTowardsPosition Selects a move the brings you closest
 * from the RMap source to the specified target
 */
Move MoveTowardsPosition(const RMap& r, const Position& position);

/**
 * @brief MoveTowardsSafePlace Returns the direction towards one
 * of the closest points from the source that is safe from explosions.
 * (In the given radius). Note: Doesn't guarantee that the agent doesn't
 * die along the way by other dangers. Only considers the imminent danger
 * in his current position.
 *
 * @param r The filled RMap
 * @param radius The search radius
 * @return IDLE if no safe place could be found
 */
Move MoveTowardsSafePlace(const State& state, const RMap& r, int radius);

/**
 * @brief MoveTowardsPowerup Returns the move that brings the agent
 * closer to a powerup in a specified radius. If no nearby powerup is
 * in that radius, then don't the move is IDLE
 * @param r A filled map with all information about distances and
 * paths. See bboard::strategy::RMap for more info
 * @param radius Maximum search distance (manhattan)
 */
Move MoveTowardsPowerup(const State& state, const RMap& r, int radius);

/**
 * @brief MoveTowardsEnemy Returns the move that brings the agent
 * closer to an enemy in a specified radius. If no nearby enemy is
 * in that radius, then default to IDLE
 * @param r A filled map with all information about distances and
 * paths. See bboard::strategy::RMap for more info
 * @param radius Maximum search distance (manhattan)
 */
Move MoveTowardsEnemy(const State& state, const RMap& r, int radius);

/**
 * @brief FilterSafeDirections Adds all possible safe moves to the
 * queue
 */
void SafeDirections(const State& state, FixedQueue<Move, MOVE_COUNT>& q, int x, int y);

/**
 * @brief SortDirections Sort a move-queue, where unvisited states are
 * last in the queue
 */
template <int X>
void SortDirections(FixedQueue<Move, MOVE_COUNT>& q,
                    FixedQueue<Position, X>& p, int x, int y)
{
    int moves = q.count;

    for(int i = 0, totalSteps = 0; totalSteps < moves; i++, totalSteps++)
    {
        Position pos = util::DesiredPosition(x, y, q[i]);
        for(int j = 0; j < p.count; j++)
        {
            if(pos == p[j])
            {
                q.RemoveAt(i);
                q.AddElem(q[i]);
                i--;
                break;
            }
        }
    }
}



/**
 * @brief IsSafe Returns true if the agent is endangered (in range of a bomb).
 * The int-value says how much time the agent has to flee.
 */
int IsInDanger(const State& state, int agentID);
int IsInDanger(const State& state, int x, int y);

/**
 * @brief IsInBombRange Returns True if the given position is in range
 * of a bomb planted at (x, y) with strength s
 */
inline bool IsInBombRange(int x, int y, int s, const Position& pos)
{
    return (pos.y == y && (x-s <= pos.x && pos.x <= x+s))
           ||
           (pos.x == x && (y-s <= pos.y && pos.y <= y+s));
}


/**
 * @brief PrintMap Pretty-prints the reachable map
 */
void PrintMap(RMap& r);

/**
 * @brief PrintMap Pretty-prints the path from-to
 */
void PrintPath(RMap& r, Position from, Position to);


bool _safe_condition(int danger, int min = 2);

}


#endif // STRATEGY_H
