/* ============================================================================
 * SnakeGame 核心逻辑层 —— 公共基础类型
 * ----------------------------------------------------------------------------
 * 本文件只定义“纯数据”类型（不含任何逻辑与 raylib 依赖），
 * 供 core（逻辑）与 ui（渲染）共同使用，避免两边重复定义或类型不一致。
 * ==========================================================================*/

#ifndef SNAKE_CORE_TYPES_H
#define SNAKE_CORE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 逻辑格子坐标。
 *
 * x 为列（0 .. GRID_COLS-1），y 为行（0 .. GRID_ROWS-1），原点在左上角。
 */
typedef struct {
    int x;
    int y;
} Cell;

/**
 * @brief 蛇的移动方向。
 */
typedef enum {
    DIR_UP = 0,    /**< 上 */
    DIR_DOWN,      /**< 下 */
    DIR_LEFT,      /**< 左 */
    DIR_RIGHT      /**< 右 */
} Direction;

/**
 * @brief 道具类型。
 *
 * ITEM_NONE 用于表示“该槽位为空”，便于道具数组统一管理。
 */
typedef enum {
    ITEM_NONE = 0,   /**< 无道具（空槽位） */
    ITEM_SPEED,      /**< 加速 */
    ITEM_SLOW,       /**< 减速 */
    ITEM_SHRINK,     /**< 缩短身体 */
    ITEM_SHIELD      /**< 护盾（免死一次） */
} ItemType;

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_CORE_TYPES_H */
