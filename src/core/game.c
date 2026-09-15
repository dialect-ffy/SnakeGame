/* ============================================================================
 * SnakeGame 核心逻辑层 —— 游戏状态与规则实现
 * ----------------------------------------------------------------------------
 * 本文件为纯 C 逻辑，禁止包含 raylib 头文件。
 * 关键概念：
 *   - 蛇身用带哨兵的双向循环链表表示，蛇头 = sentinel->next，蛇尾 = sentinel->prev；
 *   - 每步“头插新蛇头 + 删蛇尾”，吃到食物时只头插不删尾，长度自然 +1。
 * ==========================================================================*/

#include "core/game.h"

#include <stdlib.h>

/* 前向声明：难度成长时需要动态补充障碍（定义在文件后半部分） */
static void update_obstacles(GameState *state);

/* ---------------------------------------------------------------------------
 * 内部辅助函数
 * -------------------------------------------------------------------------*/

/** @brief 判断两个格子是否相同。 */
static int cell_equal(Cell a, Cell b)
{
    return (a.x == b.x) && (a.y == b.y);
}

/**
 * @brief 判断格子 b 是否落在以 a 为中心的拾取邻域内（视觉覆盖即拾取）。
 *
 * 蛇头在视觉上是 2x2 的方块，会覆盖到周围 3x3 格，因此食物/道具只要落在
 * 该邻域内就应被吃掉，避免“看起来碰到了却吃不到”。
 */
static int cell_within_pickup(Cell a, Cell b)
{
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return (dx <= PICKUP_RADIUS) && (dy <= PICKUP_RADIUS);
}

/** @brief 返回某方向对应的单位位移。 */
static Cell dir_delta(Direction dir)
{
    switch (dir) {
        case DIR_UP:    return (Cell){  0, -1 };
        case DIR_DOWN:  return (Cell){  0,  1 };
        case DIR_LEFT:  return (Cell){ -1,  0 };
        case DIR_RIGHT: return (Cell){  1,  0 };
        default:        return (Cell){  0,  0 };
    }
}

/**
 * @brief 结束游戏：切换阶段并更新最高分。
 */
static void end_game(GameState *state)
{
    state->phase = PHASE_GAMEOVER;
    if (state->score > state->high_score) {
        state->high_score = state->score;
    }
}

/**
 * @brief 根据已吃食物数量刷新等级与速度。
 *
 * 规则：每 FOODS_PER_LEVEL 个食物升 1 级；速度 = 基础速度 - (等级-1)*步进，且不低于下限。
 */
static void update_level_and_speed(GameState *state)
{
    int level = 1 + state->eaten / FOODS_PER_LEVEL;
    state->level = level;

    int speed = SPEED_BASE_MS - (level - 1) * SPEED_STEP_MS;
    if (speed < SPEED_MIN_MS) {
        speed = SPEED_MIN_MS;
    }
    state->speed_ms = speed;

    /* 等级提升后按规则补足障碍 */
    update_obstacles(state);
}

/**
 * @brief 判断某格子是否可作为新食物位置（在界内、非蛇身、非障碍、非当前食物）。
 */
static int cell_free_for_food(const GameState *state, Cell c)
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
    if (cell_equal(c, state->food)) {
        return 0;   /* 避免与现有食物重叠 */
    }
    return 1;
}

/**
 * @brief 判断格子是否落在出生保护区内。
 *
 * 保护区以初始蛇所在行为中心向四周扩展若干格；仅用于“自动生成”道具/障碍时避让，
 * 不影响显式调用 game_add_obstacle 的合法格子。
 */
int game_is_in_birth_zone(Cell c)
{
    int cx = GRID_COLS / 2;
    int cy = GRID_ROWS / 2;
    return (c.y >= cy - 2) && (c.y <= cy + 2) &&
           (c.x >= cx - 4) && (c.x <= cx + 2);
}

int game_find_free_cell(GameState *state, GameCellPredicate pred, Cell *out)
{
    if (state == NULL || pred == NULL || out == NULL) {
        return 0;
    }

    /* 先随机尝试，分布更自然 */
    const int MAX_TRIES = 200;
    for (int i = 0; i < MAX_TRIES; ++i) {
        Cell c = {
            rng_range(&state->rng, 0, GRID_COLS - 1),
            rng_range(&state->rng, 0, GRID_ROWS - 1)
        };
        if (pred(state, c)) {
            *out = c;
            return 1;
        }
    }

    /* 退化路径：顺序扫描，保证存在空位时一定能找到 */
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Cell c = { x, y };
            if (pred(state, c)) {
                *out = c;
                return 1;
            }
        }
    }
    return 0;
}

/**
 * @brief 判断格子是否适合自动生成障碍（在界内、非蛇/障碍/食物/道具、非出生区）。
 */
static int cell_free_for_obstacle(GameState *state, Cell c)
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
    if (cell_equal(c, state->food)) {
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
 * @brief 随机生成一个障碍；无空位时返回 0。
 */
static int spawn_one_obstacle(GameState *state)
{
    Cell c;
    if (!game_find_free_cell(state, cell_free_for_obstacle, &c)) {
        return 0;
    }
    return game_add_obstacle(state, c);
}

/**
 * @brief 依据当前等级补足障碍数量。
 *
 * 目标数量 = (等级 - 1) / OBSTACLES_PER_LEVELS，上限 MAX_OBSTACLES。
 */
static void update_obstacles(GameState *state)
{
    int target = (state->level - 1) / OBSTACLES_PER_LEVELS;
    if (target > MAX_OBSTACLES) {
        target = MAX_OBSTACLES;
    }

    int guard = 0;
    while (state->obstacle_count < target && guard++ < MAX_OBSTACLES * 4) {
        if (!spawn_one_obstacle(state)) {
            break;   /* 无空位 */
        }
    }
}

/* ---------------------------------------------------------------------------
 * 公开接口实现
 * -------------------------------------------------------------------------*/

GameState *game_create(uint32_t seed)
{
    GameState *state = (GameState *)malloc(sizeof(GameState));
    if (state == NULL) {
        return NULL;
    }

    state->snake = snake_list_create();
    if (state->snake == NULL) {
        free(state);
        return NULL;
    }

    state->high_score = 0;   /* 最高分在会话内跨局保留，重置时不归零 */
    rng_seed(&state->rng, seed);

    game_reset(state);
    return state;
}

void game_destroy(GameState *state)
{
    if (state == NULL) {
        return;
    }
    snake_list_destroy(state->snake);
    free(state);
}

void game_reset(GameState *state)
{
    if (state == NULL) {
        return;
    }

    /* 清空蛇身，并在棋盘中央水平摆放初始蛇：头在右，身体向左延伸 */
    snake_list_clear(state->snake);
    int cx = GRID_COLS / 2;
    int cy = GRID_ROWS / 2;
    for (int i = INITIAL_SNAKE_LENGTH - 1; i >= 0; --i) {
        /* 先插入最靠左的（将成为蛇尾），最后插入蛇头 */
        snake_list_push_front(state->snake, cx - i, cy);
    }

    state->dir = DIR_RIGHT;
    state->input_count = 0;

    state->obstacle_count = 0;

    state->score = 0;
    state->level = 1;
    state->eaten = 0;
    state->speed_ms = SPEED_BASE_MS;

    state->phase = PHASE_PLAYING;

    /* 重置道具与效果 */
    game_items_reset(state);

    /* 放置一个初始食物（不覆盖蛇身） */
    state->food = (Cell){ 0, 0 };
    game_spawn_food(state);
}

int game_set_direction(GameState *state, Direction dir)
{
    if (state == NULL || state->phase != PHASE_PLAYING) {
        return 0;
    }
    if (state->input_count >= GAME_INPUT_BUFFER) {
        return 0;   /* 缓冲队列已满 */
    }

    /* 以“队列中最后一个方向”为基准判重；队列为空时基准为当前方向。
     * 本游戏允许直接反向（可覆盖自己往回走），因此不再拒绝 180 度转向。 */
    Direction last = (state->input_count > 0)
                         ? state->input_queue[state->input_count - 1]
                         : state->dir;

    if (dir == last) {
        return 0;   /* 与基准方向相同，视为无效输入，不占用队列名额 */
    }

    state->input_queue[state->input_count] = dir;
    state->input_count += 1;
    return 1;
}

void game_step(GameState *state)
{
    if (state == NULL || state->phase != PHASE_PLAYING) {
        return;
    }

    /* 从输入缓冲队列取出本步生效的转向（先进先出） */
    if (state->input_count > 0) {
        state->dir = state->input_queue[0];
        for (int i = 1; i < state->input_count; ++i) {
            state->input_queue[i - 1] = state->input_queue[i];
        }
        state->input_count -= 1;
    }

    Cell head = game_head_cell(state);
    Cell delta = dir_delta(state->dir);
    Cell next = { head.x + delta.x, head.y + delta.y };

    int growing = cell_within_pickup(next, state->food);

    /* 碰撞判定：撞墙 / 撞障碍。
     * 注意：按需求，撞到自己身体不再死亡，且允许反向覆盖自己。 */
    int hit = (!game_is_inside_board(next)) || game_is_obstacle(state, next);

    if (hit) {
        if (state->effects.shield_sec > 0.0f) {
            /* 护盾生效：消耗护盾并免死一次，本步原地不动 */
            state->effects.shield_sec = 0.0f;
            return;
        }
        end_game(state);
        return;
    }

    /* 头插新蛇头 */
    snake_list_push_front(state->snake, next.x, next.y);

    if (growing) {
        /* 吃到食物：不删尾（长度 +1），结算分数与难度，并刷新食物 */
        state->score += SCORE_PER_FOOD;
        state->eaten += 1;
        update_level_and_speed(state);
        game_spawn_food(state);
    } else {
        /* 正常移动：删尾，长度不变 */
        snake_list_pop_back(state->snake);
    }

    /* 拾取道具：若新蛇头所在格有道具则立即生效 */
    game_items_pickup_at(state, next);
}

Cell game_head_cell(const GameState *state)
{
    if (state == NULL) {
        return (Cell){ 0, 0 };
    }
    const SnakeNode *head = snake_list_head(state->snake);
    if (head == NULL) {
        return (Cell){ 0, 0 };
    }
    return (Cell){ head->x, head->y };
}

int game_snake_length(const GameState *state)
{
    if (state == NULL) {
        return 0;
    }
    return snake_list_length(state->snake);
}

int game_is_cell_on_snake(const GameState *state, Cell c)
{
    if (state == NULL) {
        return 0;
    }
    return snake_list_contains(state->snake, c.x, c.y);
}

int game_is_obstacle(const GameState *state, Cell c)
{
    if (state == NULL) {
        return 0;
    }
    for (int i = 0; i < state->obstacle_count; ++i) {
        if (cell_equal(state->obstacles[i], c)) {
            return 1;
        }
    }
    return 0;
}

int game_is_inside_board(Cell c)
{
    return (c.x >= 0) && (c.x < GRID_COLS) && (c.y >= 0) && (c.y < GRID_ROWS);
}

int game_add_obstacle(GameState *state, Cell c)
{
    if (state == NULL) {
        return 0;
    }
    if (state->obstacle_count >= MAX_OBSTACLES) {
        return 0;   /* 达到上限 */
    }
    if (!game_is_inside_board(c)) {
        return 0;   /* 越界 */
    }
    if (game_is_obstacle(state, c)) {
        return 0;   /* 重复 */
    }
    if (game_is_cell_on_snake(state, c)) {
        return 0;   /* 不允许压在蛇身上 */
    }
    if (cell_equal(c, state->food)) {
        return 0;   /* 不允许压在食物上 */
    }
    if (game_is_cell_on_item(state, c)) {
        return 0;   /* 不允许压在道具上 */
    }

    state->obstacles[state->obstacle_count] = c;
    state->obstacle_count += 1;
    return 1;
}

void game_spawn_food(GameState *state)
{
    if (state == NULL) {
        return;
    }

    /* 先随机尝试若干次，命中率高且分布自然 */
    const int MAX_TRIES = 200;
    for (int i = 0; i < MAX_TRIES; ++i) {
        Cell c = {
            rng_range(&state->rng, 0, GRID_COLS - 1),
            rng_range(&state->rng, 0, GRID_ROWS - 1)
        };
        if (cell_free_for_food(state, c)) {
            state->food = c;
            return;
        }
    }

    /* 退化路径：棋盘接近填满时，顺序扫描保证找到空位 */
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            Cell c = { x, y };
            if (cell_free_for_food(state, c)) {
                state->food = c;
                return;
            }
        }
    }
    /* 棋盘已满：无空位，保持原食物不变 */
}
