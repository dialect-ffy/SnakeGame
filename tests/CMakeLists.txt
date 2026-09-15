# ============================================================================
# SnakeGame 单元测试构建脚本
# ----------------------------------------------------------------------------
# 测试代码使用 C++ 编写（googletest 要求），通过 extern "C" 头文件链接
# 纯 C 的 snake_core 静态库，从而验证逻辑层功能。
# 每个 Phase 新增测试文件后，在此处追加对应的 test_*.cpp 即可。
# ============================================================================

# P0 冒烟 + P1 链表 + P3 随机源/游戏逻辑 + P5 道具 + P7 存档
add_executable(snake_tests
    test_smoke.cpp
    test_list.cpp
    test_rng.cpp
    test_game.cpp
    test_items.cpp
    test_persist.cpp)

target_link_libraries(snake_tests PRIVATE
    snake_core
    GTest::gtest_main)

# 注册到 ctest：可用 `ctest --test-dir build -C Debug --output-on-failure` 运行
include(GoogleTest)
gtest_discover_tests(snake_tests)
