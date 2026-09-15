// ============================================================================
// P1 单元测试：双向循环链表（带哨兵头结点）
// ----------------------------------------------------------------------------
// 覆盖点：
//   1. 创建/销毁/清空的基本语义；
//   2. 头插（push_front）与尾删（pop_back）的顺序与长度；
//   3. 双向循环结构完整性（正向/反向遍历、指针对称性、哨兵自环）；
//   4. 查询（contains / node_at / head / tail）的边界；
//   5. 大量结点的稳定性与复用；
//   6. NULL 入参的安全性。
// ============================================================================

#include <gtest/gtest.h>

#include <utility>
#include <vector>

extern "C" {
#include "core/list.h"
}

namespace {

// 按“蛇头 -> 蛇尾”顺序收集所有真实结点坐标
std::vector<std::pair<int, int>> CellsFromHead(const SnakeList *list)
{
    std::vector<std::pair<int, int>> result;
    const SnakeNode *sentinel = list->sentinel;
    const SnakeNode *cur = sentinel->next;
    while (cur != sentinel) {
        result.emplace_back(cur->x, cur->y);
        cur = cur->next;
    }
    return result;
}

// 按“蛇尾 -> 蛇头”顺序收集所有真实结点坐标
std::vector<std::pair<int, int>> CellsFromTail(const SnakeList *list)
{
    std::vector<std::pair<int, int>> result;
    const SnakeNode *sentinel = list->sentinel;
    const SnakeNode *cur = sentinel->prev;
    while (cur != sentinel) {
        result.emplace_back(cur->x, cur->y);
        cur = cur->prev;
    }
    return result;
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. 创建与空表语义
// ---------------------------------------------------------------------------

// 新建链表应非空指针、长度为 0、为空、头尾均为 NULL，且哨兵自环
TEST(SnakeListTest, CreateYieldsEmptySelfLoopedList)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    EXPECT_EQ(snake_list_length(list), 0);
    EXPECT_NE(snake_list_is_empty(list), 0);
    EXPECT_EQ(snake_list_head(list), nullptr);
    EXPECT_EQ(snake_list_tail(list), nullptr);

    // 哨兵必须自环
    EXPECT_EQ(list->sentinel->next, list->sentinel);
    EXPECT_EQ(list->sentinel->prev, list->sentinel);

    snake_list_destroy(list);
}

// ---------------------------------------------------------------------------
// 2. 头插与顺序
// ---------------------------------------------------------------------------

// 头插单个结点后，头与尾是同一个结点
TEST(SnakeListTest, PushFrontSingleNodeBecomesHeadAndTail)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    SnakeNode *n = snake_list_push_front(list, 3, 4);
    ASSERT_NE(n, nullptr);

    EXPECT_EQ(snake_list_length(list), 1);
    EXPECT_EQ(snake_list_is_empty(list), 0);
    EXPECT_EQ(snake_list_head(list), n);
    EXPECT_EQ(snake_list_tail(list), n);
    EXPECT_EQ(n->x, 3);
    EXPECT_EQ(n->y, 4);

    snake_list_destroy(list);
}

// 依次头插 (1,1)(2,2)(3,3)，从头到尾应为 (3,3)(2,2)(1,1)
TEST(SnakeListTest, PushFrontKeepsNewestAtHead)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    snake_list_push_front(list, 1, 1);
    snake_list_push_front(list, 2, 2);
    snake_list_push_front(list, 3, 3);

    EXPECT_EQ(snake_list_length(list), 3);

    std::vector<std::pair<int, int>> expected = {{3, 3}, {2, 2}, {1, 1}};
    EXPECT_EQ(CellsFromHead(list), expected);

    snake_list_destroy(list);
}

// ---------------------------------------------------------------------------
// 3. 循环结构完整性
// ---------------------------------------------------------------------------

// 正向遍历应恰好经过 length 个结点并回到哨兵
TEST(SnakeListTest, ForwardTraversalVisitsExactlyLengthNodes)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    for (int i = 0; i < 10; ++i) {
        snake_list_push_front(list, i, -i);
    }

    int count = 0;
    const SnakeNode *sentinel = list->sentinel;
    const SnakeNode *cur = sentinel->next;
    while (cur != sentinel) {
        ++count;
        cur = cur->next;
    }
    EXPECT_EQ(count, snake_list_length(list));
    EXPECT_EQ(count, 10);

    snake_list_destroy(list);
}

// 反向遍历也应恰好经过 length 个结点并回到哨兵
TEST(SnakeListTest, BackwardTraversalVisitsExactlyLengthNodes)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    for (int i = 0; i < 7; ++i) {
        snake_list_push_front(list, i, i);
    }

    int count = 0;
    const SnakeNode *sentinel = list->sentinel;
    const SnakeNode *cur = sentinel->prev;
    while (cur != sentinel) {
        ++count;
        cur = cur->prev;
    }
    EXPECT_EQ(count, 7);

    snake_list_destroy(list);
}

// 任意结点满足 n->next->prev == n 且 n->prev->next == n（双向对称）
TEST(SnakeListTest, LinksAreSymmetricForAllNodes)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    for (int i = 0; i < 5; ++i) {
        snake_list_push_front(list, i, i);
    }

    const SnakeNode *sentinel = list->sentinel;
    const SnakeNode *cur = sentinel->next;
    while (cur != sentinel) {
        EXPECT_EQ(cur->next->prev, cur);
        EXPECT_EQ(cur->prev->next, cur);
        cur = cur->next;
    }
    // 哨兵自身也应满足对称性
    EXPECT_EQ(sentinel->next->prev, sentinel);
    EXPECT_EQ(sentinel->prev->next, sentinel);

    snake_list_destroy(list);
}

// ---------------------------------------------------------------------------
// 4. 尾删
// ---------------------------------------------------------------------------

// 尾删应移除最老的结点（蛇尾），保留蛇头
TEST(SnakeListTest, PopBackRemovesOldestAndKeepsHead)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    snake_list_push_front(list, 1, 1);  // 将成为尾
    snake_list_push_front(list, 2, 2);
    snake_list_push_front(list, 3, 3);  // 头

    SnakeNode *head_before = snake_list_head(list);

    snake_list_pop_back(list);

    EXPECT_EQ(snake_list_length(list), 2);
    EXPECT_EQ(snake_list_head(list), head_before);  // 头不变
    std::vector<std::pair<int, int>> expected = {{3, 3}, {2, 2}};
    EXPECT_EQ(CellsFromHead(list), expected);

    snake_list_destroy(list);
}

// 对空表尾删应为空操作，且不破坏哨兵自环
TEST(SnakeListTest, PopBackOnEmptyIsNoop)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    snake_list_pop_back(list);

    EXPECT_EQ(snake_list_length(list), 0);
    EXPECT_EQ(list->sentinel->next, list->sentinel);
    EXPECT_EQ(list->sentinel->prev, list->sentinel);

    snake_list_destroy(list);
}

// 连续尾删直到空表，头尾应回到 NULL，哨兵恢复自环
TEST(SnakeListTest, PopBackUntilEmptyResetsToEmptyState)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    for (int i = 0; i < 4; ++i) {
        snake_list_push_front(list, i, 0);
    }

    for (int i = 0; i < 4; ++i) {
        snake_list_pop_back(list);
    }

    EXPECT_EQ(snake_list_length(list), 0);
    EXPECT_NE(snake_list_is_empty(list), 0);
    EXPECT_EQ(snake_list_head(list), nullptr);
    EXPECT_EQ(snake_list_tail(list), nullptr);
    EXPECT_EQ(list->sentinel->next, list->sentinel);
    EXPECT_EQ(list->sentinel->prev, list->sentinel);

    snake_list_destroy(list);
}

// 尾删后再头插，结构应保持正确（复用场景）
TEST(SnakeListTest, PushAfterPopKeepsStructureConsistent)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    snake_list_push_front(list, 1, 1);  // A
    snake_list_push_front(list, 2, 2);  // B
    snake_list_push_front(list, 3, 3);  // C

    snake_list_pop_back(list);          // 移除 A -> (C, B)
    snake_list_push_front(list, 4, 4);  // 头插 D -> (D, C, B)

    std::vector<std::pair<int, int>> expected = {{4, 4}, {3, 3}, {2, 2}};
    EXPECT_EQ(CellsFromHead(list), expected);
    EXPECT_EQ(snake_list_length(list), 3);

    snake_list_destroy(list);
}

// ---------------------------------------------------------------------------
// 5. 查询
// ---------------------------------------------------------------------------

// contains 命中与未命中
TEST(SnakeListTest, ContainsFindsExistingCellOnly)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    snake_list_push_front(list, 5, 6);
    snake_list_push_front(list, 7, 8);

    EXPECT_NE(snake_list_contains(list, 5, 6), 0);
    EXPECT_NE(snake_list_contains(list, 7, 8), 0);
    EXPECT_EQ(snake_list_contains(list, 0, 0), 0);
    EXPECT_EQ(snake_list_contains(list, 8, 7), 0);  // 顺序不同视为不同格子

    snake_list_destroy(list);
}

// 空表 contains 恒为 0
TEST(SnakeListTest, ContainsOnEmptyListIsFalse)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    EXPECT_EQ(snake_list_contains(list, 1, 1), 0);

    snake_list_destroy(list);
}

// node_at 下标合法性与越界返回 NULL
TEST(SnakeListTest, NodeAtIndexHandlesBounds)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    snake_list_push_front(list, 10, 10);  // index 2
    snake_list_push_front(list, 20, 20);  // index 1
    snake_list_push_front(list, 30, 30);  // index 0 (头)

    ASSERT_NE(snake_list_node_at(list, 0), nullptr);
    EXPECT_EQ(snake_list_node_at(list, 0)->x, 30);
    EXPECT_EQ(snake_list_node_at(list, 1)->x, 20);
    EXPECT_EQ(snake_list_node_at(list, 2)->x, 10);

    EXPECT_EQ(snake_list_node_at(list, 3), nullptr);   // 越界（上）
    EXPECT_EQ(snake_list_node_at(list, -1), nullptr);  // 越界（下）

    snake_list_destroy(list);
}

// head/tail 取值应正确
TEST(SnakeListTest, HeadAndTailAccessors)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    snake_list_push_front(list, 1, 1);  // 尾
    snake_list_push_front(list, 2, 2);  // 头

    EXPECT_EQ(snake_list_head(list)->x, 2);
    EXPECT_EQ(snake_list_tail(list)->x, 1);

    snake_list_destroy(list);
}

// ---------------------------------------------------------------------------
// 6. 清空与规模稳定性
// ---------------------------------------------------------------------------

// clear 后链表仍可用（哨兵保留）
TEST(SnakeListTest, ClearRemovesAllAndKeepsListUsable)
{
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    for (int i = 0; i < 6; ++i) {
        snake_list_push_front(list, i, i);
    }

    snake_list_clear(list);

    EXPECT_EQ(snake_list_length(list), 0);
    EXPECT_EQ(snake_list_head(list), nullptr);
    EXPECT_EQ(snake_list_tail(list), nullptr);

    // 清空后仍可继续插入
    snake_list_push_front(list, 9, 9);
    EXPECT_EQ(snake_list_length(list), 1);
    EXPECT_EQ(snake_list_head(list)->x, 9);

    snake_list_destroy(list);
}

// 大量结点下正向/反向遍历数量一致，且 contains 正确
TEST(SnakeListTest, LargeListStaysConsistent)
{
    const int N = 1000;
    SnakeList *list = snake_list_create();
    ASSERT_NE(list, nullptr);

    for (int i = 0; i < N; ++i) {
        snake_list_push_front(list, i, i % 32);
    }

    EXPECT_EQ(snake_list_length(list), N);

    int forward = 0;
    const SnakeNode *sentinel = list->sentinel;
    for (const SnakeNode *cur = sentinel->next; cur != sentinel; cur = cur->next) {
        ++forward;
    }
    EXPECT_EQ(forward, N);

    int backward = 0;
    for (const SnakeNode *cur = sentinel->prev; cur != sentinel; cur = cur->prev) {
        ++backward;
    }
    EXPECT_EQ(backward, N);

    EXPECT_NE(snake_list_contains(list, N - 1, (N - 1) % 32), 0);
    EXPECT_EQ(snake_list_contains(list, N, N), 0);

    snake_list_destroy(list);
}

// ---------------------------------------------------------------------------
// 7. NULL 入参安全
// ---------------------------------------------------------------------------

// 所有接口在 NULL 入参下应安全返回默认值或不崩溃
TEST(SnakeListTest, NullArgumentsAreSafe)
{
    EXPECT_EQ(snake_list_length(nullptr), 0);
    EXPECT_NE(snake_list_is_empty(nullptr), 0);
    EXPECT_EQ(snake_list_head(nullptr), nullptr);
    EXPECT_EQ(snake_list_tail(nullptr), nullptr);
    EXPECT_EQ(snake_list_node_at(nullptr, 0), nullptr);
    EXPECT_EQ(snake_list_contains(nullptr, 0, 0), 0);
    EXPECT_EQ(snake_list_push_front(nullptr, 0, 0), nullptr);

    snake_list_pop_back(nullptr);  // 不应崩溃
    snake_list_clear(nullptr);     // 不应崩溃
    snake_list_destroy(nullptr);   // 不应崩溃
}
