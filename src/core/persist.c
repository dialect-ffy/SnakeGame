/* ============================================================================
 * SnakeGame 核心逻辑层 —— 最高分持久化实现
 * ----------------------------------------------------------------------------
 * 文件格式：一行十进制整数文本（例如 "1230"）。
 * 读取时做基本校验（非负、解析成功），异常一律回退为 0，保证游戏不会因存档损坏而崩溃。
 * ==========================================================================*/

#include "core/persist.h"

#include <stdio.h>

int persist_load_high_score(const char *path)
{
    if (path == NULL) {
        return 0;
    }

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        return 0;   /* 文件不存在或无法打开：视为 0 */
    }

    int value = 0;
    int parsed = fscanf(fp, "%d", &value);
    fclose(fp);

    if (parsed != 1 || value < 0) {
        return 0;   /* 内容非法：回退为 0 */
    }
    return value;
}

int persist_save_high_score(const char *path, int score)
{
    if (path == NULL || score < 0) {
        return 0;
    }

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        return 0;   /* 无写权限或路径非法 */
    }

    int written = fprintf(fp, "%d\n", score);
    int closed = fclose(fp);

    return (written > 0 && closed == 0) ? 1 : 0;
}
