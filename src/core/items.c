/* ============================================================================
 * SnakeGame 核心逻辑层 —— 道具系统与效果实现
 * ----------------------------------------------------------------------------
 * 职责：
 *   - 道具的定时刷新、存活倒计时与过期清除；
 *   - 拾取道具并应用效果（加速/减速/缩短/护盾）；
 *   - 效果计时推进与“有效速度”计算；
 *   - 难度成长时障碍的动态补充（配合 game.c 的 update_obstacles）。
 * 约束：纯 C，无 raylib 依赖，随机行为经由可注入种子的 Rng，便于测试复现。
 * ==========================================================================*/

#include "core/game.h"

#include <stddef.h>

/* ---------------------------------------------------------------------------
 * 内部辅助函数
 * -------------------------------------------------------------------------*/

/** @brief 统计当前地图上存在的道具数量。 */
static int item_count(const GameState *state)
{
    int n = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (state->items[i].type != ITEM_NONE) {
            ++n;
        }
    }
    return n;
}

/** @brief 生成 [ITEM_SPAWN_MIN_SEC, ITEM_SPAWN_MAX_SEC] 之间的随机刷新间隔。 */
static float random_spawn_delay(GameState *state)
{
    int r = rng_range(&state->rng, 0, 1000);
    return ITEM_SPAWN_MIN_SEC +
           (ITEM_SPAWN_MAX_SEC - ITEM_SPAWN_MIN_SEC) * ((float)r / 1000.0f);
}

/** @brief 等概率随机选择一种道具类型。 */
static ItemType pick_random_item_type(GameState *state)
{
    switch (rng_range(&state->rng, 0, 3)) {
        case 0:  return ITEM_SPEED;
        case 1:  return ITEM_SLOW;
        case 2:  return ITEM_SHRINK;
        default: return ITEM_SHIELD;
    }
}

/**
 * @brief 判断格子是否适合自动生成道具。
 * @return 非 0 表示可放置。
 */
static int cell_free_for_item(GameState *state, Cell c)
{
    if (!game_is_inside_board(c)) {
        return 0;
    }
    if (game_is_cell_on_snake(state, c)) {
        return 0;
    }
    if (game_is_obstacle(state, c)) {
        return 0;
    }
    if (c.x == state->food.x && c.y == state->food.y) {
        return 0;
    }
    if (game_is_cell_on_item(state, c)) {
        return 0;
    }
    if (game_is_in_birth_zone(c)) {
        return 0;
    }
    return 1;
}

/**
 * @brief 在空槽位生成一个道具。
 * @return 1 表示生成成功，0 表示无空槽或无空位。
 */
static int spawn_item(GameState *state)
{
    int slot = -1;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (state->items[i].type == ITEM_NONE) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return 0;   /* 无空槽 */
    }

    Cell c;
    if (!game_find_free_cell(state, cell_free_for_item, &c)) {
        return 0;   /* 无空位 */
    }

    state->items[slot].type = pick_random_item_type(state);
    state->items[slot].cell = c;
    state->items[slot].remaining_sec = ITEM_LIFETIME_SEC;
    return 1;
}

/**
 * @brief 从蛇尾删除若干节（缩短道具效果），但保证长度不低于 MIN_SNAKE_LENGTH。
 */
static void shrink_snake(GameState *state, int amount)
{
    int len = snake_list_length(state->snake);
    int removable = len - MIN_SNAKE_LENGTH;
    if (removable <= 0) {
        return;   /* 已达最小长度，不再缩短 */
    }
    if (amount > removable) {
        amount = removable;
    }
    for (int i = 0; i < amount; ++i) {
        snake_list_pop_back(state->snake);
    }
}

/**
 * @brief 应用道具效果。同类效果采用“刷新时长”而非叠加。
 */
static void apply_item_effect(GameState *state, ItemType type)
{
    switch (type) {
        case ITEM_SPEED:
            state->effects.speed_sec = EFFECT_DURATION_SEC;   /* 刷新时长 */
            break;
        case ITEM_SLOW:
            state->effects.slow_sec = EFFECT_DURATION_SEC;
            break;
        case ITEM_SHIELD:
            state->effects.shield_sec = EFFECT_DURATION_SEC;
            break;
        case ITEM_SHRINK:
            shrink_snake(state, SHRINK_AMOUNT);               /* 立即生效 */
            break;
        case ITEM_NONE:
        default:
            break;
    }
}

/* ---------------------------------------------------------------------------
 * 公开接口实现
 * -------------------------------------------------------------------------*/

int game_is_cell_on_item(const GameState *state, Cell c)
{
    if (state == NULL) {
        return 0;
    }
    for (int i = 0; i < MAX_ITEMS; ++i) {
        const Item *it = &state->items[i];
        if (it->type != ITEM_NONE && it->cell.x == c.x && it->cell.y == c.y) {
            return 1;
        }
    }
    return 0;
}

void game_items_reset(GameState *state)
{
    if (state == NULL) {
        return;
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        state->items[i].type = ITEM_NONE;
        state->items[i].cell = (Cell){ 0, 0 };
        state->items[i].remaining_sec = 0.0f;
    }

    state->effects.speed_sec = 0.0f;
    state->effects.slow_sec = 0.0f;
    state->effects.shield_sec = 0.0f;

    state->last_pickup_event = ITEM_NONE;

    state->item_spawn_timer = random_spawn_delay(state);
}

void game_items_update(GameState *state, float dt)
{
    if (state == NULL || dt <= 0.0f) {
        return;
    }

    /* 1) 道具存活倒计时与过期清除 */
    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *it = &state->items[i];
        if (it->type == ITEM_NONE) {
            continue;
        }
        it->remaining_sec -= dt;
        if (it->remaining_sec <= 0.0f) {
            it->type = ITEM_NONE;
            it->remaining_sec = 0.0f;
        }
    }

    /* 2) 刷新计时：到点后若未满则尝试生成一个道具 */
    state->item_spawn_timer -= dt;
    if (state->item_spawn_timer <= 0.0f) {
        state->item_spawn_timer = random_spawn_delay(state);
        if (item_count(state) < MAX_ITEMS) {
            spawn_item(state);
        }
    }
}

void game_effects_update(GameState *state, float dt)
{
    if (state == NULL || dt <= 0.0f) {
        return;
    }

    if (state->effects.speed_sec > 0.0f) {
        state->effects.speed_sec -= dt;
        if (state->effects.speed_sec < 0.0f) {
            state->effects.speed_sec = 0.0f;
        }
    }
    if (state->effects.slow_sec > 0.0f) {
        state->effects.slow_sec -= dt;
        if (state->effects.slow_sec < 0.0f) {
            state->effects.slow_sec = 0.0f;
        }
    }
    if (state->effects.shield_sec > 0.0f) {
        state->effects.shield_sec -= dt;
        if (state->effects.shield_sec < 0.0f) {
            state->effects.shield_sec = 0.0f;
        }
    }
}

ItemType game_items_pickup_at(GameState *state, Cell head)
{
    if (state == NULL) {
        return ITEM_NONE;
    }

    for (int i = 0; i < MAX_ITEMS; ++i) {
        Item *it = &state->items[i];
        if (it->type == ITEM_NONE) {
            continue;
        }

        /* 视觉覆盖即拾取：蛇头 2x2 方块会覆盖周围 3x3 格，
         * 因此只要道具落在该邻域内就吃掉（而不是必须精确同格）。 */
        int dx = it->cell.x - head.x;
        int dy = it->cell.y - head.y;
        if (dx < -PICKUP_RADIUS || dx > PICKUP_RADIUS ||
            dy < -PICKUP_RADIUS || dy > PICKUP_RADIUS) {
            continue;
        }

        ItemType picked = it->type;
        /* 先移除地图上的道具，再应用效果 */
        it->type = ITEM_NONE;
        it->remaining_sec = 0.0f;
        apply_item_effect(state, picked);
        state->last_pickup_event = picked;   /* 供 UI 播报拾取音效 */
        return picked;
    }
    return ITEM_NONE;
}

int game_effective_speed_ms(const GameState *state)
{
    if (state == NULL) {
        return SPEED_BASE_MS;
    }

    float ms = (float)state->speed_ms;
    if (state->effects.speed_sec > 0.0f) {
        ms *= ITEM_SPEED_FACTOR;   /* 加速：毫秒变小 -> 更快 */
    }
    if (state->effects.slow_sec > 0.0f) {
        ms *= ITEM_SLOW_FACTOR;    /* 减速：毫秒变大 -> 更慢 */
    }

    if (ms < (float)EFFECT_SPEED_MIN_MS) {
        ms = (float)EFFECT_SPEED_MIN_MS;
    } else if (ms > (float)EFFECT_SPEED_MAX_MS) {
        ms = (float)EFFECT_SPEED_MAX_MS;
    }
    return (int)(ms + 0.5f);
}
