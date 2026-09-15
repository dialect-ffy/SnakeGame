/* ============================================================================
 * SnakeGame UI 层 —— 音效（代码合成，无外部音频文件）
 * ----------------------------------------------------------------------------
 * 使用 raylib 的 Wave/Sound 接口，在初始化时用正弦波合成几个短提示音，
 * 运行时按事件播放。无 BGM。
 * ==========================================================================*/

#ifndef SNAKE_UI_AUDIO_H
#define SNAKE_UI_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 音效种类。
 */
typedef enum {
    SFX_EAT_FOOD = 0,   /**< 吃到普通食物 */
    SFX_EAT_ITEM,       /**< 吃到道具 */
    SFX_LEVEL_UP,       /**< 升级 */
    SFX_DIE,            /**< 死亡 */
    SFX_COUNT           /**< 音效数量（哨兵） */
} Sfx;

/**
 * @brief 初始化音频设备并合成所有音效。
 * @return 1 表示初始化成功，0 表示音频设备不可用（此时播放为空操作）。
 */
int audio_init(void);

/**
 * @brief 释放所有音效并关闭音频设备。
 */
void audio_shutdown(void);

/**
 * @brief 播放指定音效（未初始化时为空操作）。
 * @param sfx 音效种类。
 */
void audio_play(Sfx sfx);

#ifdef __cplusplus
}
#endif

#endif /* SNAKE_UI_AUDIO_H */
