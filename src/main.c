/* ============================================================================
 * SnakeGame 主程序（P6/P8：完整可玩版本）
 * ----------------------------------------------------------------------------
 * 主循环职责：
 *   1. 轮询输入（方向/暂停/重开/退出/全屏）并驱动 core 逻辑；
 *   2. 以“固定步长 + 时间累加器”的方式按有效速度触发 game_step；
 *   3. 在相邻两次逻辑位置之间做插值，交给 ui/render 平滑绘制；
 *   4. 驱动道具刷新/过期与效果计时，播放合成音效；
 *   5. 最高分读写存档；
 *   6. 通过“虚拟分辨率 + 缩放绘制”支持窗口放大与 F11 全屏。
 *
 * 说明：游戏规则全部在 core 中，本文件只做“调度 + 渲染 + 输入”，不含规则判断。
 * ==========================================================================*/

#include <math.h>
#include <time.h>

#include <raylib.h>

#include "core/game.h"
#include "core/persist.h"
#include "ui/audio.h"
#include "ui/input.h"
#include "ui/render.h"
#include "ui/theme.h"

#define MAX_SNAKE_CELLS (GRID_COLS * GRID_ROWS)   /**< 蛇身最多占满整个棋盘 */
#define SAVE_PATH       "highscore.dat"           /**< 最高分存档文件 */

/**
 * @brief 虚拟分辨率到实际窗口的映射参数。
 *
 * 渲染始终按固定的 WINDOW_WIDTH x WINDOW_HEIGHT 进行，再整体等比缩放到窗口，
 * 从而在放大窗口或全屏时保持布局与比例不变（带黑边）。
 */
typedef struct {
    float scale;   /**< 缩放比例 */
    float ox;      /**< 水平黑边偏移 */
    float oy;      /**< 垂直黑边偏移 */
} Viewport;

/** @brief 根据当前窗口尺寸计算等比缩放视口。 */
static Viewport compute_viewport(void)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scale = fminf((float)sw / (float)WINDOW_WIDTH,
                        (float)sh / (float)WINDOW_HEIGHT);
    if (scale <= 0.0f) {
        scale = 1.0f;
    }
    Viewport vp;
    vp.scale = scale;
    vp.ox = ((float)sw - (float)WINDOW_WIDTH * scale) * 0.5f;
    vp.oy = ((float)sh - (float)WINDOW_HEIGHT * scale) * 0.5f;
    return vp;
}

/** @brief 把窗口坐标换算为虚拟分辨率坐标（用于鼠标命中检测）。 */
static Vector2 to_virtual(Viewport vp, Vector2 screen)
{
    return (Vector2){ (screen.x - vp.ox) / vp.scale, (screen.y - vp.oy) / vp.scale };
}

/** @brief 把当前蛇身坐标快照到数组（索引 0 为蛇头）。 */
static int snapshot_snake(const GameState *game, Cell *out)
{
    int n = 0;
    const SnakeNode *sentinel = game->snake->sentinel;
    for (const SnakeNode *cur = sentinel->next;
         cur != sentinel && n < MAX_SNAKE_CELLS;
         cur = cur->next) {
        out[n].x = cur->x;
        out[n].y = cur->y;
        ++n;
    }
    return n;
}

int main(void)
{
    /* 允许窗口缩放；使用垂直同步以获得更平滑的帧节奏（不再叠加 SetTargetFPS，避免双重限帧导致抖动） */
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "SnakeGame");

    /* 虚拟分辨率画布：所有绘制都先画到这里，再整体缩放到窗口 */
    RenderTexture2D canvas = LoadRenderTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    audio_init();

    /* 创建游戏并载入历史最高分 */
    GameState *game = game_create((uint32_t)time(NULL));
    if (game == NULL) {
        audio_shutdown();
        UnloadRenderTexture(canvas);
        CloseWindow();
        return 1;
    }
    int saved_high = persist_load_high_score(SAVE_PATH);
    if (saved_high > game->high_score) {
        game->high_score = saved_high;
    }

    /* 插值所需：上一步位置(prev) 与当前位置(cur) */
    Cell cur[MAX_SNAKE_CELLS];
    Cell prev[MAX_SNAKE_CELLS];
    Cell from[MAX_SNAKE_CELLS];
    int cur_count = snapshot_snake(game, cur);
    int prev_count = cur_count;
    for (int i = 0; i < cur_count; ++i) {
        prev[i] = cur[i];
    }

    double accumulator = 0.0;

    /* 用于触发音效/存档的状态快照 */
    int prev_score = game->score;
    int prev_level = game->level;
    GamePhase prev_phase = game->phase;

    /* 蛇身变色：记录最近拾取的道具类型；一旦变色就永久保留，直到再次吃到其它道具 */
    ItemType tint_item = ITEM_NONE;

    int running = 1;
    while (running && !WindowShouldClose())
    {
        Viewport vp = compute_viewport();

        /* ---------------------------- 全屏切换 ---------------------------- */
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        /* ---------------------------- 键盘输入 ---------------------------- */
        InputAction action = input_poll();
        if (action == INPUT_ACTION_QUIT) {
            break;
        }

        /* ---------------------------- 鼠标输入 ---------------------------- */
        int mouse_clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        Vector2 vmouse = to_virtual(vp, GetMousePosition());
        int hover_restart = CheckCollisionPointRec(vmouse, render_restart_button_rect());
        int hover_quit = CheckCollisionPointRec(vmouse, render_quit_button_rect());

        if (action == INPUT_ACTION_RESTART ||
            (mouse_clicked && game->phase == PHASE_GAMEOVER && hover_restart)) {
            game_reset(game);
            if (saved_high > game->high_score) {
                game->high_score = saved_high;
            }
            cur_count = snapshot_snake(game, cur);
            prev_count = cur_count;
            for (int i = 0; i < cur_count; ++i) {
                prev[i] = cur[i];
            }
            accumulator = 0.0;
            tint_item = ITEM_NONE;
        } else if (mouse_clicked && game->phase == PHASE_GAMEOVER && hover_quit) {
            break;
        } else if (action == INPUT_ACTION_PAUSE) {
            if (game->phase == PHASE_PLAYING) {
                game->phase = PHASE_PAUSED;
            } else if (game->phase == PHASE_PAUSED) {
                game->phase = PHASE_PLAYING;
            }
        } else {
            switch (action) {
                case INPUT_ACTION_UP:    game_set_direction(game, DIR_UP);    break;
                case INPUT_ACTION_DOWN:  game_set_direction(game, DIR_DOWN);  break;
                case INPUT_ACTION_LEFT:  game_set_direction(game, DIR_LEFT);  break;
                case INPUT_ACTION_RIGHT: game_set_direction(game, DIR_RIGHT); break;
                default: break;
            }
        }

        /* -------------------- 道具刷新/过期与效果计时 -------------------- */
        float dt = GetFrameTime();
        game_items_update(game, dt);
        game_effects_update(game, dt);

        /* -------------------- 固定步长驱动逻辑步进 -------------------- */
        int effective_ms = game_effective_speed_ms(game);
        double interval = (double)effective_ms / 1000.0;
        if (game->phase == PHASE_PLAYING) {
            accumulator += (double)dt;
            int guard = 0;
            while (accumulator >= interval && game->phase == PHASE_PLAYING && guard++ < 8) {
                prev_count = cur_count;
                for (int i = 0; i < cur_count; ++i) {
                    prev[i] = cur[i];
                }

                game_step(game);

                cur_count = snapshot_snake(game, cur);
                accumulator -= interval;
                interval = (double)game_effective_speed_ms(game) / 1000.0;
            }
            if (accumulator > interval) {
                accumulator = interval;   /* 防止长时间卡顿后累积过多 */
            }
        }

        /* ---------------------------- 音效事件 ---------------------------- */
        if (game->last_pickup_event != ITEM_NONE) {
            /* 吃到什么道具，蛇就一直保持该颜色（不再变回默认色） */
            tint_item = game->last_pickup_event;
            audio_play(SFX_EAT_ITEM);
            game->last_pickup_event = ITEM_NONE;
        }
        if (game->score > prev_score) {
            audio_play(SFX_EAT_FOOD);
        }
        if (game->level > prev_level) {
            audio_play(SFX_LEVEL_UP);
        }
        if (game->phase == PHASE_GAMEOVER && prev_phase != PHASE_GAMEOVER) {
            audio_play(SFX_DIE);
            if (game->score > saved_high) {
                saved_high = game->score;
                persist_save_high_score(SAVE_PATH, saved_high);
            }
        }
        prev_score = game->score;
        prev_level = game->level;
        prev_phase = game->phase;

        /* 插值系数：0=上一步位置，1=当前位置 */
        float t = 1.0f;
        if (game->phase == PHASE_PLAYING && interval > 0.0) {
            t = (float)(accumulator / interval);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
        }

        /* 组装插值起点数组：变长时新增的尾节没有历史位置，直接用当前位置 */
        for (int i = 0; i < cur_count; ++i) {
            from[i] = (i < prev_count) ? prev[i] : cur[i];
        }

        /* 根据最近拾取的道具决定蛇身主色 */
        Color snake_color = COLOR_SNAKE_HEAD;
        switch (tint_item) {
            case ITEM_SPEED:  snake_color = COLOR_ITEM_SPEED;  break;
            case ITEM_SLOW:   snake_color = COLOR_ITEM_SLOW;   break;
            case ITEM_SHIELD: snake_color = COLOR_ITEM_SHIELD; break;
            case ITEM_SHRINK: snake_color = COLOR_ITEM_SHRINK; break;
            default:          snake_color = COLOR_SNAKE_HEAD;  break;
        }

        /* ------------------------- 绘制到虚拟画布 ------------------------- */
        BeginTextureMode(canvas);
        render_draw_background();
        render_draw_playfield();

        /* 先画食物/障碍/道具，再画蛇，保证蛇在最上层 */
        render_draw_food(game->food);
        for (int i = 0; i < game->obstacle_count; ++i) {
            render_draw_obstacle(game->obstacles[i]);
        }
        for (int i = 0; i < MAX_ITEMS; ++i) {
            if (game->items[i].type != ITEM_NONE) {
                render_draw_item(game->items[i].cell, game->items[i].type);
            }
        }
        render_draw_snake(from, cur, cur_count, game->dir, t, snake_color);

        HudInfo hud = {
            .score = game->score,
            .high_score = game->high_score,
            .level = game->level,
            .speed_ms = effective_ms,
            .speed_effect_sec = game->effects.speed_sec,
            .slow_effect_sec = game->effects.slow_sec,
            .shield_effect_sec = game->effects.shield_sec
        };
        render_draw_hud(&hud);
        render_draw_hint_bar();

        if (game->phase == PHASE_PAUSED) {
            render_draw_pause_overlay();
        } else if (game->phase == PHASE_GAMEOVER) {
            render_draw_gameover_overlay(game->score, game->high_score);

            /* 鼠标悬停高亮，提升“按钮”可点击感 */
            if (hover_restart) {
                DrawRectangleLinesEx(render_restart_button_rect(), 3.0f, WHITE);
            } else if (hover_quit) {
                DrawRectangleLinesEx(render_quit_button_rect(), 3.0f, WHITE);
            }
        }
        EndTextureMode();

        /* ---------------------- 把画布缩放到窗口显示 ---------------------- */
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            canvas.texture,
            (Rectangle){ 0.0f, 0.0f, (float)WINDOW_WIDTH, -(float)WINDOW_HEIGHT },
            (Rectangle){ vp.ox, vp.oy,
                         (float)WINDOW_WIDTH * vp.scale,
                         (float)WINDOW_HEIGHT * vp.scale },
            (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
        EndDrawing();
    }

    (void)running;
    game_destroy(game);
    audio_shutdown();
    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}
