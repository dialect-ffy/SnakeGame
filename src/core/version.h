/* ============================================================================
 * SnakeGame 核心逻辑层版本信息
 * ----------------------------------------------------------------------------
 * 该文件在 P0（基建）阶段仅用于验证：
 *   1. 纯 C 静态库 snake_core 能被正确编译；
 *   2. C 代码能通过 extern "C" 被 C++ 编写的 googletest 链接与调用。
 * 后续 Phase 会在此基础上加入 list / game / items / persist 等模块。
 * ==========================================================================*/

#ifndef SNAKE_CORE_VERSION_H
#define SNAKE_CORE_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 返回核心逻辑层的版本字符串。
 *
 * 该函数不依赖 raylib，可在无图形环境下被单元测试调用。
 *
 * @return 以 '\0' 结尾的静态版本字符串，调用者无需释放。
 */
const char *snake_core_version(void);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_CORE_VERSION_H */
