#pragma once
#include <graphics.h>
#include <cmath>
#include <vector>
#include <cstdlib>

// ============================================================================
//  DrawUtil.h —— 国风绘图的公共小工具
//
//  说明：EasyX 既没有 alpha 通道，也没有渐变填充函数。这里用两个技巧弥补：
//    1) BlendColor           —— 把颜色向背景色混合，模拟"半透明"
//    2) FillGradientRoundRect—— 逐行填充 + 圆角几何内缩，得到圆角垂直渐变
//  另外提供带暗影与光晕的文字（简易 bloom），用于标题 / 卦名 / 动爻。
//
//  本文件为纯头文件内联实现，不需要加入 CMakeLists 的源文件列表。
// ============================================================================

// 颜色向 to 插值：t = 0 返回 from，t = 1 返回 to
inline COLORREF BlendColor(COLORREF from, COLORREF to, double t) {
    if (t <= 0.0) return from;
    if (t >= 1.0) return to;
    const int fr = GetRValue(from), fg = GetGValue(from), fb = GetBValue(from);
    const int tr = GetRValue(to),   tg = GetGValue(to),   tb = GetBValue(to);
    return RGB((int)(fr + (tr - fr) * t),
               (int)(fg + (tg - fg) * t),
               (int)(fb + (tb - fb) * t));
}

// 夜空底色（与 main.cpp 里 cleardevice 用的背景色保持一致）
inline COLORREF NightBg() { return RGB(15, 10, 25); }

// ---- 全屏窗口的 TOPMOST 让位 / 恢复 ----
// 图形窗口是 HWND_TOPMOST（为了遮住任务栏），弹模态对话框前必须先让位，
// 否则 InputBox 等系统弹窗会被压在图形窗口下面看不见。
inline bool& TopmostYieldedFlag() {
    static bool yielded = false;   // 是否正处于"让出前台"状态
    return yielded;
}

inline void YieldTopmost() {
    HWND h = GetHWnd();
    if (h) SetWindowPos(h, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    TopmostYieldedFlag() = true;
}

inline void RestoreTopmost() {
    HWND h = GetHWnd();
    if (h) SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    TopmostYieldedFlag() = false;
}

// 当前是否已让出前台（打开浏览器等外部程序后，需要等用户回来再恢复）
inline bool IsTopmostYielded() { return TopmostYieldedFlag(); }

// 在圆角矩形范围内绘制垂直渐变（top -> bottom）。
// 每一行按圆角几何计算水平内缩量，因此天然形成圆角，无需裁剪区域。
inline void FillGradientRoundRect(int x1, int y1, int x2, int y2, int r,
                                  COLORREF top, COLORREF bottom) {
    if (x2 <= x1 || y2 <= y1) return;
    if (r > (x2 - x1) / 2) r = (x2 - x1) / 2;
    if (r > (y2 - y1) / 2) r = (y2 - y1) / 2;
    if (r < 0) r = 0;

    const int rows = y2 - y1;
    for (int i = 0; i < rows; ++i) {
        const int y = y1 + i;

        double inset = 0.0;
        if (r > 0) {
            if (y < y1 + r) {
                const double dy = (double)((y1 + r) - y);
                inset = r - std::sqrt((double)r * r - dy * dy);
            } else if (y > y2 - r) {
                const double dy = (double)(y - (y2 - r));
                inset = r - std::sqrt((double)r * r - dy * dy);
            }
        }

        const int xl = x1 + (int)inset;
        const int xr = x2 - (int)inset;
        if (xr <= xl) continue;

        const double t = (rows > 1) ? (double)i / (double)(rows - 1) : 0.0;
        setfillcolor(BlendColor(top, bottom, t));
        solidrectangle(xl, y, xr, y + 1);
    }
}

// 玻璃拟态面板：外层柔光 + 圆角渐变底 + 细边框
inline void DrawGlassPanel(int x1, int y1, int x2, int y2, int radius = 16,
                           COLORREF edge = RGB(255, 215, 0), double edgeAlpha = 0.28) {
    const COLORREF bg = NightBg();

    setfillcolor(BlendColor(bg, edge, 0.05));
    setlinecolor(BlendColor(bg, edge, 0.05));
    solidroundrect(x1 - 6, y1 - 6, x2 + 6, y2 + 6, radius + 6, radius + 6);

    FillGradientRoundRect(x1, y1, x2, y2, radius,
                          BlendColor(bg, RGB(40, 34, 56), 0.84),
                          BlendColor(bg, RGB(24, 20, 36), 0.92));

    setlinecolor(BlendColor(bg, edge, edgeAlpha));
    setlinestyle(PS_SOLID, 1);
    roundrect(x1, y1, x2, y2, radius, radius);
}

// 带暗影与光晕的文字（简易 bloom）。
//   core     —— 文字本色
//   glow     —— 光晕颜色
//   strength —— 光晕强度，0 表示只要暗影
inline void DrawGlowText(int x, int y, const wchar_t* text,
                         COLORREF core, COLORREF glow, double strength,
                         COLORREF bg = RGB(15, 10, 25)) {
    // 右下暗影，增加厚重感
    settextcolor(BlendColor(bg, RGB(0, 0, 0), 0.55));
    outtextxy(x + 3, y + 3, text);

    if (strength > 0.0) {
        static const int kOff[6][2] = { {-3, 0}, {3, 0}, {0, -3}, {0, 3}, {-2, -2}, {2, 2} };
        for (const auto& o : kOff) {
            settextcolor(BlendColor(bg, glow, strength * 0.30));
            outtextxy(x + o[0], y + o[1], text);
        }
    }

    settextcolor(core);
    outtextxy(x, y, text);
}

// ---- 星光：伪随机分布 + 正弦闪烁（位置由哈希决定，不会每帧乱跳）----
inline void DrawStarField(int W, int H, double time, int count = 90,
                          COLORREF bg = RGB(15, 10, 25)) {
    if (W <= 0 || H <= 0 || count <= 0) return;
    const int skyH = (int)(H * 0.62);
    if (skyH <= 1) return;

    for (int i = 0; i < count; ++i) {
        unsigned int h = (unsigned int)(i * 2654435761u);
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;

        const int    x     = (int)(h % (unsigned int)W);
        const int    y     = (int)((h >> 8) % (unsigned int)skyH);
        const double phase = (double)((h >> 20) % 628) / 100.0;      // 0 ~ 6.28
        const double tw    = 0.5 + 0.5 * std::sin(time * 1.8 + phase);
        const double a     = 0.06 + 0.55 * tw * tw;                  // 多数时间很暗
        const int    r     = 1 + (int)((h >> 17) & 1u);

        const COLORREF c = BlendColor(bg, RGB(236, 228, 205), a);
        setfillcolor(c);
        setlinecolor(c);
        solidcircle(x, y, r);
    }
}

// ---- 一朵云：多个重叠椭圆拼成 ----
inline void DrawCloud(int cx, int cy, int w, int h, COLORREF color) {
    if (w < 4 || h < 2) return;
    setfillcolor(color);
    setlinecolor(color);
    solidellipse(cx - w / 2,       cy - h / 4, cx - w / 6,   cy + h / 4);
    solidellipse(cx - w / 3,       cy - h / 2, cx + w / 3,   cy + h / 2);
    solidellipse(cx + w / 6,       cy - h / 4, cx + w / 2,   cy + h / 4);
    solidellipse(cx - w / 8,       cy - h / 3, cx + w / 8,   cy + h / 5);
}

// ---- 古风灯笼：灯绳 + 光晕 + 灯身 + 骨架 + 灯口 + 流苏 ----
inline void DrawLantern(int cx, int cordTop, int cy, int r, double glow,
                        COLORREF bg = RGB(15, 10, 25)) {
    if (r < 3) return;

    // 灯绳
    setlinecolor(BlendColor(bg, RGB(172, 142, 82), 0.85));
    setlinestyle(PS_SOLID, 1);
    line(cx, cordTop, cx, cy - (int)(r * 0.9));

    // 光晕：几层逐渐收紧
    const COLORREF halo = BlendColor(bg, RGB(220, 62, 50), 1.0);
    setlinecolor(BlendColor(halo, bg, 1.0 - glow * 0.06));
    setfillcolor(BlendColor(halo, bg, 1.0 - glow * 0.06));
    solidcircle(cx, cy, r * 3);
    setfillcolor(BlendColor(halo, bg, 1.0 - glow * 0.11));
    solidcircle(cx, cy, (int)(r * 2.2));
    setfillcolor(BlendColor(halo, bg, 1.0 - glow * 0.20));
    solidcircle(cx, cy, (int)(r * 1.5));

    // 灯身
    setfillcolor(BlendColor(bg, RGB(186, 38, 36), 1.0));
    setlinecolor(BlendColor(bg, RGB(255, 176, 120), 0.9));
    solidellipse(cx - r, cy - (int)(r * 0.82), cx + r, cy + (int)(r * 0.82));

    // 骨架竖纹
    setlinecolor(BlendColor(bg, RGB(255, 190, 120), 0.32));
    for (int i = -1; i <= 1; ++i) {
        if (i == 0) continue;
        const int ox = cx + i * r / 2;
        ellipse(ox - r / 6, cy - (int)(r * 0.70), ox + r / 6, cy + (int)(r * 0.70));
    }

    // 上下灯口
    setfillcolor(BlendColor(bg, RGB(228, 186, 92), 0.95));
    setlinecolor(BlendColor(bg, RGB(228, 186, 92), 0.95));
    solidrectangle(cx - r / 3, cy - (int)(r * 0.95), cx + r / 3, cy - (int)(r * 0.78));
    solidrectangle(cx - r / 3, cy + (int)(r * 0.78), cx + r / 3, cy + (int)(r * 0.95));

    // 流苏
    setlinecolor(BlendColor(bg, RGB(228, 186, 92), 0.9));
    line(cx, cy + (int)(r * 0.95), cx, cy + (int)(r * 1.45));
    setfillcolor(BlendColor(bg, RGB(214, 60, 48), 0.92));
    solidrectangle(cx - r / 8, cy + (int)(r * 1.45), cx + r / 8, cy + (int)(r * 1.85));
}

// ---- 毛笔笔触：两端尖尖的横向光条，中间最亮 ----
// 用椭圆轮廓逐行填充：垂直方向越靠中间，水平跨度越满
inline void DrawBrushBar(int left, int top, int right, int bottom,
                         COLORREF core, COLORREF edge,
                         COLORREF bg = RGB(15, 10, 25)) {
    const int w = right - left;
    const int h = bottom - top;
    if (w <= 0 || h <= 0) return;

    const int cx = (left + right) / 2;
    for (int i = 0; i < h; ++i) {
        const double dy = (h > 1) ? ((double)i / (double)(h - 1) - 0.5) * 2.0 : 0.0;  // -1..1
        const double k0 = 1.0 - dy * dy;                                            // 可能为负
        const double shape = (k0 > 0.0) ? std::sqrt(k0) : 0.0;                      // 0..1
        const int inset = (int)((double)w * 0.5 * (1.0 - shape));
        const int xl = left + inset;
        const int xr = right - inset;
        if (xr <= xl) continue;

        // 中间亮、两端暗，形成渐变金光
        const double k = 0.30 + 0.70 * shape;
        setfillcolor(BlendColor(bg, BlendColor(edge, core, k), 0.25 + 0.60 * k));
        solidrectangle(xl, top + i, xr, top + i + 1);
    }
}


// ============================================================================
//  FireflyField —— 萤火虫粒子群（ header-only，可被多个场景复用 ）
//
//  每只萤火虫带光晕 + 亮核，位置按正弦缓慢游走，亮度用 tw^2 加权，
//  因此大部分时间很暗、偶尔亮起，比线性闪烁更自然。
// ============================================================================
class FireflyField {
public:
    void Init(int count, int w, int h) {
        m_items.clear();
        if (count < 0) count = 0;
        m_items.reserve((size_t)count);
        for (int i = 0; i < count; ++i) {
            Item f;
            f.x     = (float)(rand() % (w > 0 ? w : 1));
            f.y     = (float)(rand() % (h > 0 ? h : 1));
            f.phase = (float)(rand() % 628) / 100.0f;
            f.speed = 0.35f + (float)(rand() % 10) / 12.0f;
            f.size  = 1.2f + (float)(rand() % 5) / 10.0f;
            m_items.push_back(f);
        }
    }

    void Draw(double time, COLORREF bg = RGB(15, 10, 25)) const {
        for (const auto& f : m_items) {
            const float x = (float)(f.x + std::sin(time * f.speed + f.phase) * 26.0);
            const float y = (float)(f.y + std::cos(time * f.speed * 0.7 + f.phase) * 20.0);

            const double tw = 0.5 + 0.5 * std::sin(time * 2.4 + f.phase * 3.0);
            const double a  = 0.05 + 0.85 * tw * tw;

            const COLORREF halo = BlendColor(bg, RGB(255, 224, 138), a * 0.20);
            setfillcolor(halo);
            setlinecolor(halo);
            solidcircle((int)x, (int)y, (int)(f.size * 3.4f));

            const COLORREF core = BlendColor(bg, RGB(255, 242, 196), a);
            setfillcolor(core);
            setlinecolor(core);
            solidcircle((int)x, (int)y, (int)f.size);
        }
    }

private:
    struct Item {
        float x = 0, y = 0;
        float phase = 0;
        float speed = 1.0f;
        float size  = 1.5f;
    };
    std::vector<Item> m_items;
};
