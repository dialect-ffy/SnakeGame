/* ============================================================================
 * SnakeGame UI 层 —— 霓虹暗黑风主题辅助函数实现
 * ==========================================================================*/

#include "ui/theme.h"

/* 将浮点值夹取到 [0, 1] 区间 */
static float clamp01(float t)
{
    if (t < 0.0f) {
        return 0.0f;
    }
    if (t > 1.0f) {
        return 1.0f;
    }
    return t;
}

Color theme_lerp_color(Color a, Color b, float t)
{
    t = clamp01(t);
    Color out;
    out.r = (unsigned char)(a.r + (b.r - a.r) * t);
    out.g = (unsigned char)(a.g + (b.g - a.g) * t);
    out.b = (unsigned char)(a.b + (b.b - a.b) * t);
    out.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return out;
}

Color theme_alpha(Color c, unsigned char alpha)
{
    c.a = alpha;
    return c;
}
