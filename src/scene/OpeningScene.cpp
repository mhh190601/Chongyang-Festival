#include "OpeningScene.h"
#include "../core/Input.h"
#include "../core/Config.h"
#include "../core/DrawUtil.h"

#include <windows.h>
#include <cmath>
#include <vector>

namespace {

// ---- 时间轴（单位：秒）----
constexpr double kLanternStart = 0.0;    // 孔明灯开始升腾
constexpr double kLanternDur   = 1.3;    // 持续到点火
constexpr double kTitleStart   = 1.3;    // 主标题点火浮现
constexpr double kTitleDur     = 1.7;    // 主标题时长
constexpr double kSubStart     = 2.6;    // 副标题开始
constexpr double kSubDur       = 1.5;    // 副标题时长
constexpr double kTipStart     = 3.4;    // 底部提示出现
constexpr double kTipDur       = 0.8;
constexpr double kTotalTime    = 6.0;    // 总时长
constexpr double kSkipDelay    = 1.5;    // 防抖：1.5 秒后才允许按键跳过
constexpr double kBackFadeDur  = 1.5;    // 背景（山水 / 菊花）淡入时长
constexpr double kIgniteDur    = 0.9;    // 点火光环扩散时长

constexpr double kPI = 3.14159265358979323846;

const COLORREF kBg   = NightBg();
const COLORREF kGold = RGB(255, 215, 0);

// ---- 圆润的山脊：多个正弦叠加，轮廓平滑无尖角 ----
void BuildRidge(std::vector<POINT>& pts, int W, int H,
                double baseY, double amp, double phase, int samples) {
    pts.clear();
    for (int i = 0; i <= samples; ++i) {
        const double t = (double)i / samples;
        const double y = baseY
                       + amp * 0.55 * std::sin(t * 3.1 + phase)
                       + amp * 0.30 * std::sin(t * 6.7 + phase * 1.7)
                       + amp * 0.15 * std::sin(t * 11.3 + phase * 2.3);
        POINT p;
        p.x = (LONG)(t * W);
        p.y = (LONG)((y < 0.0) ? 0.0 : ((y > H) ? (double)H : y));
        pts.push_back(p);
    }
    POINT br; br.x = W; br.y = H; pts.push_back(br);
    POINT bl; bl.x = 0; bl.y = H; pts.push_back(bl);
}

// ---- 水墨远山：六层纯色剪影 + 三道云海 ----
// 注意：fillpolygon 会用“当前线条颜色”描边，所以必须把 linecolor 设成和
// fillcolor 一致，否则前面设过的红色线条颜色会给山体描上红边。
void DrawInkMountains(double fade, double time) {
    const int W = getwidth();
    const int H = getheight();
    if (W <= 0 || H <= 0) return;

    struct Layer {
        double   baseY;
        double   amp;
        double   phase;
        COLORREF color;
    };

    // 由远及近：越远越淡（向底色混合），越近越实，形成有厚度的层叠山峦
    const Layer layers[6] = {
        { 0.60, 0.14, 0.0, BlendColor(kBg, RGB(26, 20, 36), fade * 0.30) },
        { 0.66, 0.11, 1.7, BlendColor(kBg, RGB(23, 17, 33), fade * 0.44) },
        { 0.73, 0.09, 3.4, BlendColor(kBg, RGB(20, 15, 30), fade * 0.60) },
        { 0.80, 0.07, 5.1, BlendColor(kBg, RGB(17, 12, 27), fade * 0.76) },
        { 0.88, 0.05, 6.6, BlendColor(kBg, RGB(13,  9, 21), fade * 0.90) },
        { 0.96, 0.03, 8.2, BlendColor(kBg, RGB(10,  5, 15), fade)        }
    };

    std::vector<POINT> pts;
    for (const auto& L : layers) {
        BuildRidge(pts, W, H, H * L.baseY, H * L.amp, L.phase, 56);
        setfillcolor(L.color);
        setlinecolor(L.color);
        fillpolygon(pts.data(), (int)pts.size());
    }

    // ---- 云海：三道半透明云带横卧山腰，缓慢左右漂移 ----
    struct Band { double y; double scale; int count; COLORREF color; double phase; };
    const Band bands[3] = {
        { 0.615, 1.00, 3, BlendColor(kBg, RGB(230, 228, 236), fade * 0.17), 0.0 },
        { 0.700, 1.25, 4, BlendColor(kBg, RGB(212, 212, 222), fade * 0.12), 2.1 },
        { 0.790, 0.85, 3, BlendColor(kBg, RGB(230, 228, 236), fade * 0.15), 4.3 }
    };

    for (const auto& b : bands) {
        for (int i = 0; i < b.count; ++i) {
            const double t = (b.count == 1) ? 0.5 : (double)i / (double)(b.count - 1);
            const int cw    = (int)(W * 0.16 * b.scale);
            const int ch    = (int)(H * 0.030 * b.scale);
            const int drift = (int)(std::sin(time * 0.10 + b.phase + i * 1.3) * W * 0.012);
            DrawCloud((int)(W * (0.12 + 0.76 * t)) + drift, (int)(H * b.y), cw, ch, b.color);
        }
    }
}

// ---- 单层花瓣环：细长花瓣沿自身角度倾斜 ----
void DrawPetalRing(int cx, int cy, int r, int count, double tiltDeg,
                   double lenFactor, double widthFactor, COLORREF color) {
    const double a = r * lenFactor;
    const double b = r * widthFactor;

    setfillcolor(color);
    setlinecolor(color);
    for (int i = 0; i < count; ++i) {
        const double th = (tiltDeg + i * 360.0 / count) * kPI / 180.0;
        const double c  = std::cos(th);
        const double s  = std::sin(th);

        const double ox = cx + c * r * 0.52;
        const double oy = cy + s * r * 0.52;

        const double hw = std::sqrt(a * a * c * c + b * b * s * s);
        const double hh = std::sqrt(a * a * s * s + b * b * c * c);

        solidellipse((int)(ox - hw), (int)(oy - hh), (int)(ox + hw), (int)(oy + hh));
    }
}

// ---- 金菊：光晕 + 两层细长花瓣 + 花心 ----
void DrawChrysanthemum(int cx, int cy, int r, double tiltDeg, double fade) {
    if (r <= 2) return;

    const COLORREF halo = BlendColor(kBg, RGB(200, 150, 50), 1.0);
    setlinecolor(BlendColor(halo, kBg, 1.0 - fade * 0.05));
    setfillcolor(BlendColor(halo, kBg, 1.0 - fade * 0.05));
    solidcircle(cx, cy, (int)(r * 2.4));
    setfillcolor(BlendColor(halo, kBg, 1.0 - fade * 0.10));
    solidcircle(cx, cy, (int)(r * 1.8));
    setfillcolor(BlendColor(halo, kBg, 1.0 - fade * 0.18));
    solidcircle(cx, cy, (int)(r * 1.35));

    DrawPetalRing(cx, cy, r, 12, tiltDeg, 0.68, 0.075,
                  BlendColor(kBg, RGB(200, 150, 50), fade));
    DrawPetalRing(cx, cy, r, 8, tiltDeg + 15.0, 0.46, 0.065,
                  BlendColor(kBg, RGB(242, 202, 112), fade * 0.96));

    setfillcolor(BlendColor(kBg, RGB(150, 105, 35), fade));
    solidcircle(cx, cy, (int)(r * 0.17));
    setfillcolor(BlendColor(kBg, RGB(246, 214, 140), fade));
    solidcircle(cx - r / 14, cy - r / 14, (int)(r * 0.07));
}

void DrawChrysanthemumCluster(int cx, int cy, int r, double fade, double spin) {
    DrawChrysanthemum(cx, cy, r, 12.0 + spin, fade);
    DrawChrysanthemum(cx + (int)(r * 1.15), cy - (int)(r * 0.85), (int)(r * 0.58), 40.0 + spin, fade);
    DrawChrysanthemum(cx - (int)(r * 1.05), cy - (int)(r * 0.45), (int)(r * 0.46), 68.0 + spin, fade);
}

// ---- 孔明灯（天灯）：发光圆角矩形灯体 + 下方光晕 ----
// phase: 0 = 刚从底部升起，1 = 已飞出屏幕顶部
void DrawSkyLantern(int cx, int cy, int halfW, double alpha, COLORREF bg) {
    if (halfW < 4 || alpha <= 0.0) return;

    const int halfH = (int)(halfW * 1.30);

    // 下方光晕：几圈逐渐收紧的暖光
    for (int i = 3; i >= 1; --i) {
        const COLORREF g = BlendColor(bg, RGB(255, 168, 74), alpha * 0.055 * i);
        setfillcolor(g);
        setlinecolor(g);
        solidcircle(cx, cy + (int)(halfH * 0.75), (int)(halfW * (0.7 + 0.42 * i)));
    }

    // 灯体：圆角矩形纸罩
    const COLORREF body = BlendColor(bg, RGB(255, 198, 112), alpha * 0.92);
    setfillcolor(body);
    setlinecolor(BlendColor(bg, RGB(255, 228, 176), alpha * 0.85));
    setlinestyle(PS_SOLID, 1);
    solidroundrect(cx - halfW, cy - halfH, cx + halfW, cy + halfH,
                   (int)(halfW * 0.60), (int)(halfW * 0.60));

    // 灯内火光
    const COLORREF fire = BlendColor(bg, RGB(255, 244, 206), alpha * 0.80);
    setfillcolor(fire);
    setlinecolor(fire);
    solidcircle(cx, cy + (int)(halfH * 0.22), (int)(halfW * 0.34));

    // 底部灯口
    const COLORREF mouth = BlendColor(bg, RGB(146, 88, 38), alpha * 0.90);
    setfillcolor(mouth);
    setlinecolor(mouth);
    solidrectangle(cx - halfW / 3, cy + halfH - 2, cx + halfW / 3, cy + halfH + 4);
}

// ---- 光芒扫过：旋转的楔形光束 ----
void DrawLightSweep(double t, double alpha, int cx, int cy, int R) {
    if (alpha <= 0.0) return;

    const double a0   = t * 1.7;
    const double span = 0.32;
    const int    steps = 12;

    POINT wedge[steps + 2];
    wedge[0].x = cx;
    wedge[0].y = cy;
    for (int i = 0; i <= steps; ++i) {
        const double a = a0 + span * (double)i / steps;
        wedge[i + 1].x = (LONG)(cx + std::cos(a) * R * 1.5);
        wedge[i + 1].y = (LONG)(cy + std::sin(a) * R * 1.5);
    }
    setfillcolor(BlendColor(kBg, RGB(255, 228, 156), alpha * 0.055));
    setlinecolor(BlendColor(kBg, RGB(255, 228, 156), alpha * 0.055));
    fillpolygon(wedge, steps + 2);
}

// ---- 火焰点燃：扩散光环 ----
void DrawIgniteRings(double p, int cx, int cy, int R) {
    for (int i = 0; i < 3; ++i) {
        const double q = p - i * 0.14;
        if (q <= 0.0 || q >= 1.0) continue;
        const double e = 1.0 - (1.0 - q) * (1.0 - q) * (1.0 - q);
        const int    r = (int)(R * 0.15 + R * 1.15 * e);
        const double a = (1.0 - q) * 0.40;
        setlinecolor(BlendColor(kBg, RGB(255, 196, 92), a));
        setlinestyle(PS_SOLID, 2 + i);
        circle(cx, cy, r);
    }
}

} // namespace

// 背景音乐已由 core/Audio 统一处理（winmm / mciSendString），
// 在 main() 中 Init + PlayBGM 循环播放，退出时 Shutdown，这里无需再处理。

void OpeningScene::InitLanterns(int count) {
    m_lanterns.clear();
    m_lanterns.reserve((size_t)count);
    for (int i = 0; i < count; ++i) {
        SkyLantern l;
        l.x0        = (float)(rand() % 1000) / 1000.0f;
        l.period    = 13.0f + (float)(rand() % 100) / 100.0f * 9.0f;   // 13 ~ 22 秒
        l.phase     = (float)(rand() % 1000) / 1000.0f;                // 随机初始高度
        l.size      = 9.0f + (float)(rand() % 18);                     // 9 ~ 26 像素半宽
        l.swayPhase = (float)(rand() % 628) / 100.0f;
        m_lanterns.push_back(l);
    }
}

OpeningScene::OpeningScene() {
    startTick = GetTickCount();
    elapsed   = 0.0;

    InitLanterns(18);

    int W = getwidth();
    int H = getheight();
    if (W <= 0) W = 1920;
    if (H <= 0) H = 1080;
    fireflies.Init(30, W, (int)(H * 0.85));
}

double OpeningScene::EaseOutCubic(double t) {
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    const double u = 1.0 - t;
    return 1.0 - u * u * u;
}

double OpeningScene::Progress(double elapsed, double start, double duration) {
    if (elapsed <= start) return 0.0;
    if (elapsed >= start + duration) return 1.0;
    return (elapsed - start) / duration;
}

GameState OpeningScene::Update() {
    elapsed = (GetTickCount() - startTick) / 1000.0;

    // 持续 6 秒后自动进入主菜单
    if (elapsed >= kTotalTime) {
        return STATE_MENU;
    }

    // 1.5 秒强制防抖：
    // 程序刚启动时，玩家很可能还按着回车（比如启动前敲的回车），
    // 若不等待，会瞬间跳过开场并连带误触主菜单。
    if (elapsed >= kSkipDelay) {
        if (Input::IsKeyPressed(VK_RETURN) || Input::IsKeyPressed(VK_ESCAPE)) {
            return STATE_MENU;
        }
    }

    return STATE_OPENING;
}

void OpeningScene::Render() {
    // 开场动画使用全屏坐标系
    setorigin(0, 0);

    const int W = getwidth();
    const int H = getheight();
    if (W <= 0 || H <= 0) return;

    const int cx = W / 2;
    const int cy = H / 2;

    const double pTitle  = Progress(elapsed, kTitleStart, kTitleDur);
    const double pSub    = Progress(elapsed, kSubStart,   kSubDur);
    const double pTip    = Progress(elapsed, kTipStart,   kTipDur);
    const double pBack   = Progress(elapsed, 0.0, kBackFadeDur);
    const double pIgnite = Progress(elapsed, kTitleStart, kIgniteDur);

    const double eTitle = EaseOutCubic(pTitle);
    const double eSub   = EaseOutCubic(pSub);

    // 孔明灯的整体透明度：淡入，标题点火后略微退场让位
    double pLantern = Progress(elapsed, kLanternStart, kLanternDur);
    pLantern *= (1.0 - 0.45 * pTitle);

    const int sceneR = (int)((W < H ? W : H) * 0.34);

    setbkmode(TRANSPARENT);   // 文字透明背景，避免出现底色方块

    // ---- 夜空：星光 ----
    DrawStarField(W, H, elapsed, 80);

    // ---- 光芒扫过 ----
    if (pLantern > 0.05) DrawLightSweep(elapsed, pLantern, cx, cy, sceneR);

    // ---- 山水：六层远山 + 云海 ----
    DrawInkMountains(pBack, elapsed);

    // ---- 满天孔明灯：从底部缓缓升向顶部 ----
    if (pLantern > 0.02) {
        const double travel = H + 240.0;          // 从屏幕下方到飞出顶部的总行程
        for (const auto& l : m_lanterns) {
            // 上升进度 0..1（循环）
            double p = std::fmod(elapsed / l.period + l.phase, 1.0);
            if (p < 0.0) p += 1.0;

            // 两端淡入淡出，避免突然出现 / 消失
            double a = pLantern;
            if (p < 0.10) a *= p / 0.10;
            else if (p > 0.90) a *= (1.0 - p) / 0.10;
            if (a <= 0.01) continue;

            const int px = (int)(W * (0.05 + 0.90 * l.x0)
                               + std::sin(elapsed * 0.45 + l.swayPhase) * 22.0);
            const int py = (int)(H + 120.0 - p * travel);

            DrawSkyLantern(px, py, (int)l.size, a, kBg);
        }
    }

    // ---- 萤火虫 ----
    if (pBack > 0.15) fireflies.Draw(elapsed);

    // ---- 菊花簇：左下 / 右下各一簇（一大两小），右上方补一朵 ----
    const int flowerR = (int)((W < H ? W : H) * 0.085);
    if (pBack > 0.0) {
        DrawChrysanthemumCluster((int)(W * 0.11), (int)(H * 0.88), flowerR, pBack, 0.0);
        DrawChrysanthemumCluster((int)(W * 0.89), (int)(H * 0.91), (int)(flowerR * 0.85), pBack, 1.6);
        DrawChrysanthemum((int)(W * 0.88), (int)(H * 0.20),
                          (int)(flowerR * 0.55), 40.0, pBack * 0.9);
    }

    // ---- 灯笼：屏幕上方左右两侧，带发光 ----
    const int lanternR = (int)((W < H ? W : H) * 0.042);
    if (pBack > 0.0) {
        const double glow = 0.55 + 0.16 * std::sin(elapsed * 2.1);
        DrawLantern((int)(W * 0.10), 0, (int)(H * 0.09), lanternR, glow);
        DrawLantern((int)(W * 0.90), 0, (int)(H * 0.09), lanternR, glow);
    }

    // ---- 火焰点燃：扩散光环 ----
    if (pIgnite > 0.0 && pIgnite < 1.0) {
        DrawIgniteRings(pIgnite, cx, cy - 60, sceneR);
    }

    // ---- 阶段一：主标题“重 阳”（150 -> 100，颜色 黑 -> 金，火焰般浮现）----
    if (pTitle > 0.0) {
        const int      size  = (int)(150.0 - 50.0 * eTitle);
        const wchar_t* title = L"重 阳";

        settextstyle(size, 0, L"楷体");
        const int titleW = textwidth(title);
        const int titleH = textheight(title);
        const int tx = cx - titleW / 2;
        const int ty = cy - titleH - 24;

        // 点火瞬间用暖橙光晕，随后转为金色
        const COLORREF glowColor = (pIgnite < 1.0)
            ? BlendColor(RGB(255, 150, 60), kGold, pIgnite)
            : kGold;

        DrawGlowText(tx, ty, title,
                     BlendColor(RGB(12, 8, 20), kGold, pTitle),
                     glowColor, pTitle * (0.55 + 0.45 * pIgnite));
    }

    // ---- 阶段二：副标题淡入 + 轻微上浮 ----
    if (pSub > 0.0) {
        const wchar_t* sub = L"九九重阳 · 敬老爱老";
        settextstyle(36, 0, L"楷体");
        const int subW = textwidth(sub);
        const int subY = cy + 40 + (int)(28.0 * (1.0 - eSub));
        DrawGlowText(cx - subW / 2, subY, sub,
                     BlendColor(RGB(12, 8, 20), RGB(235, 225, 195), pSub),
                     RGB(235, 225, 195), pSub * 0.45);
    }

    // ---- 底部：跳过提示 ----
    if (pTip > 0.0) {
        const wchar_t* tip = L"按 [Enter] 跳过开场";
        settextstyle(18, 0, L"微软雅黑");
        const int tipW = textwidth(tip);
        settextcolor(BlendColor(RGB(12, 8, 20), RGB(160, 160, 160), pTip));
        outtextxy(cx - tipW / 2, H - 90, tip);
    }
}
