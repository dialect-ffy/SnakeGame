/* ============================================================================
 * SnakeGame UI 层 —— 音效合成实现
 * ----------------------------------------------------------------------------
 * 合成方式：为每个音效生成一段带线性衰减包络的正弦波，打包成 16bit 单声道 Wave，
 *           再用 LoadSoundFromWave 转成 Sound。全程不依赖任何外部音频文件。
 * ==========================================================================*/

#include "ui/audio.h"

#include <math.h>
#include <stdlib.h>

#include <raylib.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

static Sound g_sounds[SFX_COUNT];
static int   g_ready = 0;

/**
 * @brief 合成一个正弦提示音。
 * @param freq     频率（Hz）。
 * @param duration 时长（秒）。
 * @param volume   音量（0~1）。
 * @return 生成的声音；失败返回一个零值 Sound。
 */
static Sound make_tone(float freq, float duration, float volume)
{
    const int sample_rate = 44100;
    const int frames = (int)(duration * (float)sample_rate);

    short *samples = (short *)malloc((size_t)frames * sizeof(short));
    if (samples == NULL) {
        return (Sound){ 0 };
    }

    for (int i = 0; i < frames; ++i) {
        float t = (float)i / (float)sample_rate;
        float envelope = 1.0f - (float)i / (float)frames;   /* 线性衰减，避免爆音 */
        float value = sinf(2.0f * PI * freq * t) * envelope * volume;
        samples[i] = (short)(value * 32000.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = (unsigned int)frames;
    wave.sampleRate = (unsigned int)sample_rate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples;

    Sound sound = LoadSoundFromWave(wave);   /* raylib 会拷贝数据到音频缓冲 */
    free(samples);
    return sound;
}

int audio_init(void)
{
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        g_ready = 0;
        return 0;
    }

    g_sounds[SFX_EAT_FOOD] = make_tone(880.0f, 0.07f, 0.35f);
    g_sounds[SFX_EAT_ITEM] = make_tone(1245.0f, 0.12f, 0.35f);
    g_sounds[SFX_LEVEL_UP] = make_tone(1568.0f, 0.18f, 0.40f);
    g_sounds[SFX_DIE]      = make_tone(180.0f, 0.40f, 0.45f);

    g_ready = 1;
    return 1;
}

void audio_shutdown(void)
{
    if (g_ready) {
        for (int i = 0; i < SFX_COUNT; ++i) {
            UnloadSound(g_sounds[i]);
        }
        g_ready = 0;
    }
    CloseAudioDevice();
}

void audio_play(Sfx sfx)
{
    if (!g_ready || sfx < 0 || sfx >= SFX_COUNT) {
        return;
    }
    PlaySound(g_sounds[sfx]);
}
