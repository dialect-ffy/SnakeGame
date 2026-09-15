/* ============================================================================
 * SnakeGame UI 层 —— 输入映射
 * ----------------------------------------------------------------------------
 * 职责：把 raylib 的按键状态翻译成与游戏无关的“抽象动作”，
 *       使 main 主循环不必直接依赖具体键位，便于后续改键。
 * ==========================================================================*/

#ifndef SNAKE_UI_INPUT_H
#define SNAKE_UI_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 抽象输入动作。
 */
typedef enum {
    INPUT_ACTION_NONE = 0,   /**< 本帧无有效输入 */
    INPUT_ACTION_UP,         /**< 上（方向键 / W） */
    INPUT_ACTION_DOWN,       /**< 下（方向键 / S） */
    INPUT_ACTION_LEFT,       /**< 左（方向键 / A） */
    INPUT_ACTION_RIGHT,      /**< 右（方向键 / D） */
    INPUT_ACTION_PAUSE,      /**< 暂停/继续（P） */
    INPUT_ACTION_RESTART,    /**< 重新开始（R） */
    INPUT_ACTION_QUIT        /**< 退出（Esc） */
} InputAction;

/**
 * @brief 轮询本帧输入，返回优先级最高的一个动作。
 *
 * 优先级：退出 > 重开 > 暂停 > 方向。
 * 方向键与 WASD 等价；同一帧只返回一个方向（按 上/下/左/右 顺序判定）。
 *
 * @return 抽象动作；无输入时返回 INPUT_ACTION_NONE。
 */
InputAction input_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_UI_INPUT_H */
