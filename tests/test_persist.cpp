// ============================================================================
// P7 单元测试：最高分持久化
// ----------------------------------------------------------------------------
// 覆盖点：读取不存在文件、写入后读取、覆盖写入、NULL/非法参数安全。
// ============================================================================

#include <gtest/gtest.h>

#include <cstdio>

extern "C" {
#include "core/persist.h"
}

namespace {
const char *kPath = "test_highscore_tmp.dat";

// 每个用例前后都清理临时文件，保证互不影响
void RemoveTempFile()
{
    std::remove(kPath);
}
}  // namespace

// 文件不存在时应返回 0
TEST(PersistTest, LoadNonexistentReturnsZero)
{
    RemoveTempFile();
    EXPECT_EQ(persist_load_high_score(kPath), 0);
}

// 写入后再读取应得到相同分数
TEST(PersistTest, SaveThenLoadRoundTrip)
{
    RemoveTempFile();

    ASSERT_EQ(persist_save_high_score(kPath, 1230), 1);
    EXPECT_EQ(persist_load_high_score(kPath), 1230);

    RemoveTempFile();
}

// 再次写入应覆盖旧值
TEST(PersistTest, SaveOverwritesPrevious)
{
    RemoveTempFile();

    ASSERT_EQ(persist_save_high_score(kPath, 100), 1);
    ASSERT_EQ(persist_save_high_score(kPath, 999), 1);
    EXPECT_EQ(persist_load_high_score(kPath), 999);

    RemoveTempFile();
}

// 0 分也能正常保存与读取
TEST(PersistTest, SaveZeroScore)
{
    RemoveTempFile();

    ASSERT_EQ(persist_save_high_score(kPath, 0), 1);
    EXPECT_EQ(persist_load_high_score(kPath), 0);

    RemoveTempFile();
}

// NULL 路径与负分应安全失败
TEST(PersistTest, InvalidArgumentsAreSafe)
{
    EXPECT_EQ(persist_load_high_score(nullptr), 0);
    EXPECT_EQ(persist_save_high_score(nullptr, 10), 0);
    EXPECT_EQ(persist_save_high_score(kPath, -5), 0);
}
