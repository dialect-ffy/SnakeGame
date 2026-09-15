/* ============================================================================
 * SnakeGame 核心逻辑层 —— 可注入种子的伪随机数发生器实现（xorshift32）
 * ==========================================================================*/

#include "core/rng.h"

/* 当种子为 0 时使用的兜底非零种子（黄金比例常量） */
#define RNG_FALLBACK_SEED 0x9E3779B9u

void rng_seed(Rng *rng, uint32_t seed)
{
    if (rng == NULL) {
        return;
    }
    rng->state = (seed != 0u) ? seed : RNG_FALLBACK_SEED;
}

uint32_t rng_next_u32(Rng *rng)
{
    if (rng == NULL) {
        return 0u;
    }
    uint32_t x = rng->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng->state = x;
    return x;
}

int rng_range(Rng *rng, int min_inclusive, int max_inclusive)
{
    if (max_inclusive < min_inclusive) {
        int tmp = min_inclusive;
        min_inclusive = max_inclusive;
        max_inclusive = tmp;
    }
    uint32_t span = (uint32_t)(max_inclusive - min_inclusive + 1);
    return min_inclusive + (int)(rng_next_u32(rng) % span);
}
