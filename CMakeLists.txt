# ============================================================================
# SnakeGame 根构建脚本
# ----------------------------------------------------------------------------
# 职责：
#   1. 使用 CMake FetchContent 自动拉取并编译第三方依赖（raylib / googletest），
#      避免手动安装库，保证 Windows + Visual Studio 下一键构建。
#   2. 定义三个目标：
#        - snake_core : 纯 C 逻辑静态库（零 raylib 依赖，供游戏与测试共同链接）
#        - SnakeGame  : 基于 raylib 的可执行程序（渲染 + 输入 + 主循环）
#        - snake_tests: 基于 googletest 的单元测试可执行程序（C++ 编写）
# ============================================================================

cmake_minimum_required(VERSION 3.20)

project(SnakeGame
    VERSION 0.1.0
    DESCRIPTION "A single-player Snake game implemented in C with raylib"
    LANGUAGES C CXX)

# ----------------------------------------------------------------------------
# 语言标准：逻辑层使用 C11，测试层使用 C++17
# ----------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------------
# MSVC 源码编码：强制以 UTF-8 解析源文件。
# 若不加此选项，中文注释在代码页 936（GBK）环境下会被误读，导致编译错误。
# ----------------------------------------------------------------------------
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 统一把可执行文件输出到 build/bin，便于运行与手动验收
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 允许通过 -DENABLE_TESTS=OFF 关闭测试（默认开启）
option(ENABLE_TESTS "Build unit tests with googletest" ON)

# ----------------------------------------------------------------------------
# 拉取第三方依赖
# ----------------------------------------------------------------------------
include(FetchContent)

# --- raylib：轻量跨平台游戏库，用于窗口、绘图与输入 ---
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # 不编译 raylib 示例，缩短构建时间
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)   # 不编译 raylib 自带游戏
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(raylib)

# --- googletest：单元测试框架（仅测试代码使用，不影响游戏本体） ---
if(ENABLE_TESTS)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)          # 不安装 gtest 到系统
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # Windows 下与 MSVC 运行时一致
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()  # 允许使用 ctest 运行测试
endif()

# ----------------------------------------------------------------------------
# snake_core：纯 C 逻辑静态库（零 raylib 依赖，可被 gtest 独立链接）
# ----------------------------------------------------------------------------
add_library(snake_core STATIC
    src/core/version.c
    src/core/list.c       # P1：双向循环链表（哨兵）ADT
    src/core/rng.c        # P3：可注入种子的 PRNG
    src/core/game.c       # P3：游戏状态与规则
    src/core/items.c      # P5：道具系统与效果
    src/core/persist.c)   # P7：最高分持久化

target_include_directories(snake_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src")

# ----------------------------------------------------------------------------
# SnakeGame：可执行程序（raylib 渲染层 + 主循环）
# ----------------------------------------------------------------------------
add_executable(SnakeGame
    src/main.c
    src/ui/theme.c
    src/ui/render.c
    src/ui/input.c
    src/ui/audio.c)

target_link_libraries(SnakeGame PRIVATE snake_core raylib)

# ----------------------------------------------------------------------------
# snake_tests：googletest 单元测试（C++ 编写，通过 extern "C" 链接纯 C 逻辑）
# ----------------------------------------------------------------------------
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
