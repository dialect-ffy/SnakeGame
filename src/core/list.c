/* ============================================================================
 * SnakeGame 核心逻辑层 —— 双向循环链表（带哨兵头结点）实现
 * ----------------------------------------------------------------------------
 * 本文件属于纯 C 逻辑层，禁止包含任何 raylib 头文件，以便被 googletest 独立测试。
 *
 * 结构不变量（invariant）：
 *   I1. sentinel 始终存在，且 sentinel->prev / sentinel->next 永不为 NULL；
 *   I2. 空表时 sentinel->prev == sentinel->next == sentinel（自环）；
 *   I3. 从 sentinel->next 正向遍历一圈（回到 sentinel）经过的结点数 == length；
 *   I4. 任意结点 n 满足 n->next->prev == n 且 n->prev->next == n。
 * 所有操作均需维持以上不变量。
 * ==========================================================================*/

#include "core/list.h"

#include <stdlib.h>   /* malloc / free */

/* ---------------------------------------------------------------------------
 * 内部辅助函数
 * -------------------------------------------------------------------------*/

/**
 * @brief 创建一个哨兵结点并使其自环（prev = next = 自身）。
 * @return 新哨兵结点；分配失败返回 NULL。
 */
static SnakeNode *make_sentinel(void)
{
    SnakeNode *s = (SnakeNode *)malloc(sizeof(SnakeNode));
    if (s == NULL) {
        return NULL;
    }
    s->x = 0;
    s->y = 0;
    s->prev = s;   /* 自环，满足不变量 I2 */
    s->next = s;
    return s;
}

/**
 * @brief 把结点 node 从链表中摘除（不释放内存）。
 *
 * 仅调整前后结点的指针关系，用于 pop_back / clear 内部复用。
 *
 * @param node 待摘除的结点，必须不是哨兵。
 */
static void unlink_node(SnakeNode *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

/* ---------------------------------------------------------------------------
 * 公开接口实现
 * -------------------------------------------------------------------------*/

SnakeList *snake_list_create(void)
{
    SnakeList *list = (SnakeList *)malloc(sizeof(SnakeList));
    if (list == NULL) {
        return NULL;
    }

    list->sentinel = make_sentinel();
    if (list->sentinel == NULL) {
        free(list);           /* 哨兵创建失败，回滚已分配的链表结构 */
        return NULL;
    }

    list->length = 0;
    return list;
}

void snake_list_clear(SnakeList *list)
{
    if (list == NULL) {
        return;
    }

    /* 从哨兵后第一个真实结点开始，逐个摘除并释放，直到回到哨兵 */
    SnakeNode *cur = list->sentinel->next;
    while (cur != list->sentinel) {
        SnakeNode *next = cur->next;   /* 先保存后继，避免释放后丢失 */
        free(cur);
        cur = next;
    }

    /* 恢复空表自环结构 */
    list->sentinel->next = list->sentinel;
    list->sentinel->prev = list->sentinel;
    list->length = 0;
}

void snake_list_destroy(SnakeList *list)
{
    if (list == NULL) {
        return;
    }

    snake_list_clear(list);   /* 先释放所有真实结点 */
    free(list->sentinel);     /* 再释放哨兵 */
    free(list);               /* 最后释放链表结构本身 */
}

int snake_list_length(const SnakeList *list)
{
    if (list == NULL) {
        return 0;
    }
    return list->length;
}

int snake_list_is_empty(const SnakeList *list)
{
    /* length == 0 即空表；list 为 NULL 时也视为空 */
    return (list == NULL) || (list->length == 0);
}

SnakeNode *snake_list_push_front(SnakeList *list, int x, int y)
{
    if (list == NULL) {
        return NULL;
    }

    SnakeNode *node = (SnakeNode *)malloc(sizeof(SnakeNode));
    if (node == NULL) {
        return NULL;
    }
    node->x = x;
    node->y = y;

    /* 将 node 插入到 sentinel 与原来的第一个结点之间：
     *     sentinel <-> node <-> old_first
     * 这样 node 成为新的蛇头。 */
    SnakeNode *sentinel = list->sentinel;
    SnakeNode *old_first = sentinel->next;

    node->prev = sentinel;
    node->next = old_first;
    sentinel->next = node;
    old_first->prev = node;

    list->length += 1;
    return node;
}

void snake_list_pop_back(SnakeList *list)
{
    if (list == NULL || list->length == 0) {
        return;   /* 空表空操作，保证不变量 I2 不被破坏 */
    }

    /* 蛇尾 = 哨兵的前驱 */
    SnakeNode *tail = list->sentinel->prev;
    unlink_node(tail);
    free(tail);
    list->length -= 1;
}

SnakeNode *snake_list_head(const SnakeList *list)
{
    if (list == NULL || list->length == 0) {
        return NULL;
    }
    return list->sentinel->next;
}

SnakeNode *snake_list_tail(const SnakeList *list)
{
    if (list == NULL || list->length == 0) {
        return NULL;
    }
    return list->sentinel->prev;
}

SnakeNode *snake_list_node_at(const SnakeList *list, int index)
{
    if (list == NULL || index < 0 || index >= list->length) {
        return NULL;
    }

    /* 从蛇头开始顺序前进 index 步 */
    SnakeNode *cur = list->sentinel->next;
    for (int i = 0; i < index; ++i) {
        cur = cur->next;
    }
    return cur;
}

int snake_list_contains(const SnakeList *list, int x, int y)
{
    if (list == NULL) {
        return 0;
    }

    /* 从蛇头遍历到蛇尾（遇到哨兵即停） */
    const SnakeNode *sentinel = list->sentinel;
    const SnakeNode *cur = sentinel->next;
    while (cur != sentinel) {
        if (cur->x == x && cur->y == y) {
            return 1;
        }
        cur = cur->next;
    }
    return 0;
}
