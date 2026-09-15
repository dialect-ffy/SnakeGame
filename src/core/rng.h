/* ============================================================================
 * SnakeGame 核心逻辑层 —— 可注入种子的伪随机数发生器（PRNG）
 * ----------------------------------------------------------------------------
 * 为什么不用标准库 rand()：
 *   1. 单元测试需要“固定种子 -> 固定序列”，保证结果可复现；
 *   2. 逻辑层不依赖全局状态，随机数状态由调用方（GameState）持有，便于测试。
 * 算法：xorshift32，简单快速，够用且确定性强。
 * ==========================================================================*/

#ifndef SNAKE_CORE_RNG_H
#define SNAKE_CORE_RNG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 伪随机数发生器状态。
 */
typedef struct {
    uint32_t state;   /**< 内部状态；为 0 时会被替换为非零种子 */
} Rng;

/**
 * @brief 以给定种子初始化随机源。
 * @param rng  随机源指针，不可为 NULL。
 * @param seed 种子；若为 0 会自动替换为一个非零常量（xorshift 不能处于 0 状态）。
 */
void rng_seed(Rng *rng, uint32_t seed);

/**
 * @brief 生成下一个 32 位无符号随机数。
 * @param rng 随机源指针，不可为 NULL。
 * @return 随机数。
 */
uint32_t rng_next_u32(Rng *rng);

/**
 * @brief 生成 [min_inclusive, max_inclusive] 闭区间内的随机整数。
 * @param rng           随机源指针，不可为 NULL。
 * @param min_inclusive 下界（含）。
 * @param max_inclusive 上界（含）；若小于下界会自动交换。
 * @return 区间内的随机整数。
 */
int rng_range(Rng *rng, int min_inclusive, int max_inclusive);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_CORE_RNG_H */
