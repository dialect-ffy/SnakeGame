// ============================================================================
// P3 单元测试：游戏状态与移动规则
// ----------------------------------------------------------------------------
// 覆盖点：
//   1. 初始化/重置的初始状态；
//   2. 方向输入（允许反向、输入缓冲、同向无效、非进行中拒绝）；
//   3. 步进移动（头插尾删、应用待生效方向）；
//   4. 吃食物成长、分数、等级与速度成长、速度下限；
//   5. 撞墙 / 撞障碍 的死亡判定；自撞不死亡（可覆盖自己）；
//   6. 障碍添加校验与食物生成避让；
//   7. 最高分更新与跨局保留。
//
// 注意：gtest 的 EXPECT_*/ASSERT_* 是宏，参数中的裸逗号会被当作参数分隔符，
//       因此统一用辅助函数 C(x, y) 构造 Cell（函数调用的括号可保护逗号）。
// ============================================================================

#include <gtest/gtest.h>

#include <vector>

extern "C" {
#include "core/game.h"
}

namespace {

// 构造一个格子（用于宏参数，避免逗号被宏拆分）
Cell C(int x, int y)
{
    Cell c{x, y};
    return c;
}

// 用给定格子重建蛇身：cells[0] 为蛇头，最后一个为蛇尾
void BuildSnake(GameState *s, const std::vector<Cell> &cells)
{
    snake_list_clear(s->snake);
    for (int i = (int)cells.size() - 1; i >= 0; --i) {
        snake_list_push_front(s->snake, cells[i].x, cells[i].y);
    }
}

// 把食物放到蛇头正前方一格（按当前方向），便于构造“必定吃到”的场景
void PlaceFoodAhead(GameState *s)
{
    Cell h = game_head_cell(s);
    switch (s->dir) {
        case DIR_UP:    s->food = C(h.x, h.y - 1); break;
        case DIR_DOWN:  s->food = C(h.x, h.y + 1); break;
        case DIR_LEFT:  s->food = C(h.x - 1, h.y); break;
        case DIR_RIGHT: s->food = C(h.x + 1, h.y); break;
        default:        s->food = C(h.x + 1, h.y); break;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. 初始化与重置
// ---------------------------------------------------------------------------

// 初始状态：长度 3、朝右、进行中、分数 0、等级 1、速度 150、食物在界内且不在蛇身
TEST(GameTest, CreateYieldsInitialState)
{
    GameState *s = game_create(12345u);
    ASSERT_NE(s, nullptr);

    EXPECT_EQ(game_snake_length(s), INITIAL_SNAKE_LENGTH);
    EXPECT_EQ(s->dir, DIR_RIGHT);
    EXPECT_EQ(s->phase, PHASE_PLAYING);
    EXPECT_EQ(s->score, 0);
    EXPECT_EQ(s->level, 1);
    EXPECT_EQ(s->eaten, 0);
    EXPECT_EQ(s->speed_ms, SPEED_BASE_MS);
    EXPECT_EQ(s->obstacle_count, 0);
    EXPECT_NE(game_is_inside_board(s->food), 0);
    EXPECT_EQ(game_is_cell_on_snake(s, s->food), 0);

    game_destroy(s);
}

// 重置后应回到初始状态（蛇长、方向、分数等）
TEST(GameTest, ResetRestoresInitialState)
{
    GameState *s = game_create(1u);
    ASSERT_NE(s, nullptr);

    // 人为破坏状态
    s->score = 999;
    s->eaten = 50;
    s->level = 9;
    s->speed_ms = 70;
    s->phase = PHASE_GAMEOVER;
    s->obstacle_count = 3;

    game_reset(s);

    EXPECT_EQ(game_snake_length(s), INITIAL_SNAKE_LENGTH);
    EXPECT_EQ(s->dir, DIR_RIGHT);
    EXPECT_EQ(s->phase, PHASE_PLAYING);
    EXPECT_EQ(s->score, 0);
    EXPECT_EQ(s->level, 1);
    EXPECT_EQ(s->eaten, 0);
    EXPECT_EQ(s->speed_ms, SPEED_BASE_MS);
    EXPECT_EQ(s->obstacle_count, 0);

    game_destroy(s);
}

// destroy(NULL) 不应崩溃
TEST(GameTest, DestroyNullIsSafe)
{
    game_destroy(nullptr);
}

// ---------------------------------------------------------------------------
// 2. 方向输入
// ---------------------------------------------------------------------------

// 允许直接反向（可覆盖自己往回走）：朝右时按左被接受并入队
TEST(GameTest, SetDirectionAllowsReverse)
{
    GameState *s = game_create(2u);
    ASSERT_NE(s, nullptr);

    EXPECT_EQ(game_set_direction(s, DIR_LEFT), 1);
    EXPECT_EQ(s->input_queue[0], DIR_LEFT);
    EXPECT_EQ(s->input_count, 1);

    game_destroy(s);
}

// 同向输入视为无效
TEST(GameTest, SetDirectionRejectsSameDirection)
{
    GameState *s = game_create(3u);
    ASSERT_NE(s, nullptr);

    EXPECT_EQ(game_set_direction(s, DIR_RIGHT), 0);
    EXPECT_EQ(s->input_count, 0);

    game_destroy(s);
}

// 垂直转向被接受并入队（当前方向尚未改变）
TEST(GameTest, SetDirectionAcceptsPerpendicularAsPending)
{
    GameState *s = game_create(4u);
    ASSERT_NE(s, nullptr);

    EXPECT_EQ(game_set_direction(s, DIR_UP), 1);
    EXPECT_EQ(s->input_queue[0], DIR_UP);
    EXPECT_EQ(s->input_count, 1);
    EXPECT_EQ(s->dir, DIR_RIGHT);   // 未步进前不生效

    game_destroy(s);
}

// 输入缓冲：一步内可缓存多次转向，队列满后拒绝
TEST(GameTest, SetDirectionBuffersMultipleTurns)
{
    GameState *s = game_create(5u);
    ASSERT_NE(s, nullptr);

    s->food = C(0, 0);

    // 朝右：先上（垂直，接受），再左（相对上垂直，接受）→ 队列满
    EXPECT_EQ(game_set_direction(s, DIR_UP), 1);
    EXPECT_EQ(game_set_direction(s, DIR_LEFT), 1);
    EXPECT_EQ(s->input_count, GAME_INPUT_BUFFER);
    // 队列已满，再按被拒
    EXPECT_EQ(game_set_direction(s, DIR_DOWN), 0);

    // 逐步消耗：第一步转为上，第二步转为左
    game_step(s);
    EXPECT_EQ(s->dir, DIR_UP);
    EXPECT_EQ(s->input_count, 1);

    s->food = C(0, 0);
    game_step(s);
    EXPECT_EQ(s->dir, DIR_LEFT);
    EXPECT_EQ(s->input_count, 0);

    game_destroy(s);
}

// 反向以“队列中最后一个方向”为基准，且允许反向
TEST(GameTest, SetDirectionAllowsReverseRelativeToQueuedDirection)
{
    GameState *s = game_create(30u);
    ASSERT_NE(s, nullptr);

    EXPECT_EQ(game_set_direction(s, DIR_UP), 1);
    EXPECT_EQ(game_set_direction(s, DIR_DOWN), 1);   // 与队列中的 UP 相反，允许
    EXPECT_EQ(s->input_count, 2);

    game_destroy(s);
}

// 非进行中阶段拒绝方向输入
TEST(GameTest, SetDirectionRejectedWhenNotPlaying)
{
    GameState *s = game_create(6u);
    ASSERT_NE(s, nullptr);

    s->phase = PHASE_PAUSED;
    EXPECT_EQ(game_set_direction(s, DIR_UP), 0);

    s->phase = PHASE_GAMEOVER;
    EXPECT_EQ(game_set_direction(s, DIR_UP), 0);

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 3. 步进移动
// ---------------------------------------------------------------------------

// 正常步进：蛇头右移一格，长度不变
TEST(GameTest, StepMovesHeadAndKeepsLength)
{
    GameState *s = game_create(7u);
    ASSERT_NE(s, nullptr);

    s->food = C(0, 0);   // 放到不挡路的地方，避免吃到
    Cell before = game_head_cell(s);

    game_step(s);

    Cell after = game_head_cell(s);
    EXPECT_EQ(after.x, before.x + 1);
    EXPECT_EQ(after.y, before.y);
    EXPECT_EQ(game_snake_length(s), INITIAL_SNAKE_LENGTH);
    EXPECT_EQ(s->phase, PHASE_PLAYING);

    game_destroy(s);
}

// 步进时应用待生效方向
TEST(GameTest, StepAppliesPendingDirection)
{
    GameState *s = game_create(8u);
    ASSERT_NE(s, nullptr);

    s->food = C(0, 0);
    ASSERT_EQ(game_set_direction(s, DIR_UP), 1);
    Cell before = game_head_cell(s);

    game_step(s);

    Cell after = game_head_cell(s);
    EXPECT_EQ(after.x, before.x);
    EXPECT_EQ(after.y, before.y - 1);
    EXPECT_EQ(s->dir, DIR_UP);
    EXPECT_EQ(s->input_count, 0);

    game_destroy(s);
}

// 无效输入（同向）被拒后，步进仍按原方向前进
TEST(GameTest, StepAfterRejectedSameDirectionKeepsDirection)
{
    GameState *s = game_create(9u);
    ASSERT_NE(s, nullptr);

    s->food = C(0, 0);
    ASSERT_EQ(game_set_direction(s, DIR_RIGHT), 0);   // 同向无效
    Cell before = game_head_cell(s);

    game_step(s);

    Cell after = game_head_cell(s);
    EXPECT_EQ(after.x, before.x + 1);
    EXPECT_EQ(after.y, before.y);

    game_destroy(s);
}

// 游戏结束后步进为空操作
TEST(GameTest, StepDoesNothingWhenGameOver)
{
    GameState *s = game_create(10u);
    ASSERT_NE(s, nullptr);

    s->phase = PHASE_GAMEOVER;
    Cell before = game_head_cell(s);
    int len = game_snake_length(s);

    game_step(s);

    Cell after = game_head_cell(s);
    EXPECT_EQ(after.x, before.x);
    EXPECT_EQ(after.y, before.y);
    EXPECT_EQ(game_snake_length(s), len);

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 4. 吃食物与难度成长
// ---------------------------------------------------------------------------

// 吃到食物：长度 +1、分数 +10、已吃 +1，并刷新出新食物
TEST(GameTest, EatFoodGrowsAndScores)
{
    GameState *s = game_create(11u);
    ASSERT_NE(s, nullptr);

    PlaceFoodAhead(s);
    Cell food_cell = s->food;

    game_step(s);

    EXPECT_EQ(game_snake_length(s), INITIAL_SNAKE_LENGTH + 1);
    EXPECT_EQ(s->score, SCORE_PER_FOOD);
    EXPECT_EQ(s->eaten, 1);
    EXPECT_EQ(game_head_cell(s).x, food_cell.x);
    EXPECT_EQ(game_head_cell(s).y, food_cell.y);
    EXPECT_EQ(game_is_cell_on_snake(s, s->food), 0);   // 新食物不与蛇重叠

    game_destroy(s);
}

// 每吃 FOODS_PER_LEVEL 个食物升 1 级，且速度按步进下降
TEST(GameTest, LevelUpEveryFoodsPerLevel)
{
    GameState *s = game_create(12u);
    ASSERT_NE(s, nullptr);

    // 直接设定已吃数量到阈值前一个，再吃一个触发升级
    s->eaten = FOODS_PER_LEVEL - 1;
    PlaceFoodAhead(s);
    game_step(s);

    EXPECT_EQ(s->eaten, FOODS_PER_LEVEL);
    EXPECT_EQ(s->level, 2);
    EXPECT_EQ(s->speed_ms, SPEED_BASE_MS - SPEED_STEP_MS);

    game_destroy(s);
}

// 速度存在下限，不会无限变快
TEST(GameTest, SpeedClampsAtMinimum)
{
    GameState *s = game_create(13u);
    ASSERT_NE(s, nullptr);

    s->eaten = 500;   // 远超需要
    PlaceFoodAhead(s);
    game_step(s);

    EXPECT_EQ(s->speed_ms, SPEED_MIN_MS);
    EXPECT_GE(s->level, 2);

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 5. 碰撞死亡
// ---------------------------------------------------------------------------

// 撞右墙死亡
TEST(GameTest, WallCollisionEndsGame)
{
    GameState *s = game_create(14u);
    ASSERT_NE(s, nullptr);

    s->food = C(0, 0);   // 不在路径上，避免中途变长

    int guard = 0;
    while (s->phase == PHASE_PLAYING && guard++ < 1000) {
        game_step(s);
    }

    EXPECT_EQ(s->phase, PHASE_GAMEOVER);
    game_destroy(s);
}

// 撞到自己身体不再死亡：允许覆盖自己
TEST(GameTest, SelfOverlapDoesNotKill)
{
    GameState *s = game_create(15u);
    ASSERT_NE(s, nullptr);

    // 蛇头 (5,5)，身体 (4,5)(4,6)(5,6)，蛇尾 (6,6)；向下会覆盖 (5,6)
    BuildSnake(s, {C(5, 5), C(4, 5), C(4, 6), C(5, 6), C(6, 6)});
    s->dir = DIR_DOWN;
    s->input_count = 0;
    s->food = C(20, 20);

    game_step(s);

    EXPECT_EQ(s->phase, PHASE_PLAYING);
    EXPECT_EQ(game_head_cell(s).x, 5);
    EXPECT_EQ(game_head_cell(s).y, 6);

    game_destroy(s);
}

// 进入当前蛇尾格合法（不吃食物时蛇尾会让出）
TEST(GameTest, MovingIntoTailCellIsAllowed)
{
    GameState *s = game_create(16u);
    ASSERT_NE(s, nullptr);

    // 蛇头 (5,5)，蛇尾 (5,6)；向下正好进入蛇尾格
    BuildSnake(s, {C(5, 5), C(4, 5), C(4, 6), C(5, 6)});
    s->dir = DIR_DOWN;
    s->input_count = 0;
    s->food = C(20, 20);

    game_step(s);

    EXPECT_EQ(s->phase, PHASE_PLAYING);
    EXPECT_EQ(game_snake_length(s), 4);
    EXPECT_EQ(game_head_cell(s).x, 5);
    EXPECT_EQ(game_head_cell(s).y, 6);

    game_destroy(s);
}

// 撞障碍死亡
TEST(GameTest, ObstacleCollisionEndsGame)
{
    GameState *s = game_create(17u);
    ASSERT_NE(s, nullptr);

    Cell h = game_head_cell(s);
    ASSERT_EQ(game_add_obstacle(s, C(h.x + 1, h.y)), 1);
    s->food = C(0, 0);

    game_step(s);

    EXPECT_EQ(s->phase, PHASE_GAMEOVER);
    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 6. 障碍添加校验
// ---------------------------------------------------------------------------

// 障碍添加的合法性与各种拒绝条件
TEST(GameTest, AddObstacleValidation)
{
    GameState *s = game_create(18u);
    ASSERT_NE(s, nullptr);

    // 合法
    EXPECT_EQ(game_add_obstacle(s, C(0, 0)), 1);
    EXPECT_NE(game_is_obstacle(s, C(0, 0)), 0);

    // 重复
    EXPECT_EQ(game_add_obstacle(s, C(0, 0)), 0);
    // 越界
    EXPECT_EQ(game_add_obstacle(s, C(-1, 0)), 0);
    EXPECT_EQ(game_add_obstacle(s, C(GRID_COLS, 0)), 0);
    // 压在蛇身上
    Cell head = game_head_cell(s);
    EXPECT_EQ(game_add_obstacle(s, head), 0);
    // 压在食物上
    EXPECT_EQ(game_add_obstacle(s, s->food), 0);

    // 填满到上限后再加应被拒绝
    int added = 1;   // 已加 (0,0)
    for (int y = 1; y < GRID_ROWS && added < MAX_OBSTACLES; ++y) {
        for (int x = 0; x < GRID_COLS && added < MAX_OBSTACLES; ++x) {
            Cell c = C(x, y);
            if (game_add_obstacle(s, c)) {
                ++added;
            }
        }
    }
    EXPECT_EQ(s->obstacle_count, MAX_OBSTACLES);
    EXPECT_EQ(game_add_obstacle(s, C(GRID_COLS - 1, GRID_ROWS - 1)), 0);

    game_destroy(s);
}

// 食物生成不会落在蛇身或障碍上
TEST(GameTest, FoodSpawnAvoidsSnakeAndObstacles)
{
    GameState *s = game_create(19u);
    ASSERT_NE(s, nullptr);

    ASSERT_EQ(game_add_obstacle(s, C(0, 0)), 1);
    ASSERT_EQ(game_add_obstacle(s, C(1, 0)), 1);

    for (int i = 0; i < 100; ++i) {
        game_spawn_food(s);
        EXPECT_NE(game_is_inside_board(s->food), 0);
        EXPECT_EQ(game_is_cell_on_snake(s, s->food), 0);
        EXPECT_EQ(game_is_obstacle(s, s->food), 0);
    }

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 7. 最高分
// ---------------------------------------------------------------------------

// 死亡时更新最高分；重置后最高分仍保留
TEST(GameTest, HighScoreUpdatesOnDeathAndSurvivesReset)
{
    GameState *s = game_create(20u);
    ASSERT_NE(s, nullptr);

    // 造一个较大的分数，再走到墙上触发死亡结算
    s->score = 250;
    s->food = C(0, 0);   // 不在路径上，避免中途吃到食物改变分数

    int guard = 0;
    while (s->phase == PHASE_PLAYING && guard++ < 1000) {
        game_step(s);
    }
    ASSERT_EQ(s->phase, PHASE_GAMEOVER);
    EXPECT_EQ(s->high_score, 250);

    game_reset(s);
    EXPECT_EQ(s->high_score, 250);   // 跨局保留

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 8. 边界与访问器
// ---------------------------------------------------------------------------

// 棋盘范围判定
TEST(GameTest, IsInsideBoardBounds)
{
    EXPECT_NE(game_is_inside_board(C(0, 0)), 0);
    EXPECT_NE(game_is_inside_board(C(GRID_COLS - 1, GRID_ROWS - 1)), 0);
    EXPECT_EQ(game_is_inside_board(C(-1, 0)), 0);
    EXPECT_EQ(game_is_inside_board(C(0, -1)), 0);
    EXPECT_EQ(game_is_inside_board(C(GRID_COLS, 0)), 0);
    EXPECT_EQ(game_is_inside_board(C(0, GRID_ROWS)), 0);
}

// 访问器：初始蛇头位于中央
TEST(GameTest, HeadCellAtBoardCenter)
{
    GameState *s = game_create(21u);
    ASSERT_NE(s, nullptr);

    Cell h = game_head_cell(s);
    EXPECT_EQ(h.x, GRID_COLS / 2);
    EXPECT_EQ(h.y, GRID_ROWS / 2);

    game_destroy(s);
}
