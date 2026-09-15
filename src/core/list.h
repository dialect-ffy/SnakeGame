/* ============================================================================
 * SnakeGame 核心逻辑层 —— 双向循环链表（带哨兵头结点）
 * ----------------------------------------------------------------------------
 * 设计说明：
 *   - 链表用于表示贪吃蛇的身体，每个“真实结点”保存一个逻辑格子坐标 (x, y)。
 *   - 采用“哨兵头结点（sentinel）”：哨兵本身不保存有效数据，仅作为固定的
 *     锚点存在。空表时 sentinel->prev == sentinel->next == sentinel。
 *   - 链表是“双向 + 循环”的：
 *         哨兵 <-> 蛇头 <-> ... <-> 蛇尾 <-> 哨兵
 *     因此从任意结点出发都能遍历整圈，且头尾操作无需判空边界。
 *   - 贪吃蛇每走一步对应：
 *         snake_list_push_front()  在哨兵后插入新蛇头
 *         snake_list_pop_back()    删除蛇尾（未吃到食物时）
 *     若吃到食物则只 push_front 不 pop_back，长度自然 +1。
 *
 * 坐标约定：
 *   x 为列（0 .. GRID_COLS-1），y 为行（0 .. GRID_ROWS-1），原点在左上角。
 * ==========================================================================*/

#ifndef SNAKE_CORE_LIST_H
#define SNAKE_CORE_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 链表结点。
 *
 * 真实结点保存一个格子坐标；哨兵结点的 x/y 无意义，不应被读取。
 */
typedef struct SnakeNode {
    int x;                    /**< 列坐标（哨兵结点该值无意义） */
    int y;                    /**< 行坐标（哨兵结点该值无意义） */
    struct SnakeNode *prev;   /**< 前驱结点（循环，空表时指向哨兵自身） */
    struct SnakeNode *next;   /**< 后继结点（循环，空表时指向哨兵自身） */
} SnakeNode;

/**
 * @brief 双向循环链表（带哨兵）。
 */
typedef struct {
    SnakeNode *sentinel;      /**< 哨兵结点，永远存在，不保存有效数据 */
    int        length;        /**< 真实结点数量（不含哨兵） */
} SnakeList;

/**
 * @brief 创建一个空链表（仅含哨兵，哨兵自环）。
 * @return 新链表指针；内存分配失败时返回 NULL。
 */
SnakeList *snake_list_create(void);

/**
 * @brief 销毁链表，释放所有真实结点与哨兵及链表本身。
 * @param list 目标链表，允许为 NULL（此时不做任何事）。
 */
void snake_list_destroy(SnakeList *list);

/**
 * @brief 删除所有真实结点，保留哨兵，使链表恢复为空表。
 * @param list 目标链表，允许为 NULL。
 */
void snake_list_clear(SnakeList *list);

/**
 * @brief 获取真实结点数量（不含哨兵）。
 * @param list 目标链表，允许为 NULL（返回 0）。
 * @return 结点数量。
 */
int snake_list_length(const SnakeList *list);

/**
 * @brief 判断链表是否为空（仅含哨兵）。
 * @param list 目标链表，允许为 NULL（视为空）。
 * @return 非 0 表示空，0 表示非空。
 */
int snake_list_is_empty(const SnakeList *list);

/**
 * @brief 在哨兵之后（即作为新的“蛇头”）插入一个真实结点。
 * @param list 目标链表，不可为 NULL。
 * @param x    新结点列坐标。
 * @param y    新结点行坐标。
 * @return 新插入的结点指针；list 为 NULL 或分配失败时返回 NULL。
 */
SnakeNode *snake_list_push_front(SnakeList *list, int x, int y);

/**
 * @brief 删除“蛇尾”结点（哨兵的前驱，即最后一个真实结点）。
 *        空表时为空操作，不会破坏哨兵自环结构。
 * @param list 目标链表，允许为 NULL（空操作）。
 */
void snake_list_pop_back(SnakeList *list);

/**
 * @brief 获取蛇头结点（哨兵的后继，第一个真实结点）。
 * @param list 目标链表。
 * @return 蛇头结点指针；链表为空或 list 为 NULL 时返回 NULL。
 */
SnakeNode *snake_list_head(const SnakeList *list);

/**
 * @brief 获取蛇尾结点（哨兵的前驱，最后一个真实结点）。
 * @param list 目标链表。
 * @return 蛇尾结点指针；链表为空或 list 为 NULL 时返回 NULL。
 */
SnakeNode *snake_list_tail(const SnakeList *list);

/**
 * @brief 按从头到尾的顺序获取第 index 个真实结点（0 基）。
 * @param list  目标链表。
 * @param index 下标，0 表示蛇头。
 * @return 对应结点指针；越界或 list 为 NULL 时返回 NULL。
 */
SnakeNode *snake_list_node_at(const SnakeList *list, int index);

/**
 * @brief 判断链表中是否存在坐标为 (x, y) 的真实结点。
 *
 * 用于贪吃蛇的自撞检测（注意：调用方需自行决定是否排除蛇头）。
 *
 * @param list 目标链表。
 * @param x    待查询列坐标。
 * @param y    待查询行坐标。
 * @return 非 0 表示存在，0 表示不存在。
 */
int snake_list_contains(const SnakeList *list, int x, int y);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_CORE_LIST_H */
