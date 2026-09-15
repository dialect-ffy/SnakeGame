// ============================================================================
// P5 单元测试：道具系统与效果
// ----------------------------------------------------------------------------
// 覆盖点：
//   1. 重置后道具/效果状态干净，刷新计时在合理范围；
//   2. 定时刷新、存活倒计时与过期清除、数量上限；
//   3. 生成位置避让（蛇身/食物/障碍/其他道具/出生区）；
//   4. 拾取四种道具的效果：加速、减速、护盾、缩短（含最短长度保护）；
//   5. 同类效果“刷新时长”而非叠加；
//   6. 效果计时推进与“有效速度”计算（含限幅）；
//   7. 护盾免死一次并在触发后消耗；
//   8. 难度成长时障碍按等级补充、上限封顶、避让出生区。
// ============================================================================

#include <gtest/gtest.h>

#include <vector>

extern "C" {
#include "core/game.h"
}

namespace {

// 构造格子（避免在宏参数中出现裸逗号）
Cell C(int x, int y)
{
    Cell c{x, y};
    return c;
}

// 用给定格子重建蛇身：cells[0] 为蛇头
void BuildSnake(GameState *s, const std::vector<Cell> &cells)
{
    snake_list_clear(s->snake);
    for (int i = (int)cells.size() - 1; i >= 0; --i) {
        snake_list_push_front(s->snake, cells[i].x, cells[i].y);
    }
}

// 构造一条水平直蛇：蛇头在 (head_x, head_y)，朝右，长度 len
void BuildStraightSnake(GameState *s, int head_x, int head_y, int len)
{
    std::vector<Cell> cells;
    for (int i = 0; i < len; ++i) {
        cells.push_back(C(head_x - i, head_y));
    }
    BuildSnake(s, cells);
    s->dir = DIR_RIGHT;
    s->input_count = 0;
}

// 蛇头正前方一格
Cell CellAhead(const GameState *s)
{
    Cell h = game_head_cell(s);
    switch (s->dir) {
        case DIR_UP:    return C(h.x, h.y - 1);
        case DIR_DOWN:  return C(h.x, h.y + 1);
        case DIR_LEFT:  return C(h.x - 1, h.y);
        default:        return C(h.x + 1, h.y);
    }
}

// 在 0 号槽位放置一个道具到蛇头正前方
void PlaceItemAhead(GameState *s, ItemType type)
{
    s->items[0].type = type;
    s->items[0].cell = CellAhead(s);
    s->items[0].remaining_sec = ITEM_LIFETIME_SEC;
}

// 统计地图上的道具数量
int CountItems(const GameState *s)
{
    int n = 0;
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (s->items[i].type != ITEM_NONE) {
            ++n;
        }
    }
    return n;
}

// 强制触发一次道具刷新（把刷新计时清零后推进），随后把计时推到很远的未来，
// 以免后续 update 再次刷新干扰断言。
int ForceSpawnItem(GameState *s)
{
    s->item_spawn_timer = 0.0f;
    game_items_update(s, 0.001f);
    s->item_spawn_timer = 1000.0f;
    return CountItems(s);
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. 重置
// ---------------------------------------------------------------------------

// 初始（重置后）道具为空、效果归零、刷新计时在 [MIN, MAX] 之间
TEST(ItemsTest, ResetClearsItemsAndEffects)
{
    GameState *s = game_create(101u);
    ASSERT_NE(s, nullptr);

    EXPECT_EQ(CountItems(s), 0);
    EXPECT_FLOAT_EQ(s->effects.speed_sec, 0.0f);
    EXPECT_FLOAT_EQ(s->effects.slow_sec, 0.0f);
    EXPECT_FLOAT_EQ(s->effects.shield_sec, 0.0f);
    EXPECT_GE(s->item_spawn_timer, ITEM_SPAWN_MIN_SEC);
    EXPECT_LE(s->item_spawn_timer, ITEM_SPAWN_MAX_SEC);

    // 人为放置后再重置应清空
    s->items[0].type = ITEM_SPEED;
    s->items[0].remaining_sec = 3.0f;
    s->effects.speed_sec = 4.0f;
    game_items_reset(s);

    EXPECT_EQ(CountItems(s), 0);
    EXPECT_FLOAT_EQ(s->effects.speed_sec, 0.0f);

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 2. 刷新与过期
// ---------------------------------------------------------------------------

// 刷新计时到点后应生成一个道具，存活时间为 ITEM_LIFETIME_SEC
TEST(ItemsTest, SpawnsItemWhenTimerElapses)
{
    GameState *s = game_create(102u);
    ASSERT_NE(s, nullptr);

    s->item_spawn_timer = 0.0f;
    game_items_update(s, 0.001f);

    ASSERT_EQ(CountItems(s), 1);
    for (int i = 0; i < MAX_ITEMS; ++i) {
        if (s->items[i].type != ITEM_NONE) {
            EXPECT_FLOAT_EQ(s->items[i].remaining_sec, ITEM_LIFETIME_SEC);
        }
    }

    game_destroy(s);
}

// 道具超过存活时间后应被清除
TEST(ItemsTest, ItemExpiresAfterLifetime)
{
    GameState *s = game_create(103u);
    ASSERT_NE(s, nullptr);

    ASSERT_EQ(ForceSpawnItem(s), 1);

    game_items_update(s, ITEM_LIFETIME_SEC + 0.5f);

    EXPECT_EQ(CountItems(s), 0);

    game_destroy(s);
}

// 道具数量不超过 MAX_ITEMS
TEST(ItemsTest, ItemCountNeverExceedsMax)
{
    GameState *s = game_create(104u);
    ASSERT_NE(s, nullptr);

    for (int i = 0; i < 10; ++i) {
        ForceSpawnItem(s);
        EXPECT_LE(CountItems(s), MAX_ITEMS);
    }
    EXPECT_EQ(CountItems(s), MAX_ITEMS);

    game_destroy(s);
}

// 生成的道具不与蛇身/食物/障碍重叠，也不在出生保护区内
TEST(ItemsTest, SpawnedItemAvoidsOccupiedCellsAndBirthZone)
{
    GameState *s = game_create(105u);
    ASSERT_NE(s, nullptr);

    ASSERT_EQ(game_add_obstacle(s, C(0, 0)), 1);

    for (int i = 0; i < 50; ++i) {
        ForceSpawnItem(s);
        for (int k = 0; k < MAX_ITEMS; ++k) {
            if (s->items[k].type == ITEM_NONE) {
                continue;
            }
            Cell c = s->items[k].cell;
            EXPECT_NE(game_is_inside_board(c), 0);
            EXPECT_EQ(game_is_cell_on_snake(s, c), 0);
            EXPECT_EQ(game_is_obstacle(s, c), 0);
            EXPECT_FALSE(c.x == s->food.x && c.y == s->food.y);
            EXPECT_EQ(game_is_in_birth_zone(c), 0);
        }
    }

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 3. 拾取效果
// ---------------------------------------------------------------------------

// 加速道具：设置 5 秒加速效果并移除地图道具
TEST(ItemsTest, PickupSpeedSetsEffect)
{
    GameState *s = game_create(106u);
    ASSERT_NE(s, nullptr);

    PlaceItemAhead(s, ITEM_SPEED);
    game_step(s);

    EXPECT_FLOAT_EQ(s->effects.speed_sec, EFFECT_DURATION_SEC);
    EXPECT_EQ(CountItems(s), 0);

    game_destroy(s);
}

// 减速道具
TEST(ItemsTest, PickupSlowSetsEffect)
{
    GameState *s = game_create(107u);
    ASSERT_NE(s, nullptr);

    PlaceItemAhead(s, ITEM_SLOW);
    game_step(s);

    EXPECT_FLOAT_EQ(s->effects.slow_sec, EFFECT_DURATION_SEC);
    EXPECT_EQ(CountItems(s), 0);

    game_destroy(s);
}

// 护盾道具
TEST(ItemsTest, PickupShieldSetsEffect)
{
    GameState *s = game_create(108u);
    ASSERT_NE(s, nullptr);

    PlaceItemAhead(s, ITEM_SHIELD);
    game_step(s);

    EXPECT_FLOAT_EQ(s->effects.shield_sec, EFFECT_DURATION_SEC);
    EXPECT_EQ(CountItems(s), 0);

    game_destroy(s);
}

// 缩短道具：长度 8 -> 5
TEST(ItemsTest, PickupShrinkRemovesThree)
{
    GameState *s = game_create(109u);
    ASSERT_NE(s, nullptr);

    BuildStraightSnake(s, 20, 12, 8);
    s->food = C(0, 0);
    PlaceItemAhead(s, ITEM_SHRINK);

    game_step(s);

    EXPECT_EQ(game_snake_length(s), 5);

    game_destroy(s);
}

// 缩短道具受最小长度保护：长度 3 时不再缩短
TEST(ItemsTest, ShrinkRespectsMinimumLength)
{
    GameState *s = game_create(110u);
    ASSERT_NE(s, nullptr);

    BuildStraightSnake(s, 20, 12, 3);
    s->food = C(0, 0);
    PlaceItemAhead(s, ITEM_SHRINK);

    game_step(s);

    EXPECT_EQ(game_snake_length(s), MIN_SNAKE_LENGTH);

    game_destroy(s);
}

// 缩短道具只删到下限：长度 4 -> 3（可删 1 节）
TEST(ItemsTest, ShrinkRemovesOnlyDownToMinimum)
{
    GameState *s = game_create(111u);
    ASSERT_NE(s, nullptr);

    BuildStraightSnake(s, 20, 12, 4);
    s->food = C(0, 0);
    PlaceItemAhead(s, ITEM_SHRINK);

    game_step(s);

    EXPECT_EQ(game_snake_length(s), MIN_SNAKE_LENGTH);

    game_destroy(s);
}

// 同类效果刷新时长而非叠加
TEST(ItemsTest, SameEffectRefreshesDurationNotStack)
{
    GameState *s = game_create(112u);
    ASSERT_NE(s, nullptr);

    s->effects.speed_sec = 2.0f;   // 已有一半时间
    PlaceItemAhead(s, ITEM_SPEED);
    game_step(s);

    EXPECT_FLOAT_EQ(s->effects.speed_sec, EFFECT_DURATION_SEC);   // 刷新回满，而非 7

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 4. 效果计时与有效速度
// ---------------------------------------------------------------------------

// 效果计时随 dt 递减，归零后不再为负
TEST(ItemsTest, EffectsTickDownAndClampAtZero)
{
    GameState *s = game_create(113u);
    ASSERT_NE(s, nullptr);

    s->effects.speed_sec = 1.0f;
    s->effects.shield_sec = 0.5f;

    game_effects_update(s, 0.4f);
    EXPECT_NEAR(s->effects.speed_sec, 0.6f, 1e-4);
    EXPECT_NEAR(s->effects.shield_sec, 0.1f, 1e-4);

    game_effects_update(s, 1.0f);
    EXPECT_FLOAT_EQ(s->effects.speed_sec, 0.0f);
    EXPECT_FLOAT_EQ(s->effects.shield_sec, 0.0f);

    game_destroy(s);
}

// 加速让有效步进毫秒变小
TEST(ItemsTest, EffectiveSpeedSpeedUp)
{
    GameState *s = game_create(114u);
    ASSERT_NE(s, nullptr);

    s->speed_ms = 150;
    s->effects.speed_sec = 1.0f;

    int ms = game_effective_speed_ms(s);
    EXPECT_EQ(ms, (int)(150 * ITEM_SPEED_FACTOR + 0.5f));   // 90

    game_destroy(s);
}

// 减速让有效步进毫秒变大
TEST(ItemsTest, EffectiveSpeedSlowDown)
{
    GameState *s = game_create(115u);
    ASSERT_NE(s, nullptr);

    s->speed_ms = 75;
    s->effects.slow_sec = 1.0f;

    int ms = game_effective_speed_ms(s);
    EXPECT_EQ(ms, (int)(75 * ITEM_SLOW_FACTOR + 0.5f));    // 120

    game_destroy(s);
}

// 加速与减速同时生效时基本抵消
TEST(ItemsTest, EffectiveSpeedSpeedAndSlowCancel)
{
    GameState *s = game_create(116u);
    ASSERT_NE(s, nullptr);

    s->speed_ms = 150;
    s->effects.speed_sec = 1.0f;
    s->effects.slow_sec = 1.0f;

    int ms = game_effective_speed_ms(s);
    EXPECT_EQ(ms, (int)(150 * ITEM_SPEED_FACTOR * ITEM_SLOW_FACTOR + 0.5f));   // 144

    game_destroy(s);
}

// 有效速度限幅：下限
TEST(ItemsTest, EffectiveSpeedClampsToMinimum)
{
    GameState *s = game_create(117u);
    ASSERT_NE(s, nullptr);

    s->speed_ms = 10;
    s->effects.speed_sec = 1.0f;   // 10*0.6=6 -> 下限

    EXPECT_EQ(game_effective_speed_ms(s), EFFECT_SPEED_MIN_MS);

    game_destroy(s);
}

// 有效速度限幅：上限
TEST(ItemsTest, EffectiveSpeedClampsToMaximum)
{
    GameState *s = game_create(118u);
    ASSERT_NE(s, nullptr);

    s->speed_ms = 1000;
    s->effects.slow_sec = 1.0f;    // 1600 -> 上限 300

    EXPECT_EQ(game_effective_speed_ms(s), EFFECT_SPEED_MAX_MS);

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 5. 护盾免死
// ---------------------------------------------------------------------------

// 护盾生效时撞墙免死一次，并原地不动、消耗护盾
TEST(ItemsTest, ShieldPreventsDeathOnce)
{
    GameState *s = game_create(119u);
    ASSERT_NE(s, nullptr);

    // 蛇头贴右墙，朝右
    BuildStraightSnake(s, GRID_COLS - 1, 5, 3);
    s->food = C(0, 0);
    s->effects.shield_sec = EFFECT_DURATION_SEC;

    Cell before = game_head_cell(s);
    game_step(s);

    EXPECT_EQ(s->phase, PHASE_PLAYING);
    EXPECT_FLOAT_EQ(s->effects.shield_sec, 0.0f);   // 护盾被消耗
    EXPECT_EQ(game_head_cell(s).x, before.x);       // 原地不动
    EXPECT_EQ(game_head_cell(s).y, before.y);

    game_destroy(s);
}

// 护盾消耗后再次撞墙则死亡
TEST(ItemsTest, SecondCollisionAfterShieldKills)
{
    GameState *s = game_create(120u);
    ASSERT_NE(s, nullptr);

    BuildStraightSnake(s, GRID_COLS - 1, 5, 3);
    s->food = C(0, 0);
    s->effects.shield_sec = EFFECT_DURATION_SEC;

    game_step(s);   // 第一次：护盾免死
    ASSERT_EQ(s->phase, PHASE_PLAYING);

    game_step(s);   // 第二次：死亡
    EXPECT_EQ(s->phase, PHASE_GAMEOVER);

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 6. 障碍随等级增长
// ---------------------------------------------------------------------------

// 升到 4 级时按规则增加 1 个障碍
TEST(ItemsTest, ObstaclesGrowWithLevel)
{
    GameState *s = game_create(121u);
    ASSERT_NE(s, nullptr);

    ASSERT_EQ(s->obstacle_count, 0);

    // 直接设定到阈值前一个食物，再吃一个升到 4 级
    s->eaten = FOODS_PER_LEVEL * 3 - 1;   // 23 -> 吃后 24 -> 等级 4
    Cell h = game_head_cell(s);
    s->food = C(h.x + 1, h.y);

    game_step(s);

    EXPECT_EQ(s->level, 4);
    EXPECT_EQ(s->obstacle_count, 1);

    game_destroy(s);
}

// 障碍数量封顶于 MAX_OBSTACLES
TEST(ItemsTest, ObstacleGrowthCapsAtMax)
{
    GameState *s = game_create(122u);
    ASSERT_NE(s, nullptr);

    s->eaten = 400;   // 高等级
    Cell h = game_head_cell(s);
    s->food = C(h.x + 1, h.y);

    game_step(s);

    EXPECT_EQ(s->obstacle_count, MAX_OBSTACLES);

    game_destroy(s);
}

// 自动生成的障碍都不在出生保护区，也不与蛇/食物/道具重叠
TEST(ItemsTest, SpawnedObstaclesAvoidBirthZoneAndOccupied)
{
    GameState *s = game_create(123u);
    ASSERT_NE(s, nullptr);

    s->eaten = 400;
    Cell h = game_head_cell(s);
    s->food = C(h.x + 1, h.y);
    game_step(s);

    ASSERT_EQ(s->obstacle_count, MAX_OBSTACLES);
    for (int i = 0; i < s->obstacle_count; ++i) {
        EXPECT_EQ(game_is_in_birth_zone(s->obstacles[i]), 0);
    }

    game_destroy(s);
}

// ---------------------------------------------------------------------------
// 7. 视觉覆盖即拾取（蛇头 2x2 方块覆盖到即可）
// ---------------------------------------------------------------------------

// 斜向相邻（对角）的道具也能吃到
TEST(ItemsTest, PickupWithVisualOverlapDiagonal)
{
    GameState *s = game_create(200u);
    ASSERT_NE(s, nullptr);

    Cell h = game_head_cell(s);   // (GRID_COLS/2, GRID_ROWS/2)，朝右
    s->items[0].type = ITEM_SPEED;
    s->items[0].cell = C(h.x + 1, h.y + 1);   // 蛇头下一步的斜下方
    s->items[0].remaining_sec = ITEM_LIFETIME_SEC;
    s->food = C(0, 0);

    game_step(s);   // 蛇头到 (h.x+1, h.y)，道具在其下方一格，视觉方块覆盖

    EXPECT_FLOAT_EQ(s->effects.speed_sec, EFFECT_DURATION_SEC);
    EXPECT_EQ(CountItems(s), 0);

    game_destroy(s);
}

// 超出拾取邻域（2 格以上）的道具不会被吃
TEST(ItemsTest, PickupBeyondRadiusIsIgnored)
{
    GameState *s = game_create(201u);
    ASSERT_NE(s, nullptr);

    Cell h = game_head_cell(s);
    s->items[0].type = ITEM_SHIELD;
    s->items[0].cell = C(h.x + 3, h.y);   // 前方 3 格
    s->items[0].remaining_sec = ITEM_LIFETIME_SEC;
    s->food = C(0, 0);

    game_step(s);   // 蛇头到 h.x+1，道具在 h.x+3 -> 距离 2 格，不拾取

    EXPECT_FLOAT_EQ(s->effects.shield_sec, 0.0f);
    EXPECT_EQ(CountItems(s), 1);

    game_destroy(s);
}

// 食物同样遵循“视觉覆盖即拾取”：斜向相邻也能吃到并成长
TEST(ItemsTest, FoodPickupWithVisualOverlap)
{
    GameState *s = game_create(202u);
    ASSERT_NE(s, nullptr);

    BuildStraightSnake(s, 20, 12, 6);
    s->food = C(21, 13);   // 蛇头下一步 (21,12) 的下方一格

    game_step(s);

    EXPECT_EQ(s->score, SCORE_PER_FOOD);
    EXPECT_EQ(game_snake_length(s), 7);   // 6 + 1

    game_destroy(s);
}
