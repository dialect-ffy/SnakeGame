// ============================================================================
// P3 单元测试：可注入种子的伪随机数发生器（Rng）
// ----------------------------------------------------------------------------
// 覆盖点：可复现性、区间范围、0 种子兜底、区间边界交换。
// ============================================================================

#include <gtest/gtest.h>

extern "C" {
#include "core/rng.h"
}

// 相同种子必须产生完全相同的序列（测试可复现的基础）
TEST(RngTest, SameSeedProducesSameSequence)
{
    Rng a, b;
    rng_seed(&a, 42u);
    rng_seed(&b, 42u);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng_next_u32(&a), rng_next_u32(&b));
    }
}

// 0 种子会被替换为非零常量，序列仍然可复现且不退化
TEST(RngTest, ZeroSeedFallsBackAndStaysReproducible)
{
    Rng a, b;
    rng_seed(&a, 0u);
    rng_seed(&b, 0u);

    EXPECT_NE(a.state, 0u);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(rng_next_u32(&a), rng_next_u32(&b));
    }
}

// rng_range 结果始终落在闭区间内
TEST(RngTest, RangeStaysWithinInclusiveBounds)
{
    Rng r;
    rng_seed(&r, 7u);

    for (int i = 0; i < 1000; ++i) {
        int v = rng_range(&r, 3, 9);
        EXPECT_GE(v, 3);
        EXPECT_LE(v, 9);
    }
}

// 单点区间应恒返回该值
TEST(RngTest, RangeSingleValueReturnsThatValue)
{
    Rng r;
    rng_seed(&r, 8u);

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(rng_range(&r, 5, 5), 5);
    }
}

// 上下界颠倒时应自动交换，仍返回区间内值
TEST(RngTest, RangeSwapsReversedBounds)
{
    Rng r;
    rng_seed(&r, 9u);

    for (int i = 0; i < 100; ++i) {
        int v = rng_range(&r, 9, 3);
        EXPECT_GE(v, 3);
        EXPECT_LE(v, 9);
    }
}
