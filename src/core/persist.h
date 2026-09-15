/* ============================================================================
 * SnakeGame 核心逻辑层 —— 最高分持久化
 * ----------------------------------------------------------------------------
 * 用最简的纯文本格式保存一个整数（最高分），便于人工查看与调试。
 * 纯 C，无 raylib 依赖，文件路径由调用方提供，便于单元测试使用临时文件。
 * ==========================================================================*/

#ifndef SNAKE_CORE_PERSIST_H
#define SNAKE_CORE_PERSIST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 从文件读取最高分。
 * @param path 文件路径。
 * @return 读取到的最高分；文件不存在、无法读取或内容非法时返回 0。
 */
int persist_load_high_score(const char *path);

/**
 * @brief 把最高分写入文件（覆盖写）。
 * @param path  文件路径。
 * @param score 要保存的分数。
 * @return 1 表示写入成功，0 表示失败（如路径非法或无写权限）。
 */
int persist_save_high_score(const char *path, int score);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_CORE_PERSIST_H */
