#include "InteractiveScene.h"
#include "../core/Input.h"
#include "../core/Config.h"

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr int    kMaxLanterns = 15;      // 同时存在的孔明灯上限
constexpr double kPI          = 3.14159265358979323846;

// 祝福语（随机贴在灯身上方）
const wchar_t* kWishes[] = {
    L"平安喜乐", L"岁岁安康", L"阖家幸福", L"万事顺遂",
    L"福寿绵长", L"诸事如意", L"长风破浪", L"所愿皆成",
    L"山河无恙", L"人间皆安"
};
constexpr int kWishCount = (int)(sizeof(kWishes) / sizeof(kWishes[0]));

// ---- 夜空里的一轮明月：纯白微黄实心圆 + 一层极淡柔光 ----
// （刻意不做同心圆光环，月亮就是一个干净的圆）
void DrawMoon(int cx, int cy, int r, COLORREF bg) {
    // 一层非常淡的柔光：单个大半径低透明度圆
    const int       gr = (int)(r * 2.4);
    const COLORREF g  = BlendColor(bg, RGB(250, 246, 222), 0.060);
    setfillcolor(g);
    setlinecolor(g);
    solidcircle(cx, cy, gr);

    // 月面：纯白微黄
    setfillcolor(RGB(252, 248, 228));
    setlinecolor(RGB(252, 248, 228));
    solidcircle(cx, cy, r);
}

// ---- 一层水墨远山（山脊线用多个正弦叠加，seed 决定形状）----
void DrawMountainRange(int W, int H, double baseY, double amp, int peaks,
                       double seed, COLORREF color) {
    POINT p[80];
    int   n = 0;

    p[n].x = -30;        p[n].y = H + 30;        ++n;
    const int steps = peaks * 5;
    for (int i = 0; i <= steps && n < 78; ++i) {
        const double u = (double)i / (double)steps;
        const double y = baseY
                       - std::sin(u * kPI * peaks + seed) * amp
                       - std::sin(u * kPI * peaks * 2.3 + seed * 1.7) * amp * 0.34
                       - std::sin(u * kPI * peaks * 4.1 + seed * 2.9) * amp * 0.12;
        p[n].x = (LONG)(-30 + u * (W + 60));
        p[n].y = (LONG)y;
        ++n;
    }
    p[n].x = W + 30;     p[n].y = H + 30;        ++n;

    setfillcolor(color);
    setlinecolor(color);
    fillpolygon(p, n);
}

// ---- 江面：一条微亮的横带 + 缓慢推移的波光 ----
void DrawWater(int W, int H, double t, COLORREF bg) {
    const int top = (int)(H * 0.845);
    if (top >= H) return;

    setfillcolor(BlendColor(bg, RGB(16, 14, 34), 0.95));
    setlinecolor(BlendColor(bg, RGB(16, 14, 34), 0.95));
    solidrectangle(0, top, W, H);

    // 岸线
    setlinecolor(BlendColor(bg, RGB(198, 160, 78), 0.30));
    setlinestyle(PS_SOLID, 2);
    line(0, top, W, top);

    // 波光：稀疏的横向亮线，随时间明灭
    for (int i = 0; i < 26; ++i) {
        const double ph   = t * (0.35 + (i % 5) * 0.08) + i * 1.7;
        const double tw   = 0.5 + 0.5 * std::sin(ph);
        const double a    = 0.05 + 0.16 * tw;
        const int    y    = top + 6 + (int)((double)(H - top - 12) * ((i * 37) % 100) / 100.0);
        const int    len  = 40 + (i % 7) * 26;
        const int    x    = (int)((double)W * ((i * 53) % 100) / 100.0)
                          + (int)(std::sin(ph * 0.6) * 30.0);
        setlinecolor(BlendColor(bg, RGB(232, 220, 190), a));
        setlinestyle(PS_SOLID, 1);
        line(x, y, x + len, y);
    }
}

} // namespace

// ---------------------------------------------------------------------------
//  构造 / 析构
// ---------------------------------------------------------------------------
InteractiveScene::InteractiveScene() {
    m_lastTick = GetTickCount();
    printf("[Log] 进入互动广场：孔明灯祈福\n");
}

// ---------------------------------------------------------------------------
//  逻辑更新
// ---------------------------------------------------------------------------
GameState InteractiveScene::Update() {
    const DWORD now = GetTickCount();

    double dt = (now - m_lastTick) / 1000.0;
    if (dt < 0.0)   dt = 0.0;
    if (dt > 0.1)   dt = 0.1;      // 切后台回来时不跳变
    m_lastTick = now;
    m_time += dt;

    // ---- ESC 返回主菜单 ----
    if (Input::IsKeyPressed(VK_ESCAPE)) {
        return STATE_MENU;
    }

    // ---- 空格放飞一盏孔明灯 ----
    if (Input::IsKeyPressed(VK_SPACE)) {
        if ((int)m_lanterns.size() < kMaxLanterns) {
            SpawnLantern();
        } else {
            m_fullHint = true;     // 已达上限：给出 2 秒提示
            m_fullTick = now;
        }
    }

    // ---- 逐盏更新：上升 + 正弦摇摆 ----
    for (auto& L : m_lanterns) {
        L.age += (float)dt;
        L.y   -= L.speed * (float)dt;

        // 水平位置 = 基准位置 + 正弦摇摆偏移
        const float sway = std::sin(L.age * L.swayFreq * 6.2831853f + L.phase) * L.swayAmp;
        L.x = L.baseX + sway;
    }

    // ---- 飞出屏幕顶部即移除 ----
    for (auto it = m_lanterns.begin(); it != m_lanterns.end(); ) {
        if (it->y < -(float)(it->size) - 140.0f) {
            it = m_lanterns.erase(it);
        } else {
            ++it;
        }
    }

    return STATE_INTERACTIVE;
}

// ---------------------------------------------------------------------------
//  放飞一盏灯
// ---------------------------------------------------------------------------
void InteractiveScene::SpawnLantern() {
    const int W = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int H = (SCREEN_H > 0 ? SCREEN_H : WIN_H);

    Lantern L;

    // ---- x 尽量分散：随机取若干候选，挑一个离现有灯最远的 ----
    const int margin = (int)(W * 0.10);
    const int span   = (W - margin * 2) > 1 ? (W - margin * 2) : 1;
    float bestX = (float)(margin + rand() % span);
    float bestD = -1.0f;
    for (int tries = 0; tries < 8; ++tries) {
        const float cand = (float)(margin + rand() % span);
        float minD = 1e9f;
        for (const auto& o : m_lanterns) {
            const float d = std::fabs(o.baseX - cand);
            if (d < minD) minD = d;
        }
        if (minD > bestD) { bestD = minD; bestX = cand; }
        if (minD > (float)(W * 0.16)) break;      // 已经足够分散，不用再试
    }
    L.baseX    = bestX;
    L.x        = L.baseX;

    // ---- y 从屏幕完全外面起飞：SCREEN_H + 100 ~ SCREEN_H + 200 ----
    // （offset 取 100 以上，保证 size 最大时灯顶也在屏幕下方，不会被切掉半个身子）
    L.y        = (float)H + 100.0f + (float)(rand() % 101);

    L.speed    = 26.0f + (float)(rand() % 26);     // 26 ~ 52 px/s
    L.swayAmp  = 14.0f + (float)(rand() % 22);     // 14 ~ 36 px
    L.swayFreq = 0.45f + (float)(rand() % 40) / 100.0f;   // 0.45 ~ 0.85 Hz
    L.phase    = (float)(rand() % 628) / 100.0f;
    L.size     = 46.0f + (float)(rand() % 42);     // 46 ~ 88 px（约为原来的 2.3 倍）
    L.glow     = 0.85f + (float)(rand() % 30) / 100.0f;
    L.age      = 0.0f;
    L.wish     = rand() % kWishCount;

    m_lanterns.push_back(L);
}

// ---------------------------------------------------------------------------
//  绘制一盏孔明灯（最简版：暖黄圆角矩形灯罩 + 棕色底座 + 橙色火光）
//  不用任何多层实心圆光晕，只用一层极淡的透明圆保持背景干净。
// ---------------------------------------------------------------------------
void InteractiveScene::DrawLanternBody(const Lantern& L) const {
    const COLORREF bg = NightBg();

    const int cx = (int)L.x;
    const int cy = (int)L.y;
    if (cx < -300 || cx > (SCREEN_W > 0 ? SCREEN_W : WIN_W) + 300) return;

    // 呼吸：只用来微调光晕透明度和火光大小
    const double pulse = 0.80 + 0.20 * std::sin(L.age * 2.3 + L.phase);

    const int hw = (int)(L.size * 0.60);    // 灯罩半宽
    const int hh = (int)(L.size * 0.78);    // 灯罩半高
    const int rad = (int)(hw * 0.75);       // 圆角

    // ---- 光晕：单独一层极淡的黄色透明圆（背景干净，不发褐）----
    const COLORREF glow = BlendColor(bg, RGB(255, 205, 110), 0.055 * pulse);
    setfillcolor(glow);
    setlinecolor(glow);
    solidcircle(cx, cy, (int)(L.size * 1.7 * pulse));

    // ---- 灯罩：暖黄纯色圆角矩形 ----
    setfillcolor(RGB(255, 200, 100));
    setlinecolor(RGB(196, 144, 62));
    setlinestyle(PS_SOLID, 2);
    solidroundrect(cx - hw, cy - hh, cx + hw, cy + hh, rad, rad);
    roundrect(cx - hw, cy - hh, cx + hw, cy + hh, rad, rad);

    // ---- 灯罩中心：橙色实心小圆（火光）----
    const int fr = (int)(hw * 0.34 * pulse);
    setfillcolor(RGB(255, 140, 40));
    setlinecolor(RGB(255, 170, 70));
    setlinestyle(PS_SOLID, 1);
    solidcircle(cx, cy, fr > 3 ? fr : 3);

    // ---- 底座：棕色小矩形 ----
    const int bw  = (int)(hw * 0.55);
    const int bh  = (int)(L.size * 0.16);
    setfillcolor(RGB(120, 78, 38));
    setlinecolor(RGB(158, 106, 52));
    setlinestyle(PS_SOLID, 1);
    solidrectangle(cx - bw, cy + hh, cx + bw, cy + hh + bh);
    rectangle(cx - bw, cy + hh, cx + bw, cy + hh + bh);

    // ---- 灯身上方的祝福语 ----
    settextstyle((int)(13 + L.size * 0.22), 0, L"楷体");
    settextcolor(BlendColor(bg, RGB(255, 226, 158), 0.55 * pulse + 0.25));
    const wchar_t* wish = kWishes[L.wish];
    outtextxy(cx - textwidth(wish) / 2, cy - hh - (int)(L.size * 0.95), wish);
}

// ---------------------------------------------------------------------------
//  渲染
// ---------------------------------------------------------------------------
void InteractiveScene::Render() {
    // 全屏绝对坐标：不经过设计区平移
    setorigin(0, 0);

    const COLORREF bg = NightBg();
    const int W = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int H = (SCREEN_H > 0 ? SCREEN_H : WIN_H);
    const double t = m_time;

    // ---- 夜空与繁星 ----
    DrawStarField(W, H, t, 130, bg);

    // ---- 明月（右上）----
    DrawMoon((int)(W * 0.82), (int)(H * 0.17), (int)(H * 0.045), bg);

    // ---- 两层水墨远山 ----
    DrawMountainRange(W, H, H * 0.80, H * 0.10, 3, 1.3,
                      BlendColor(bg, RGB(30, 24, 46), 0.94));
    DrawMountainRange(W, H, H * 0.87, H * 0.07, 4, 3.7,
                      BlendColor(bg, RGB(18, 14, 30), 0.96));

    // ---- 江面 ----
    DrawWater(W, H, t, bg);

    // ---- 孔明灯（先画小的，大的后画，形成层次）----
    std::vector<const Lantern*> order;
    order.reserve(m_lanterns.size());
    for (const auto& L : m_lanterns) order.push_back(&L);
    for (size_t i = 0; i < order.size(); ++i) {
        for (size_t j = i + 1; j < order.size(); ++j) {
            if (order[j]->size < order[i]->size) std::swap(order[i], order[j]);
        }
    }
    for (const Lantern* L : order) DrawLanternBody(*L);

    // =====================================================================
    //  文字层
    // =====================================================================

    // ---- 顶部正中：金色大字标题 ----
    settextstyle((int)(H * 0.048), 0, L"楷体");
    const wchar_t* title = L"重阳祈福 · 孔明灯";
    DrawGlowText(W / 2 - textwidth(title) / 2, (int)(H * 0.055),
                 title, RGB(255, 238, 180), RGB(255, 215, 0), 0.55, bg);

    // ---- 标题下方一行小字 ----
    settextstyle((int)(H * 0.020), 0, L"楷体");
    settextcolor(BlendColor(bg, RGB(214, 200, 168), 0.80));
    const wchar_t* sub = L"灯载心愿，上达天听";
    outtextxy(W / 2 - textwidth(sub) / 2, (int)(H * 0.125), sub);

    // ---- 右上角：已放飞数量 ----
    wchar_t cnt[64];
    swprintf(cnt, 64, L"已放飞 %d / %d 盏", (int)m_lanterns.size(), kMaxLanterns);
    settextstyle((int)(H * 0.020), 0, L"微软雅黑");
    settextcolor(BlendColor(bg, RGB(226, 210, 170), 0.78));
    outtextxy(W - (int)(W * 0.03) - textwidth(cnt), (int)(H * 0.06), cnt);

    // ---- 达到上限时的临时提示 ----
    if (m_fullHint && GetTickCount() - m_fullTick < 2000) {
        const wchar_t* full = L"天上灯已满，稍候再放";
        settextstyle((int)(H * 0.026), 0, L"楷体");
        settextcolor(BlendColor(bg, RGB(255, 190, 130), 0.92));
        outtextxy(W / 2 - textwidth(full) / 2, (int)(H * 0.175), full);
    } else if (m_fullHint) {
        m_fullHint = false;
    }

    // ---- 底部提示 ----
    settextstyle((int)(H * 0.022), 0, L"微软雅黑");
    settextcolor(BlendColor(bg, RGB(206, 192, 160), 0.88));
    const wchar_t* hint = L"按 [空格] 放飞一盏祈福灯，按 [ESC] 返回主菜单";
    outtextxy(W / 2 - textwidth(hint) / 2, H - (int)(H * 0.062), hint);
}
