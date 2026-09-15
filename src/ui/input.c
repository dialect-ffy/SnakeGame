/* ============================================================================
 * SnakeGame UI 层 —— 输入映射实现（raylib）
 * ==========================================================================*/

#include "ui/input.h"

#include <raylib.h>

/* 判断某方向键是否在本帧被按下（方向键或 WASD 任一即可） */
static int pressed(int key1, int key2)
{
    return IsKeyPressed(key1) || IsKeyPressed(key2);
}

InputAction input_poll(void)
{
    /* 高优先级动作优先返回，避免同帧多键冲突 */
    if (IsKeyPressed(KEY_ESCAPE)) {
        return INPUT_ACTION_QUIT;
    }
    if (IsKeyPressed(KEY_R)) {
        return INPUT_ACTION_RESTART;
    }
    if (IsKeyPressed(KEY_P)) {
        return INPUT_ACTION_PAUSE;
    }

    if (pressed(KEY_UP, KEY_W)) {
        return INPUT_ACTION_UP;
    }
    if (pressed(KEY_DOWN, KEY_S)) {
        return INPUT_ACTION_DOWN;
    }
    if (pressed(KEY_LEFT, KEY_A)) {
        return INPUT_ACTION_LEFT;
    }
    if (pressed(KEY_RIGHT, KEY_D)) {
        return INPUT_ACTION_RIGHT;
    }

    return INPUT_ACTION_NONE;
}
