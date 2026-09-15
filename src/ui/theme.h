/* ============================================================================
 * SnakeGame UI 层 —— 霓虹暗黑风主题与布局常量
 * ----------------------------------------------------------------------------
 * 集中管理窗口尺寸、网格尺寸、配色，避免渲染代码中出现“魔法数字”。
 * 本文件属于 UI 层，依赖 raylib 的颜色类型。
 * ==========================================================================*/

#ifndef SNAKE_UI_THEME_H
#define SNAKE_UI_THEME_H

#include <raylib.h>

#include "core/config.h"   /* 复用 GRID_COLS / GRID_ROWS，避免重复定义 */

/* ---------------------------------------------------------------------------
 * 布局常量
 * -------------------------------------------------------------------------*/
#define WINDOW_WIDTH      1120                                   /**< 窗口宽 */
#define WINDOW_HEIGHT     840                                    /**< 窗口高 */

#define CELL_SIZE         15                                     /**< 每格像素（细密网格） */

#define HUD_HEIGHT        60                                     /**< 顶部 HUD 高度 */

#define PLAYFIELD_WIDTH   (GRID_COLS * CELL_SIZE)                /**< 游戏区宽 960 */
#define PLAYFIELD_HEIGHT  (GRID_ROWS * CELL_SIZE)                /**< 游戏区高 720 */

/* 游戏区居中：左右各留 (1120-960)/2 = 80；上边距为 HUD(60)，下边距同为 60 */
#define PLAYFIELD_X       ((WINDOW_WIDTH - PLAYFIELD_WIDTH) / 2)
#define PLAYFIELD_Y       (HUD_HEIGHT)

/* ---------------------------------------------------------------------------
 * 配色（霓虹暗黑风）
 * 使用 raylib 的 CLITERAL 宏，兼容 C 与 C++ 两种编译方式。
 * -------------------------------------------------------------------------*/
#define COLOR_BACKGROUND   CLITERAL(Color){ 10,  12,  20,  255 }  /**< 窗口底色 */
#define COLOR_PLAYFIELD_BG CLITERAL(Color){ 14,  18,  30,  255 }  /**< 游戏区底色 */
#define COLOR_GRID_LINE    CLITERAL(Color){ 30,  38,  58,  255 }  /**< 淡网格线（细密网格下调暗，减少视觉干扰） */
#define COLOR_BORDER       CLITERAL(Color){ 0,   224, 255, 235 }  /**< 游戏区霓虹边框 */

#define COLOR_SNAKE_HEAD   CLITERAL(Color){ 0,   255, 220, 255 }  /**< 蛇头色（亮青） */
#define COLOR_SNAKE_TAIL   CLITERAL(Color){ 0,   90,  200, 255 }  /**< 蛇尾色（深蓝） */
#define COLOR_SNAKE_EYE    CLITERAL(Color){ 8,   16,  28,  255 }  /**< 蛇眼颜色 */

#define COLOR_FOOD         CLITERAL(Color){ 0,   255, 210, 255 }  /**< 普通食物（青） */

#define COLOR_OBSTACLE     CLITERAL(Color){ 220, 64,  96,  255 }  /**< 障碍物（霓虹红） */

#define COLOR_ITEM_SPEED   CLITERAL(Color){ 255, 214, 0,   255 }  /**< 加速（黄三角） */
#define COLOR_ITEM_SLOW    CLITERAL(Color){ 90,  150, 255, 255 }  /**< 减速（蓝圆环） */
#define COLOR_ITEM_SHRINK  CLITERAL(Color){ 224, 64,  200, 255 }  /**< 缩短（紫红箭头） */
#define COLOR_ITEM_SHIELD  CLITERAL(Color){ 235, 240, 255, 255 }  /**< 护盾（白六边形） */

#define COLOR_HUD_BG       CLITERAL(Color){ 12,  15,  26,  255 }  /**< HUD 背景 */
#define COLOR_HUD_TEXT     CLITERAL(Color){ 198, 212, 236, 255 }  /**< HUD 常规文字 */
#define COLOR_HUD_ACCENT   CLITERAL(Color){ 0,   229, 255, 255 }  /**< HUD 强调色 */
#define COLOR_HUD_DIM      CLITERAL(Color){ 126, 142, 172, 255 }  /**< HUD 次要文字 */

/**
 * @brief 在两个颜色之间做线性插值（用于蛇身渐变）。
 * @param a 起始颜色（t=0）。
 * @param b 结束颜色（t=1）。
 * @param t 插值系数，会被自动夹取到 [0, 1]。
 * @return 插值后的颜色。
 */
Color theme_lerp_color(Color a, Color b, float t);

/**
 * @brief 基于指定颜色生成一个带透明度的颜色（用于发光/阴影效果）。
 * @param c     原始颜色。
 * @param alpha 目标透明度（0~255）。
 * @return 替换 alpha 后的颜色。
 */
Color theme_alpha(Color c, unsigned char alpha);

#endif /* SNAKE_UI_THEME_H */
