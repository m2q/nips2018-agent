#include <iostream>

#include "catch.hpp"
#include "bboard.hpp"

using namespace bboard;

template <typename T, int queueSize>
void REQUIRE_QUEUE_CONTENT(const FixedQueue<T, queueSize>& queue, std::vector<T> content)
{
    REQUIRE(queue.count == content.size());
    for(uint i = 0; i < content.size(); i++)
    {
        REQUIRE(queue[i] == content.at(i));
    }
}

void TestQueue(FixedQueue<Bomb, 10>& queue)
{
    for(int i = 0; i < 10; i++)
    {
        // insert elements 0 to 9
        int& current = queue.NextPos();
        current = i;
        queue.count++;
    }

    REQUIRE_QUEUE_CONTENT(queue, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});

    // pop elements 0 to 2
    queue.PopElem();
    queue.PopElem();
    queue.PopElem();
    REQUIRE_QUEUE_CONTENT(queue, {3, 4, 5, 6, 7, 8, 9});

    // remove 8: 3 4 5 6 7 [8] 9
    queue.RemoveAt(5);
    REQUIRE_QUEUE_CONTENT(queue, {3, 4, 5, 6, 7, 9});

    // remove 3: [3] 4 5 6 7 9
    queue.RemoveAt(0);
    REQUIRE_QUEUE_CONTENT(queue, {4, 5, 6, 7, 9});

    // remove 9: 4 5 6 7 [9]
    queue.RemoveAt(4);
    REQUIRE_QUEUE_CONTENT(queue, {4, 5, 6, 7});

    // add 42: 4 5 >42< 6 7
    queue.AddElem(42, 2);
    REQUIRE_QUEUE_CONTENT(queue, {4, 5, 42, 6, 7});

    // add 3: >3< 4 5 42 6 7
    queue.AddElem(3, 0);
    REQUIRE_QUEUE_CONTENT(queue, {3, 4, 5, 42, 6, 7});

    // add 8: 3 4 5 42 6 7 >8<
    queue.AddElem(8, 6);
    REQUIRE_QUEUE_CONTENT(queue, {3, 4, 5, 42, 6, 7, 8});
}

TEST_CASE("Fixed Size Queue", "[general]")
{
    FixedQueue<Bomb, 10> queue;
    SECTION("Index 0")
    {
        queue.index = 0;
        TestQueue(queue);
    }
    SECTION("Index 5")
    {
        queue.index = 5;
        TestQueue(queue);
    }
    SECTION("Index 2")
    {
        queue.index = 2;
        TestQueue(queue);
    }
}
