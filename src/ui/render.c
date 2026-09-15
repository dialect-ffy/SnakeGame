/* ============================================================================
 * SnakeGame UI 层 —— 渲染实现（raylib）
 * ----------------------------------------------------------------------------
 * 本文件依赖 raylib，只读取传入的数据进行绘制，不修改游戏状态。
 * 所有坐标换算都基于 theme.h 中的布局常量。
 * ==========================================================================*/

#include "ui/render.h"
#include "ui/theme.h"

#include <math.h>

/* 蛇身每节在视觉上占 2x2 个细格：蛇看起来更粗壮，而每步位移只有 1 格，
 * 因此移动显得更细腻平滑。道具/食物仍只占 1 格。 */
#define SNAKE_BLOCK_SIZE (2.0f * (float)CELL_SIZE)

/* ---------------------------------------------------------------------------
 * 内部辅助函数
 * -------------------------------------------------------------------------*/

/**
 * @brief 把格子坐标换算为格子中心点的像素坐标。
 */
static Vector2 cell_center(Cell c)
{
    return (Vector2){
        (float)(PLAYFIELD_X + c.x * CELL_SIZE) + CELL_SIZE * 0.5f,
        (float)(PLAYFIELD_Y + c.y * CELL_SIZE) + CELL_SIZE * 0.5f
    };
}

/**
 * @brief 对两个二维向量做线性插值。
 */
static Vector2 lerp_vec(Vector2 a, Vector2 b, float t)
{
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

/**
 * @brief 把体节中心点限制在游戏区内，避免 2x2 的粗体节越出霓虹边框。
 * @param p    体节中心（像素）。
 * @param half 体节半边长（像素）。
 */
static Vector2 clamp_center_to_playfield(Vector2 p, float half)
{
    float min_x = (float)PLAYFIELD_X + half;
    float max_x = (float)(PLAYFIELD_X + PLAYFIELD_WIDTH) - half;
    float min_y = (float)PLAYFIELD_Y + half;
    float max_y = (float)(PLAYFIELD_Y + PLAYFIELD_HEIGHT) - half;

    if (p.x < min_x) p.x = min_x;
    if (p.x > max_x) p.x = max_x;
    if (p.y < min_y) p.y = min_y;
    if (p.y > max_y) p.y = max_y;
    return p;
}

/**
 * @brief 绘制一个“发光圆点”：用多层半透明圆近似霓虹光晕。
 * @param center 圆心。
 * @param radius 实心半径。
 * @param color  主色。
 */
static void draw_glow_dot(Vector2 center, float radius, Color color)
{
    DrawCircleV(center, radius * 2.2f, theme_alpha(color, 30));
    DrawCircleV(center, radius * 1.6f, theme_alpha(color, 55));
    DrawCircleV(center, radius * 1.2f, theme_alpha(color, 90));
    DrawCircleV(center, radius, color);
}

/**
 * @brief 绘制一个带霓虹光晕的圆角矩形（用于蛇身体节）。
 */
static void draw_glow_rounded(Rectangle rect, Color color)
{
    Rectangle glow = { rect.x - 1.5f, rect.y - 1.5f, rect.width + 3.0f, rect.height + 3.0f };
    DrawRectangleRounded(glow, 0.45f, 8, theme_alpha(color, 45));
    DrawRectangleRounded(rect, 0.45f, 8, color);
}

/**
 * @brief 在指定中心点绘制一个带霓虹光晕的圆角方形体节。
 * @param center 体节中心像素坐标。
 * @param size   体节边长（像素）。
 * @param color  体节颜色。
 */
static void draw_segment_at(Vector2 center, float size, Color color)
{
    Rectangle r = { center.x - size * 0.5f, center.y - size * 0.5f, size, size };
    draw_glow_rounded(r, color);
}

/**
 * @brief 根据朝向返回一个单位方向向量（用于绘制蛇眼位置）。
 */
static Vector2 dir_vector(Direction dir)
{
    switch (dir) {
        case DIR_UP:    return (Vector2){ 0.0f, -1.0f };
        case DIR_DOWN:  return (Vector2){ 0.0f,  1.0f };
        case DIR_LEFT:  return (Vector2){ -1.0f, 0.0f };
        case DIR_RIGHT: return (Vector2){ 1.0f,  0.0f };
        default:        return (Vector2){ 1.0f,  0.0f };
    }
}

/* ---------------------------------------------------------------------------
 * 公开接口实现
 * -------------------------------------------------------------------------*/

void render_draw_background(void)
{
    ClearBackground(COLOR_BACKGROUND);
}

void render_draw_playfield(void)
{
    /* 游戏区底板 */
    DrawRectangle(PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT, COLOR_PLAYFIELD_BG);

    /* 淡网格线：细密网格下每 2 格画一条，降低视觉噪声；不画最外圈避免与边框重叠 */
    const int LINE_STEP = 2;
    for (int col = LINE_STEP; col < GRID_COLS; col += LINE_STEP) {
        int px = PLAYFIELD_X + col * CELL_SIZE;
        DrawLine(px, PLAYFIELD_Y, px, PLAYFIELD_Y + PLAYFIELD_HEIGHT, COLOR_GRID_LINE);
    }
    for (int row = LINE_STEP; row < GRID_ROWS; row += LINE_STEP) {
        int py = PLAYFIELD_Y + row * CELL_SIZE;
        DrawLine(PLAYFIELD_X, py, PLAYFIELD_X + PLAYFIELD_WIDTH, py, COLOR_GRID_LINE);
    }

    /* 霓虹边框（外发光 + 实线） */
    Rectangle border = {
        (float)PLAYFIELD_X, (float)PLAYFIELD_Y,
        (float)PLAYFIELD_WIDTH, (float)PLAYFIELD_HEIGHT
    };
    DrawRectangleLinesEx((Rectangle){ border.x - 2, border.y - 2, border.width + 4, border.height + 4 },
                         2.0f, theme_alpha(COLOR_BORDER, 40));
    DrawRectangleLinesEx(border, 2.0f, COLOR_BORDER);
}

void render_draw_snake(const Cell *from_cells, const Cell *to_cells,
                       int count, Direction head_dir, float t, Color base_color)
{
    if (to_cells == NULL || count <= 0) {
        return;
    }
    if (from_cells == NULL) {
        from_cells = to_cells;   /* 无历史位置时退化为静态绘制 */
    }
    if (t < 0.0f) {
        t = 0.0f;
    } else if (t > 1.0f) {
        t = 1.0f;
    }

    /* 蛇尾颜色：主色向深色过渡，形成渐变 */
    Color tail_color = theme_lerp_color(base_color, (Color){ 8, 16, 40, 255 }, 0.65f);

    /* 先画身体（从尾到头），保证蛇头绘制在最上层 */
    for (int i = count - 1; i >= 0; --i) {
        /* 渐变系数：0=蛇头，1=蛇尾 */
        float grad = (count > 1) ? (float)i / (float)(count - 1) : 0.0f;
        Color color = theme_lerp_color(base_color, tail_color, grad);

        /* 在“上一步位置”与“当前位置”之间插值，形成平滑移动 */
        Vector2 from = cell_center(from_cells[i]);
        Vector2 to = cell_center(to_cells[i]);
        Vector2 pos = lerp_vec(from, to, t);

        /* 每节占 2x2 格：头略大，身体略小 */
        float size = (i == 0) ? (SNAKE_BLOCK_SIZE - 2.0f) : (SNAKE_BLOCK_SIZE - 4.0f);
        pos = clamp_center_to_playfield(pos, size * 0.5f);
        draw_segment_at(pos, size, color);
    }

    /* 在插值后的蛇头位置绘制朝向眼睛 */
    Vector2 head_c = lerp_vec(cell_center(from_cells[0]), cell_center(to_cells[0]), t);
    float head_size = SNAKE_BLOCK_SIZE - 2.0f;
    head_c = clamp_center_to_playfield(head_c, head_size * 0.5f);

    Vector2 fwd = dir_vector(head_dir);
    Vector2 side = (Vector2){ -fwd.y, fwd.x };   /* 前向向量旋转 90 度，得到侧向 */

    float eye_fwd = head_size * 0.17f;   /* 眼睛相对中心的前向偏移 */
    float eye_side = head_size * 0.20f;  /* 眼睛相对中心的侧向偏移 */
    float eye_r = head_size * 0.10f;     /* 眼睛半径 */

    for (int s = -1; s <= 1; s += 2) {
        Vector2 eye = {
            head_c.x + fwd.x * eye_fwd + side.x * eye_side * (float)s,
            head_c.y + fwd.y * eye_fwd + side.y * eye_side * (float)s
        };
        DrawCircleV(eye, eye_r, COLOR_SNAKE_EYE);
    }
}

void render_draw_food(Cell cell)
{
    Vector2 c = cell_center(cell);
    draw_glow_dot(c, CELL_SIZE * 0.42f, COLOR_FOOD);
}

void render_draw_obstacle(Cell cell)
{
    Vector2 c = cell_center(cell);
    float size = CELL_SIZE - 3.0f;
    Rectangle r = { c.x - size * 0.5f, c.y - size * 0.5f, size, size };

    /* 外发光 + 实心方块，形成霓虹“墙块” */
    DrawRectangleRounded((Rectangle){ r.x - 1.5f, r.y - 1.5f, r.width + 3.0f, r.height + 3.0f },
                         0.25f, 6, theme_alpha(COLOR_OBSTACLE, 50));
    DrawRectangleRounded(r, 0.25f, 6, COLOR_OBSTACLE);
    DrawRectangleRounded((Rectangle){ r.x + size * 0.18f, r.y + size * 0.18f,
                                      size * 0.64f, size * 0.64f },
                         0.25f, 6, theme_alpha((Color){ 255, 255, 255, 255 }, 40));
}

void render_draw_item(Cell cell, ItemType type)
{
    Vector2 c = cell_center(cell);
    float r = CELL_SIZE * 0.42f;   /* 道具占 1 格 */

    switch (type) {
        case ITEM_SPEED: {
            /* 加速：黄色向上三角形（象征“快”） */
            DrawCircleV(c, r * 1.5f, theme_alpha(COLOR_ITEM_SPEED, 30));
            Vector2 p1 = { c.x, c.y - r };
            Vector2 p2 = { c.x - r, c.y + r };
            Vector2 p3 = { c.x + r, c.y + r };
            DrawTriangle(p1, p3, p2, COLOR_ITEM_SPEED);
            DrawTriangleLines(p1, p3, p2, theme_alpha(COLOR_ITEM_SPEED, 120));
            break;
        }
        case ITEM_SLOW: {
            /* 减速：蓝色圆环（象征“刹车/环”） */
            DrawCircleV(c, r * 1.5f, theme_alpha(COLOR_ITEM_SLOW, 30));
            DrawRing(c, r * 0.45f, r, 0.0f, 360.0f, 32, COLOR_ITEM_SLOW);
            break;
        }
        case ITEM_SHRINK: {
            /* 缩短：紫红双向箭头（象征“变短”） */
            DrawCircleV(c, r * 1.5f, theme_alpha(COLOR_ITEM_SHRINK, 30));
            Vector2 left  = { c.x - r,       c.y };
            Vector2 right = { c.x + r,       c.y };
            DrawTriangle((Vector2){ c.x - r * 0.2f, c.y - r * 0.7f },
                         (Vector2){ c.x - r * 0.2f, c.y + r * 0.7f }, left, COLOR_ITEM_SHRINK);
            DrawTriangle((Vector2){ c.x + r * 0.2f, c.y + r * 0.7f },
                         (Vector2){ c.x + r * 0.2f, c.y - r * 0.7f }, right, COLOR_ITEM_SHRINK);
            DrawLineEx((Vector2){ c.x - r * 0.2f, c.y }, (Vector2){ c.x + r * 0.2f, c.y },
                       CELL_SIZE * 0.14f, COLOR_ITEM_SHRINK);
            break;
        }
        case ITEM_SHIELD: {
            /* 护盾：白色六边形（象征“护甲”） */
            DrawCircleV(c, r * 1.5f, theme_alpha(COLOR_ITEM_SHIELD, 30));
            DrawPoly(c, 6, r, 0.0f, COLOR_ITEM_SHIELD);
            DrawPolyLinesEx(c, 6, r * 0.62f, 0.0f, 1.5f, theme_alpha(COLOR_ITEM_SHIELD, 160));
            break;
        }
        case ITEM_NONE:
        default:
            break;   /* 空槽位不绘制 */
    }
}

void render_draw_hud(const HudInfo *hud)
{
    if (hud == NULL) {
        return;
    }

    /* HUD 背景与底部霓虹分隔线 */
    DrawRectangle(0, 0, WINDOW_WIDTH, HUD_HEIGHT, COLOR_HUD_BG);
    DrawLine(0, HUD_HEIGHT, WINDOW_WIDTH, HUD_HEIGHT, theme_alpha(COLOR_HUD_ACCENT, 90));

    /* 左侧：分数 / 最高分 / 等级 / 速度 */
    DrawText("SCORE", 32, 10, 12, COLOR_HUD_DIM);
    DrawText(TextFormat("%d", hud->score), 32, 26, 24, COLOR_HUD_ACCENT);

    DrawText("HIGH", 210, 10, 12, COLOR_HUD_DIM);
    DrawText(TextFormat("%d", hud->high_score), 210, 26, 24, COLOR_HUD_TEXT);

    DrawText("LEVEL", 380, 10, 12, COLOR_HUD_DIM);
    DrawText(TextFormat("%d", hud->level), 380, 26, 24, COLOR_HUD_TEXT);

    DrawText("SPEED", 520, 10, 12, COLOR_HUD_DIM);
    DrawText(TextFormat("%d ms", hud->speed_ms), 520, 26, 24, COLOR_HUD_TEXT);

    /* 右侧：当前生效道具倒计时 */
    int x = 760;
    DrawText("EFFECTS", x, 10, 12, COLOR_HUD_DIM);

    int tx = x;
    if (hud->speed_effect_sec > 0.0f) {
        DrawText(TextFormat("SPD %.1fs", hud->speed_effect_sec), tx, 28, 18, COLOR_ITEM_SPEED);
        tx += 90;
    }
    if (hud->slow_effect_sec > 0.0f) {
        DrawText(TextFormat("SLW %.1fs", hud->slow_effect_sec), tx, 28, 18, COLOR_ITEM_SLOW);
        tx += 90;
    }
    if (hud->shield_effect_sec > 0.0f) {
        DrawText(TextFormat("SHD %.1fs", hud->shield_effect_sec), tx, 28, 18, COLOR_ITEM_SHIELD);
        tx += 90;
    }
    if (tx == x) {
        DrawText("none", x, 28, 18, COLOR_HUD_DIM);
    }
}

void render_draw_pause_overlay(void)
{
    /* 半透明遮罩压暗游戏区 */
    DrawRectangle(PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT,
                  (Color){ 0, 0, 0, 150 });

    const char *title = "PAUSED";
    const char *hint = "Press P to resume";
    int title_size = 56;
    int hint_size = 20;

    int tw = MeasureText(title, title_size);
    int hw = MeasureText(hint, hint_size);
    int cx = PLAYFIELD_X + PLAYFIELD_WIDTH / 2;

    DrawText(title, cx - tw / 2, PLAYFIELD_Y + PLAYFIELD_HEIGHT / 2 - 60,
             title_size, COLOR_HUD_ACCENT);
    DrawText(hint, cx - hw / 2, PLAYFIELD_Y + PLAYFIELD_HEIGHT / 2 + 10,
             hint_size, COLOR_HUD_TEXT);
}

void render_draw_gameover_overlay(int score, int high_score)
{
    DrawRectangle(PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT,
                  (Color){ 0, 0, 0, 175 });

    int cx = PLAYFIELD_X + PLAYFIELD_WIDTH / 2;
    int cy = PLAYFIELD_Y + PLAYFIELD_HEIGHT / 2;

    const char *title = "GAME OVER";
    int title_size = 56;
    int tw = MeasureText(title, title_size);
    DrawText(title, cx - tw / 2, cy - 150, title_size, (Color){ 255, 90, 120, 255 });

    const char *score_text = TextFormat("Score: %d", score);
    const char *high_text = TextFormat("Best:  %d", high_score);
    int st = 30;
    DrawText(score_text, cx - MeasureText(score_text, st) / 2, cy - 60, st, COLOR_HUD_TEXT);
    DrawText(high_text, cx - MeasureText(high_text, st) / 2, cy - 20, st, COLOR_HUD_ACCENT);

    /* 可点击按钮：重新开始 / 退出 */
    Rectangle restart = render_restart_button_rect();
    Rectangle quit = render_quit_button_rect();

    DrawRectangleRounded(restart, 0.35f, 8, theme_alpha(COLOR_HUD_ACCENT, 230));
    DrawRectangleRounded(quit, 0.35f, 8, theme_alpha((Color){ 90, 100, 120, 255 }, 230));

    const char *restart_label = "RESTART (R)";
    const char *quit_label = "QUIT (Esc)";
    int ls = 24;
    DrawText(restart_label, (int)(restart.x + (restart.width - MeasureText(restart_label, ls)) / 2),
             (int)(restart.y + (restart.height - ls) / 2), ls, (Color){ 8, 16, 28, 255 });
    DrawText(quit_label, (int)(quit.x + (quit.width - MeasureText(quit_label, ls)) / 2),
             (int)(quit.y + (quit.height - ls) / 2), ls, (Color){ 235, 240, 255, 255 });
}

Rectangle render_restart_button_rect(void)
{
    const float w = 280.0f;
    const float h = 58.0f;
    float cx = (float)(PLAYFIELD_X + PLAYFIELD_WIDTH / 2);
    float y = (float)(PLAYFIELD_Y + PLAYFIELD_HEIGHT / 2 + 60);
    return (Rectangle){ cx - w * 0.5f, y, w, h };
}

Rectangle render_quit_button_rect(void)
{
    const float w = 280.0f;
    const float h = 58.0f;
    float cx = (float)(PLAYFIELD_X + PLAYFIELD_WIDTH / 2);
    float y = (float)(PLAYFIELD_Y + PLAYFIELD_HEIGHT / 2 + 130);
    return (Rectangle){ cx - w * 0.5f, y, w, h };
}

void render_draw_hint_bar(void)
{
    const char *hint = "F11 Fullscreen   |   P Pause   |   R Restart   |   Esc Quit";
    int fs = 18;
    int y = PLAYFIELD_Y + PLAYFIELD_HEIGHT + 20;
    DrawText(hint, (WINDOW_WIDTH - MeasureText(hint, fs)) / 2, y, fs, COLOR_HUD_DIM);
}
