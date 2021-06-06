#include <limits>
#include <algorithm>
#include <unordered_set>

#include "bboard.hpp"
#include "colors.hpp"
#include "strategy.hpp"
#include "step_utility.hpp"

namespace bboard::strategy
{

////////////////////
// RMap Functions //
////////////////////

void RMap::SetDistance(int x, int y, int distance)
{
    map[y][x] = (map[y][x] & ~chalf) + distance;
}

void RMap::SetPredecessor(int x, int y, int xp, int yp)
{
    map[y][x] = (map[y][x] & chalf) + ((xp + BOARD_SIZE * yp) << 16);
}

int RMap::GetDistance(int x, int y) const
{
    return map[y][x] & chalf;
}

int RMap::GetPredecessor(int x, int y) const
{
    return map[y][x] >> 16;
}

template <typename T, int N>
inline RMapInfo TryAdd(const State& s, FixedQueue<T, N>& q, RMap& r, Position& c, int cx, int cy)
{
    int dist = r.GetDistance(c.x, c.y);
    int item = s.items[cy][cx];
    if(!util::IsOutOfBounds(cx, cy) &&
            r.GetDistance(cx, cy) == 0 &&
            (IS_WALKABLE(item) || item >= Item::AGENT0))
    {
        r.SetPredecessor(cx, cy, c.x, c.y);
        r.SetDistance(cx, cy, dist + 1);

        // we compute paths to agent positions but don't
        // continue search
        if(item < Item::AGENT0)
            q.AddElem({cx, cy});
        return 0;
    }
    return 0;
}
// BFS
void FillRMap(const State& s, RMap& r, int agentID)
{
    std::fill(r.map[0], r.map[0] + BOARD_SIZE * BOARD_SIZE, 0);
    int x = s.agents[agentID].x;
    int y = s.agents[agentID].y;
    r.source = {x, y};

    FixedQueue<Position, BOARD_SIZE * BOARD_SIZE> queue;
    r.SetDistance(x, y, 0);
    queue.AddElem({x, y});

    const AgentInfo& a = s.agents[agentID];
    RMapInfo result = 0;

    while(queue.count != 0)
    {

        Position& c = queue.PopElem();
        int dist = r.GetDistance(c.x, c.y);
        if(IsInBombRange(a.x, a.y, a.bombStrength, c) && dist < 10)
        {
            result |= 0b1;
        }

        if(c.x != x || c.y+1 != y)
            TryAdd(s, queue, r, c, c.x, c.y + 1);
        if(c.x != x || c.y-1 != y)
            TryAdd(s, queue, r, c, c.x, c.y - 1);
        if(c.x+1 != x || c.y != y)
            TryAdd(s, queue, r, c, c.x + 1, c.y);
        if(c.x-1 != x || c.y != y)
            TryAdd(s, queue, r, c, c.x - 1, c.y);

    }
    r.info = result;
}

///////////////////////
// General Functions //
///////////////////////

Move MoveTowardsPosition(const RMap& r, const Position& position)
{
    Position curr = position;
    for(int i = 0;; i++)
    {
        int idx = r.GetPredecessor(curr.x, curr.y);
        int y = idx / BOARD_SIZE;
        int x = idx % BOARD_SIZE;
        if(x == r.source.x && y == r.source.y)
        {
            if(curr.x > r.source.x) return Move::RIGHT;
            if(curr.x < r.source.x) return Move::LEFT;
            if(curr.y > r.source.y) return Move::DOWN;
            if(curr.y < r.source.y) return Move::UP;
        }
        else if(r.GetDistance(curr.x, curr.y) == 0)
        {
            return Move::IDLE;
        }
        curr = {x, y};
    }
}

Move MoveTowardsSafePlace(const State& state, const RMap& r, int radius)
{
    int originX = r.source.x;
    int originY = r.source.y;
    // TODO: Should be y < originY + radius (?)
    for(int y = originY - radius; y < originY + radius; y++)
    {
        for(int x = originX - radius; x < originY + radius; x++)
        {
            if(util::IsOutOfBounds({x, y}) ||
                    std::abs(x - originX) + std::abs(y - originY) > radius) continue;

            if(r.GetDistance(x, y) != 0 && _safe_condition(IsInDanger(state, x, y)))
            {
                return MoveTowardsPosition(r, {x, y});
            }
        }
    }
    return Move::IDLE;
}

Move MoveTowardsPowerup(const State& state, const RMap& r, int radius)
{
    const Position& a = r.source;
    for(int y = a.y - radius; y <= a.y + radius; y++)
    {
        for(int x = a.x - radius; x <= a.x + radius; x++)
        {
            if(util::IsOutOfBounds(x, y) ||
                    std::abs(x - a.x) + std::abs(y - a.y) > radius) continue;

            if(IS_POWERUP(state.items[y][x]))
            {
                return MoveTowardsPosition(r, {x, y});
            }
        }
    }

    return Move::IDLE;
}

Move MoveTowardsEnemy(const State& state, const RMap& r, int radius)
{
    const Position& a = r.source;

    for(int i = 0; i < AGENT_COUNT; i++)
    {
        // TODO: Skip own id (?)

        const AgentInfo& inf = state.agents[i];

        if((inf.x == a.x && inf.y == a.y) || inf.dead) continue;

        int x = state.agents[i].x;
        int y = state.agents[i].y;
        if(std::abs(x - a.x) + std::abs(y - a.y) > radius)
        {
            continue;
        }
        else
        {
            return MoveTowardsPosition(r, {x, y});
        }

    }
    return Move::IDLE;
}

bool _CheckPos(const State& state, int x, int y)
{
    return !util::IsOutOfBounds(x, y) && IS_WALKABLE(state.items[y][x]);
}

bool _safe_condition(int danger, int min)
{
    return danger == 0 || danger >= min;
}
void SafeDirections(const State& state, FixedQueue<Move, MOVE_COUNT>& q, int x, int y)
{
    int d = IsInDanger(state, x + 1, y);
    if(_CheckPos(state, x + 1, y) && _safe_condition(d))
    {
        q.AddElem(Move::RIGHT);
    }

    d = IsInDanger(state, x - 1, y);
    if(_CheckPos(state, x - 1, y) && _safe_condition(d))
    {
        q.AddElem(Move::LEFT);
    }

    d = IsInDanger(state, x, y + 1);
    if(_CheckPos(state, x, y + 1) && _safe_condition(d))
    {
        q.AddElem(Move::DOWN);
    }

    d = IsInDanger(state, x, y - 1);
    if(_CheckPos(state, x, y - 1) && _safe_condition(d))
    {
        q.AddElem(Move::UP);
    }
}

int IsInDanger(const State& state, int agentID)
{
    const AgentInfo& a = state.agents[agentID];
    return IsInDanger(state, a.x, a.y);
}

int IsInDanger(const State& state, int x, int y)
{
    int minTime = std::numeric_limits<int>::max();
    // TODO: add consideration for chained bomb explosions
    for(int i = 0; i < state.bombs.count; i++)
    {
        const Bomb& b = state.bombs[i];
        if(IsInBombRange(BMB_POS_X(b), BMB_POS_Y(b), BMB_STRENGTH(b), {x,y}))
        {
            if(BMB_TIME(b) < minTime)
            {
                minTime = BMB_TIME(b);
            }
        }
    }

    if(minTime == std::numeric_limits<int>::max())
        minTime = 0;

    return minTime;
}

void PrintMap(RMap &r)
{
    std::string res = "";
    for(int i = 0; i < BOARD_SIZE; i++)
    {
        for(int j = 0; j < BOARD_SIZE; j++)
        {
            res += (r.GetDistance(j, i) >= 10 ? "" : " ");
            res += std::to_string(r.GetDistance(j, i)) + " ";
        }
        res += "\n";
    }
    std::cout << res;
}


void PrintPath(RMap &r, Position from, Position to)
{
    std::array<Position, BOARD_SIZE * BOARD_SIZE> path;
    std::unordered_set<Position> pathx;

    path[0] = {to.x, to.y};
    Position curr = path[0];

    for(uint i = 0; !(curr == from); i++)
    {
        pathx.insert(curr);
        int idx = r.GetPredecessor(curr.x, curr.y);
        int y = idx / BOARD_SIZE;
        int x = idx % BOARD_SIZE;
        curr = path[i] = {x, y};
    }

    for(int i = 0; i < BOARD_SIZE; i++)
    {
        for(int j = 0; j < BOARD_SIZE; j++)
        {
            std::cout << (r.GetDistance(j, i) >= 10 ? "" : " ");
            auto dist = std::to_string(r.GetDistance(j, i));
            std::cout << (pathx.count({j, i}) ? FRED(dist) : dist) << " ";
        }
        std::cout << std::endl;
    }
}

// TODO ADD TEAM SUPPORT
bool IsAdjacentEnemy(const State& state, int agentID, int distance)
{
    const AgentInfo& a = state.agents[agentID];

    for(int i = 0; i < bboard::AGENT_COUNT; i++)
    {
        // ignore self and dead agents
        if(i == agentID || state.agents[i].dead) continue;
        // ignore team
        if(a.team != 0 && a.team == state.agents[i].team) continue;

        // manhattan dist
        if((std::abs(state.agents[i].x - a.x) +
                std::abs(state.agents[i].y - a.y)) <= distance)
        {
            return true;
        }
    }
    return false;
}

bool IsAdjacentItem(const State& state, int agentID, int distance, Item item)
{
    const AgentInfo& a = state.agents[agentID];
    const int originX = a.x;
    const int originY = a.y;
    for(int y = originY - distance; y <= originY + distance; y++)
    {
        for(int x = originX - distance; x <= originX + distance; x++)
        {
            if(util::IsOutOfBounds(x, y) ||
                    std::abs(x - originX) + std::abs(y - originY) > distance) continue;

            if(IS_WOOD(item) && IS_WOOD(state.items[y][x]))
            {
                return true;
            }
            if(state.items[y][x] == item)
            {
                return true;
            }
        }
    }
    return false;
}

}
