/* ============================================================================
 * SnakeGame UI 层 —— 渲染接口
 * ----------------------------------------------------------------------------
 * 本模块只负责“把逻辑数据画到屏幕上”，不修改任何游戏状态：
 *   - 输入参数均为只读的坐标/数值；
 *   - 不包含游戏规则判断。
 * 这样 core 逻辑可以完全脱离 raylib 被单元测试。
 * ==========================================================================*/

#ifndef SNAKE_UI_RENDER_H
#define SNAKE_UI_RENDER_H

#include <raylib.h>

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HUD（顶部信息栏）所需的显示数据。
 *
 * 各 effect_*_sec 表示对应道具效果的剩余秒数，<= 0 表示未生效（不显示）。
 */
typedef struct {
    int   score;              /**< 当前分数 */
    int   high_score;         /**< 历史最高分 */
    int   level;              /**< 当前等级 */
    int   speed_ms;           /**< 当前每格步进毫秒数 */
    float speed_effect_sec;   /**< 加速效果剩余秒数 */
    float slow_effect_sec;    /**< 减速效果剩余秒数 */
    float shield_effect_sec;  /**< 护盾效果剩余秒数 */
} HudInfo;

/**
 * @brief 绘制窗口背景色（覆盖整屏）。
 */
void render_draw_background(void);

/**
 * @brief 绘制游戏区底板、淡网格线与霓虹边框。
 */
void render_draw_playfield(void);

/**
 * @brief 绘制整条蛇（带插值动画）。
 *
 * 逻辑步进是离散的（每 speed_ms 走一格），本函数在相邻两次逻辑位置之间做线性插值，
 * 使视觉上平滑移动。
 *
 * @param from_cells 上一步的蛇身格子数组（索引 0 为蛇头），可为 NULL（表示不插值）。
 * @param to_cells   当前蛇身格子数组（索引 0 为蛇头）。
 * @param count      当前蛇身结点数量。
 * @param head_dir   蛇头朝向，用于绘制眼睛。
 * @param t          插值系数，0 表示完全在 from，1 表示完全在 to。
 * @param base_color 蛇身主色（蛇头颜色，尾部会向深色渐变）。用于“吃到道具变色”。
 */
void render_draw_snake(const Cell *from_cells, const Cell *to_cells,
                       int count, Direction head_dir, float t, Color base_color);

/**
 * @brief 绘制暂停覆盖层（半透明遮罩 + PAUSED 文案）。
 */
void render_draw_pause_overlay(void);

/**
 * @brief 绘制游戏结束覆盖层（半透明遮罩 + 分数 + 可点击的“重新开始”按钮）。
 * @param score      本局分数。
 * @param high_score 历史最高分。
 */
void render_draw_gameover_overlay(int score, int high_score);

/**
 * @brief 获取“重新开始”按钮的矩形区域（虚拟分辨率坐标系）。
 *
 * 供 main 做鼠标点击命中检测使用；与 render_draw_gameover_overlay 中绘制的按钮一致。
 *
 * @return 按钮矩形。
 */
Rectangle render_restart_button_rect(void);

/**
 * @brief 获取“退出游戏”按钮的矩形区域（虚拟分辨率坐标系）。
 * @return 按钮矩形。
 */
Rectangle render_quit_button_rect(void);

/**
 * @brief 绘制窗口底部操作提示条。
 */
void render_draw_hint_bar(void);

/**
 * @brief 绘制普通食物（发光青色圆点）。
 * @param cell 食物所在格子。
 */
void render_draw_food(Cell cell);

/**
 * @brief 绘制障碍物（霓虹红方块）。
 * @param cell 障碍物所在格子。
 */
void render_draw_obstacle(Cell cell);

/**
 * @brief 绘制道具（按类型使用不同几何形状与颜色）。
 * @param cell 道具所在格子。
 * @param type 道具类型；ITEM_NONE 时不绘制。
 */
void render_draw_item(Cell cell, ItemType type);

/**
 * @brief 绘制顶部 HUD（分数、最高分、等级、速度、道具倒计时）。
 * @param hud HUD 数据指针，不可为 NULL。
 */
void render_draw_hud(const HudInfo *hud);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_UI_RENDER_H */
