/* ============================================================================
 * SnakeGame 核心逻辑层 —— 游戏状态与规则
 * ----------------------------------------------------------------------------
 * 职责：
 *   - 维护整局游戏的状态（蛇、方向、食物、障碍、分数、等级、速度、阶段）；
 *   - 实现“尾插头删”的步进、禁止反向、碰撞判定、吃食物成长与难度成长。
 * 约束：
 *   - 本模块为纯 C，不依赖 raylib，可被 googletest 独立测试；
 *   - 所有随机行为都经由可注入种子的 Rng，保证可复现。
 * ==========================================================================*/

#ifndef SNAKE_CORE_GAME_H
#define SNAKE_CORE_GAME_H

#include <stdint.h>

#include "core/config.h"
#include "core/items.h"
#include "core/list.h"
#include "core/rng.h"
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 游戏阶段。
 */
typedef enum {
    PHASE_READY = 0,   /**< 就绪（预留） */
    PHASE_PLAYING,     /**< 进行中 */
    PHASE_PAUSED,      /**< 暂停 */
    PHASE_GAMEOVER     /**< 已结束 */
} GamePhase;

/**
 * @brief 一局游戏的完整状态。
 */
typedef struct {
    SnakeList *snake;                        /**< 蛇身：带哨兵的双向循环链表，蛇头为 sentinel->next */

    Direction dir;                           /**< 当前生效方向 */
    Direction input_queue[GAME_INPUT_BUFFER];/**< 转向输入缓冲队列（先进先出） */
    int       input_count;                   /**< 队列中待生效的转向数量 */

    Cell      food;                          /**< 当前普通食物所在格子 */

    Cell      obstacles[MAX_OBSTACLES];      /**< 障碍物格子数组 */
    int       obstacle_count;                /**< 障碍物数量 */

    Item        items[MAX_ITEMS];            /**< 地图上的道具槽位 */
    ItemEffects effects;                     /**< 当前生效的道具效果计时 */
    float       item_spawn_timer;            /**< 距下次尝试刷新道具的剩余秒数 */
    ItemType    last_pickup_event;           /**< 最近一次拾取的道具（供 UI 播报音效，读取后置 ITEM_NONE） */

    int       score;                         /**< 当前分数 */
    int       high_score;                    /**< 历史最高分 */
    int       level;                         /**< 当前等级（从 1 开始） */
    int       eaten;                         /**< 已吃普通食物总数 */
    int       speed_ms;                      /**< 当前每格步进毫秒数 */

    GamePhase phase;                         /**< 当前阶段 */
    Rng       rng;                           /**< 随机源（可注入种子） */
} GameState;

/**
 * @brief 创建并初始化一局游戏。
 * @param seed 随机种子（用于食物/障碍生成）。
 * @return 新游戏状态指针；分配失败返回 NULL。
 */
GameState *game_create(uint32_t seed);

/**
 * @brief 销毁游戏状态（含蛇链表），释放所有内存。
 * @param state 目标状态，允许为 NULL。
 */
void game_destroy(GameState *state);

/**
 * @brief 重置为初始状态（保留随机源当前状态）。
 *
 * 蛇重置到棋盘中央、长度 INITIAL_SNAKE_LENGTH、朝右；分数/等级/障碍清空；
 * 阶段置为 PHASE_PLAYING 并立即生成一个食物。
 *
 * @param state 目标状态，允许为 NULL。
 */
void game_reset(GameState *state);

/**
 * @brief 请求改变蛇的移动方向（带缓冲）。
 *
 * 规则：
 *   - 仅当阶段为 PHASE_PLAYING 时接受；
 *   - 方向会进入长度为 GAME_INPUT_BUFFER 的缓冲队列，每个逻辑步消耗一个，
 *     因此一个步进内快速连按多次转向不会丢输入；
 *   - 允许直接反向（可覆盖自己往回走）；
 *   - 与“队列中最后一个方向（队列为空时为当前方向）”相同视为无效输入，不占用队列名额；
 *   - 队列已满时拒绝。
 *
 * @param state 目标状态。
 * @param dir   请求的方向。
 * @return 1 表示接受（已入队），0 表示拒绝。
 */
int game_set_direction(GameState *state, Direction dir);

/**
 * @brief 前进一步（一个逻辑格）。
 *
 * 流程：应用待生效方向 -> 计算新蛇头 -> 撞墙/障碍判定 ->
 *       头插新蛇头；若吃到食物则不删尾（长度 +1）并结算分数/等级/食物，
 *       否则删除蛇尾（长度不变）。
 *
 * 说明：按需求，蛇撞到自己身体不会死亡，且允许反向覆盖自己。
 *
 * 非 PHASE_PLAYING 阶段调用为空操作。
 *
 * @param state 目标状态。
 */
void game_step(GameState *state);

/**
 * @brief 获取当前蛇头所在格子。
 * @param state 目标状态。
 * @return 蛇头格子；状态为空或蛇为空时返回 {0,0}。
 */
Cell game_head_cell(const GameState *state);

/**
 * @brief 获取当前蛇长（节）。
 * @param state 目标状态。
 * @return 蛇长；状态为空时返回 0。
 */
int game_snake_length(const GameState *state);

/**
 * @brief 判断某格子是否被蛇身占据（含蛇头）。
 * @param state 目标状态。
 * @param c     待查询格子。
 * @return 非 0 表示占据。
 */
int game_is_cell_on_snake(const GameState *state, Cell c);

/**
 * @brief 判断某格子是否为障碍物。
 * @param state 目标状态。
 * @param c     待查询格子。
 * @return 非 0 表示是障碍物。
 */
int game_is_obstacle(const GameState *state, Cell c);

/**
 * @brief 判断某格子是否在棋盘范围内。
 * @param c 待查询格子。
 * @return 非 0 表示在范围内。
 */
int game_is_inside_board(Cell c);

/**
 * @brief 尝试添加一个障碍物。
 *
 * 会拒绝以下情况：达到上限、坐标越界、与已有障碍重复、落在蛇身上、落在食物上。
 *
 * @param state 目标状态。
 * @param c     障碍物格子。
 * @return 1 表示添加成功，0 表示被拒绝。
 */
int game_add_obstacle(GameState *state, Cell c);

/**
 * @brief 在棋盘上随机生成一个食物（不与蛇身、障碍、现有食物重叠）。
 *
 * 优先随机尝试若干次，若棋盘接近填满则退化为顺序扫描，保证一定能找到空位；
 * 若确实无空位（棋盘填满）则保持原食物不变。
 *
 * @param state 目标状态。
 */
void game_spawn_food(GameState *state);

/* ===========================================================================
 * 道具与难度（实现见 items.c）
 * ========================================================================= */

/**
 * @brief 判断某格子是否落在“出生保护区”内。
 *
 * 出生保护区为初始蛇所在位置及其周边若干格；道具与障碍都不会生成在该区域内，
 * 以免开局就压在玩家身上。
 *
 * @param c 待查询格子。
 * @return 非 0 表示在保护区内。
 */
int game_is_in_birth_zone(Cell c);

/**
 * @brief 判断某格子是否放置了道具。
 * @param state 目标状态。
 * @param c     待查询格子。
 * @return 非 0 表示该格有道具。
 */
int game_is_cell_on_item(const GameState *state, Cell c);

/**
 * @brief 重置道具与效果状态（清空道具、清零效果、重置刷新计时）。
 * @param state 目标状态，允许为 NULL。
 */
void game_items_reset(GameState *state);

/**
 * @brief 按真实时间推进道具系统：道具存活倒计时、过期清除、定时刷新。
 * @param state 目标状态。
 * @param dt    距上一帧的秒数（<=0 时忽略）。
 */
void game_items_update(GameState *state, float dt);

/**
 * @brief 按真实时间推进道具效果计时（加速/减速/护盾到期归零）。
 * @param state 目标状态。
 * @param dt    距上一帧的秒数（<=0 时忽略）。
 */
void game_effects_update(GameState *state, float dt);

/**
 * @brief 尝试拾取位于 head 格子的道具并立即应用其效果。
 * @param state 目标状态。
 * @param head  蛇头所在格子。
 * @return 拾取到的道具类型；未拾取到返回 ITEM_NONE。
 */
ItemType game_items_pickup_at(GameState *state, Cell head);

/**
 * @brief 计算考虑加速/减速效果后的实际每格步进毫秒数。
 * @param state 目标状态。
 * @return 实际步进毫秒数（已限幅到 [EFFECT_SPEED_MIN_MS, EFFECT_SPEED_MAX_MS]）。
 */
int game_effective_speed_ms(const GameState *state);

/**
 * @brief 格子判定谓词：返回非 0 表示该格子满足条件（用于通用空位查找）。
 */
typedef int (*GameCellPredicate)(GameState *state, Cell cell);

/**
 * @brief 在棋盘上查找一个满足谓词的空格子。
 *
 * 先随机尝试若干次，若失败则顺序扫描整个棋盘，保证在存在空位时一定能找到。
 *
 * @param state 目标状态。
 * @param pred  判定谓词。
 * @param out   找到的格子（输出参数）。
 * @return 1 表示找到，0 表示无可用格子。
 */
int game_find_free_cell(GameState *state, GameCellPredicate pred, Cell *out);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_CORE_GAME_H */
