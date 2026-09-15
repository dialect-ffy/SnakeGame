// ============================================================================
// P0 冒烟测试：验证 googletest 与纯 C 逻辑库（snake_core）的链接管线
// ----------------------------------------------------------------------------
// 目的不是测试业务逻辑，而是确认：
//   1. gtest 能正常编译、运行；
//   2. C++ 测试代码能通过 extern "C" 调用 C 逻辑函数。
// ============================================================================

#include <gtest/gtest.h>

// 以 C 语言方式链接纯 C 逻辑层头文件，避免 C++ 名称修饰导致链接失败
extern "C" {
#include "core/version.h"
}

// 版本字符串应非空，且与预期值一致
TEST(SmokeTest, CoreVersionIsAvailable)
{
    const char *version = snake_core_version();
    ASSERT_NE(version, nullptr);
    EXPECT_STREQ(version, "0.1.0");
}
