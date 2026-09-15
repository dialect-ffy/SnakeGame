/* ============================================================================
 * SnakeGame 核心逻辑层 —— 道具与效果数据结构
 * ----------------------------------------------------------------------------
 * 本文件只定义“数据结构与常量”，不包含逻辑实现（实现见 items.c），
 * 以便 game.h 在不产生循环依赖的前提下内嵌这些结构。
 *
 * 道具规则（来自规格）：
 *   - 同时最多 MAX_ITEMS 个；每 ITEM_SPAWN_MIN_SEC ~ ITEM_SPAWN_MAX_SEC 秒随机刷新 1 个；
 *   - 每个道具出现后存活 ITEM_LIFETIME_SEC 秒；
 *   - 效果持续 EFFECT_DURATION_SEC 秒；重复吃到同类效果“刷新时长”而非叠加；
 *   - 缩短道具立即减少 SHRINK_AMOUNT 节，但蛇长不低于 MIN_SNAKE_LENGTH；
 *   - 护盾在有效期内免疫一次死亡，触发后消耗。
 * ==========================================================================*/

#ifndef SNAKE_CORE_ITEMS_H
#define SNAKE_CORE_ITEMS_H

#include "core/types.h"

/* ------------------------------ 数量与计时 ------------------------------ */
#define MAX_ITEMS            2       /**< 地图上同时存在的道具上限 */
#define ITEM_LIFETIME_SEC    8.0f    /**< 单个道具存活时间（秒） */
#define ITEM_SPAWN_MIN_SEC   6.0f    /**< 道具刷新间隔下限（秒） */
#define ITEM_SPAWN_MAX_SEC   10.0f   /**< 道具刷新间隔上限（秒） */

/* ------------------------------ 效果参数 -------------------------------- */
#define EFFECT_DURATION_SEC  5.0f    /**< 加速/减速/护盾持续时间（秒） */
#define SHRINK_AMOUNT        3       /**< 缩短道具减少的节数 */
#define MIN_SNAKE_LENGTH     3       /**< 蛇身最小长度（缩短保护） */

/* 拾取判定：蛇头在视觉上是 2x2 的方块，只要该方块覆盖到道具所在格（即蛇头
 * 周围 3x3 邻域内的任意一格）就算拾取，避免“明明碰到了却吃不到”。 */
#define PICKUP_RADIUS        1       /**< 以蛇头为中心的拾取邻域半径（格） */

/* 效果对“每格步进毫秒”的倍率：加速让毫秒变小（更快），减速让毫秒变大（更慢） */
#define ITEM_SPEED_FACTOR    0.6f    /**< 加速倍率 */
#define ITEM_SLOW_FACTOR     1.6f    /**< 减速倍率 */
#define EFFECT_SPEED_MIN_MS  20      /**< 有效速度下限（毫秒，防止过快失控） */
#define EFFECT_SPEED_MAX_MS  150     /**< 有效速度上限（毫秒） */

/**
 * @brief 地图上的一个道具。
 *
 * type == ITEM_NONE 表示该槽位为空（未使用）。
 */
typedef struct {
    ItemType type;           /**< 道具类型；ITEM_NONE 表示空槽 */
    Cell     cell;           /**< 道具所在格子 */
    float    remaining_sec;  /**< 该道具在地图上剩余存在时间（秒） */
} Item;

/**
 * @brief 当前生效的道具效果剩余时间。
 *
 * 数值 > 0 表示该效果正在生效；<= 0 表示未生效。
 */
typedef struct {
    float speed_sec;    /**< 加速剩余时间（秒） */
    float slow_sec;     /**< 减速剩余时间（秒） */
    float shield_sec;   /**< 护盾剩余时间（秒） */
} ItemEffects;

#endif /* SNAKE_CORE_ITEMS_H */
