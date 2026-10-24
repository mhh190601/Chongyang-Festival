#include "MenuScene.h"
#include "../core/Input.h"
#include "../core/DrawUtil.h"

#include <cstdio>
#include <cmath>
#include <windows.h>

namespace {

constexpr double kPI = 3.14159265358979323846;

// ---- 一片梭形竹叶（叶尖朝 angDeg 方向）----
void DrawBambooLeaf(int cx, int cy, double angDeg, int len, COLORREF c) {
    const double a  = angDeg * kPI / 180.0;
    const double nx = -std::sin(a), ny = std::cos(a);   // 叶面法向

    POINT p[4];
    p[0].x = (LONG)(cx + std::cos(a) * len);          p[0].y = (LONG)(cy + std::sin(a) * len);           // 叶尖
    p[1].x = (LONG)(cx + nx * len * 0.24);            p[1].y = (LONG)(cy + ny * len * 0.24);             // 右侧
    p[2].x = (LONG)(cx - std::cos(a) * len * 0.30);   p[2].y = (LONG)(cy - std::sin(a) * len * 0.30);     // 叶柄
    p[3].x = (LONG)(cx - nx * len * 0.24);            p[3].y = (LONG)(cy - ny * len * 0.24);             // 左侧

    setfillcolor(c);
    setlinecolor(c);
    fillpolygon(p, 4);
}

// ---- 一朵梅花（5 瓣 + 金黄花心）----
void DrawPlumBlossom(int cx, int cy, int r, COLORREF bg) {
    if (r < 2) return;
    const COLORREF petal = BlendColor(bg, RGB(216, 122, 132), 0.92);
    const COLORREF heart = BlendColor(bg, RGB(238, 200, 110), 0.95);

    for (int i = 0; i < 5; ++i) {
        const double a = i * 72.0 * kPI / 180.0;
        setfillcolor(petal);
        setlinecolor(petal);
        solidcircle(cx + (int)(std::cos(a) * r * 0.58), cy + (int)(std::sin(a) * r * 0.58),
                    (int)(r * 0.44));
    }
    setfillcolor(heart);
    setlinecolor(heart);
    solidcircle(cx, cy, (int)(r * 0.30));
}

// ---- 左上角：水墨祥云笔触 + 几片竹叶 ----
void DrawCornerCloudBamboo(int cx, int cy, double t, COLORREF bg) {
    // 祥云：三道横向起伏笔触
    setlinestyle(PS_SOLID, 3);
    for (int i = 0; i < 3; ++i) {
        POINT arc[16];
        for (int k = 0; k < 16; ++k) {
            const double u = (double)k / 15.0;
            arc[k].x = (LONG)(cx - 78 + u * 160);
            arc[k].y = (LONG)(cy + i * 15 + std::sin(u * kPI + i * 0.8) * 9.0);
        }
        setlinecolor(BlendColor(bg, RGB(214, 206, 224), 0.15 - i * 0.03));
        polyline(arc, 16);
    }

    // 竹叶：缓慢摆动
    const COLORREF leaf = BlendColor(bg, RGB(122, 140, 90), 0.52);
    for (int i = 0; i < 4; ++i) {
        const double ang = -78.0 + i * 30.0 + std::sin(t * 0.8 + i * 0.9) * 5.0;
        DrawBambooLeaf(cx - 40 + i * 30, cy + 40 + (i % 2) * 20, ang, 30 + (i % 3) * 8, leaf);
    }
}

// ---- 右上角：探出来的梅枝 + 梅花 ----
void DrawCornerPlum(int cx, int cy, double t, COLORREF bg) {
    const COLORREF branch = BlendColor(bg, RGB(94, 64, 44), 0.92);

    // 主枝：从右侧探入，向左下伸展
    POINT br[12];
    for (int i = 0; i < 12; ++i) {
        const double u = (double)i / 11.0;
        br[i].x = (LONG)(cx + 96 - u * 150 + std::sin(u * 5.0) * 9.0);
        br[i].y = (LONG)(cy + 12 + u * 96 + std::sin(u * 3.5) * 7.0);
    }
    setlinecolor(branch);
    setlinestyle(PS_SOLID, 4);
    polyline(br, 12);

    // 两条小枝
    POINT twig1[5] = { br[5], { br[5].x - 30, br[5].y - 26 }, { br[5].x - 52, br[5].y - 18 },
                       { br[5].x - 62, br[5].y - 34 }, { br[5].x - 70, br[5].y - 30 } };
    setlinestyle(PS_SOLID, 2);
    polyline(twig1, 5);
    POINT twig2[5] = { br[8], { br[8].x - 26, br[8].y + 20 }, { br[8].x - 46, br[8].y + 30 },
                       { br[8].x - 58, br[8].y + 22 }, { br[8].x - 66, br[8].y + 28 } };
    polyline(twig2, 5);

    // 梅花：开在主枝与小枝上（轻微摇摆）
    const POINT buds[5] = { br[3], br[6], br[9], twig1[3], twig2[3] };
    for (int i = 0; i < 5; ++i) {
        const int bx = buds[i].x + (int)(std::sin(t * 0.9 + i) * 2.0);
        const int by = buds[i].y + (int)(std::cos(t * 0.9 + i) * 2.0);
        DrawPlumBlossom(bx, by - 14, 8 + (i % 2) * 3, bg);
    }

    // 梅枝周围飘落的花瓣
    for (int i = 0; i < 6; ++i) {
        const double p = std::fmod(t * (0.08 + 0.025 * (i % 3)) + i * 0.43, 1.0);
        const int px = cx + 60 + (int)(std::sin(t * 0.7 + i * 1.3) * 40.0);
        const int py = cy + 10 + (int)(p * 110.0);
        double a = 0.9;
        if (p < 0.12) a = 0.9 * p / 0.12;
        else if (p > 0.85) a = 0.9 * (1.0 - p) / 0.15;
        if (a <= 0.02) continue;
        const int w = 4 + (i % 3);
        const COLORREF c = BlendColor(bg, RGB(214, 122, 132), a);
        setfillcolor(c);
        setlinecolor(c);
        solidellipse(px - w, py - (int)(w * 0.7), px + w, py + (int)(w * 0.7));
    }
}

// ---- 一缕向上飘散的水汽 / 轻烟 ----
void DrawSmoke(int x, int y, double t, COLORREF bg, int rise = 96) {
    for (int i = 0; i < 3; ++i) {
        const double phase = t * 0.85 + i * 1.8;
        for (int k = 0; k <= 14; ++k) {
            const double u = (double)k / 14.0;
            const int px = x + (int)(std::sin(phase + u * 3.4) * 15.0 * u);
            const int py = y - (int)(u * rise);
            const double a = 0.19 * (1.0 - u * 0.85) * (0.55 + 0.45 * std::sin(phase * 2.0 + u * 4.0));
            if (a <= 0.005) continue;
            const COLORREF c = BlendColor(bg, RGB(234, 232, 238), a);
            setfillcolor(c);
            setlinecolor(c);
            solidcircle(px, py, (int)(2 + 5 * u));
        }
    }
}

// ---- 左下角：一尊小香炉 + 一缕轻烟 ----
void DrawCornerIncenseBurner(int cx, int cy, double t, COLORREF bg) {
    const COLORREF bronze = BlendColor(bg, RGB(126, 92, 46), 0.92);
    const COLORREF rim    = BlendColor(bg, RGB(200, 162, 94), 0.82);

    // 炉身（梯形）
    POINT body[4] = {
        { cx - 40, cy - 18 }, { cx + 40, cy - 18 },
        { cx + 28, cy + 20 },  { cx - 28, cy + 20 }
    };
    setfillcolor(bronze);
    setlinecolor(rim);
    setlinestyle(PS_SOLID, 2);
    fillpolygon(body, 4);

    // 炉口沿
    setfillcolor(bronze);
    setlinecolor(rim);
    solidellipse(cx - 42, cy - 28, cx + 42, cy - 14);

    // 双耳
    setfillcolor(bronze);
    setlinecolor(rim);
    solidcircle(cx - 46, cy - 6, 7);
    solidcircle(cx + 46, cy - 6, 7);

    // 三足
    setfillcolor(bronze);
    setlinecolor(rim);
    for (int i = -1; i <= 1; ++i) {
        solidrectangle(cx + i * 22 - 4, cy + 20, cx + i * 22 + 4, cy + 36);
    }

    // 轻烟
    DrawSmoke(cx, cy - 30, t, bg, 84);
}

// ---- 右下角：几卷堆叠的竹简 ----
void DrawCornerBambooSlips(int cx, int cy, COLORREF bg) {
    const COLORREF bamboo = BlendColor(bg, RGB(208, 184, 130), 0.86);
    const COLORREF bind   = BlendColor(bg, RGB(120, 84, 40), 0.92);

    for (int i = 0; i < 4; ++i) {
        const int y = cy - i * 17;
        setfillcolor(bamboo);
        setlinecolor(bind);
        setlinestyle(PS_SOLID, 2);
        solidroundrect(cx - 76, y - 12, cx + 76, y + 12, 5, 5);
        roundrect(cx - 76, y - 12, cx + 76, y + 12, 5, 5);

        // 两道绑绳
        setlinestyle(PS_SOLID, 2);
        line(cx - 42, y - 12, cx - 42, y + 12);
        line(cx + 42, y - 12, cx + 42, y + 12);

        // 简上刻字痕
        setlinecolor(BlendColor(bg, RGB(140, 108, 60), 0.55));
        setlinestyle(PS_SOLID, 1);
        for (int k = 0; k < 6; ++k) {
            line(cx - 28 + k * 12, y - 4, cx - 24 + k * 12, y - 4);
        }
    }
}

// ---- 展开的半透明折扇（tiltDeg 为整体摆角，可做扇动动画）----
void DrawFoldingFan(int cx, int cy, int r, double tiltDeg, COLORREF bg) {
    if (r < 10) return;

    const double a0 = -112.0;
    const double a1 = -18.0;
    const int    steps = 26;

    auto ptAt = [&](double angDeg, double rad) -> POINT {
        const double a = (angDeg + tiltDeg) * kPI / 180.0;
        POINT p;
        p.x = (LONG)(cx + std::cos(a) * rad);
        p.y = (LONG)(cy + std::sin(a) * rad);
        return p;
    };

    POINT fan[steps + 2];
    fan[0] = ptAt(0.0, 0.0);
    for (int i = 0; i <= steps; ++i) fan[i + 1] = ptAt(a0 + (a1 - a0) * i / steps, (double)r);
    setfillcolor(BlendColor(bg, RGB(228, 216, 188), 0.40));
    setlinecolor(BlendColor(bg, RGB(228, 216, 188), 0.40));
    fillpolygon(fan, steps + 2);

    POINT arc[steps + 1];
    for (int i = 0; i <= steps; ++i) arc[i] = ptAt(a0 + (a1 - a0) * i / steps, (double)r);
    setlinecolor(BlendColor(bg, RGB(198, 160, 78), 0.80));
    setlinestyle(PS_SOLID, 2);
    polyline(arc, steps + 1);

    setlinecolor(BlendColor(bg, RGB(198, 160, 78), 0.85));
    setlinestyle(PS_SOLID, 1);
    for (int i = 0; i <= steps; i += 3) {
        const POINT p = ptAt(a0 + (a1 - a0) * i / steps, (double)r);
        line(cx, cy, p.x, p.y);
    }

    setfillcolor(BlendColor(bg, RGB(150, 105, 35), 0.95));
    setlinecolor(BlendColor(bg, RGB(150, 105, 35), 0.95));
    solidcircle(cx, cy, 4);
}

// ---- 古风酒壶 ----
void DrawWinePot(int cx, int cy, COLORREF bg) {
    const int bw = 50;
    const int bh = 38;
    const COLORREF body = BlendColor(bg, RGB(150, 108, 52), 0.92);
    const COLORREF rim  = BlendColor(bg, RGB(226, 190, 110), 0.85);

    POINT handle[13];
    for (int i = 0; i < 13; ++i) {
        const double a = -1.15 + i * 2.3 / 12.0;
        handle[i].x = (LONG)(cx + bw + 4 + std::cos(a) * 24);
        handle[i].y = (LONG)(cy - 4 + std::sin(a) * 32);
    }
    setlinecolor(rim);
    setlinestyle(PS_SOLID, 4);
    polyline(handle, 13);

    POINT spout[3] = {
        { cx - bw + 6,  cy - 10 },
        { cx - bw - 28, cy - 30 },
        { cx - bw + 14, cy + 10 }
    };
    setfillcolor(body);
    setlinecolor(rim);
    setlinestyle(PS_SOLID, 2);
    fillpolygon(spout, 3);

    setfillcolor(body);
    setlinecolor(rim);
    solidellipse(cx - bw, cy - bh, cx + bw, cy + bh);

    setfillcolor(BlendColor(bg, RGB(255, 236, 190), 0.16));
    setlinecolor(BlendColor(bg, RGB(255, 236, 190), 0.16));
    solidellipse(cx - bw + 10, cy - bh + 8, cx - bw + 26, cy + bh - 12);

    POINT neck[4] = {
        { cx - 18, cy - bh + 8 }, { cx + 18, cy - bh + 8 },
        { cx + 12, cy - bh - 32 }, { cx - 12, cy - bh - 32 }
    };
    setfillcolor(body);
    setlinecolor(rim);
    fillpolygon(neck, 4);

    setfillcolor(body);
    setlinecolor(rim);
    solidellipse(cx - 17, cy - bh - 40, cx + 17, cy - bh - 28);
    solidcircle(cx, cy - bh - 44, 5);
}

} // namespace

MenuScene::MenuScene() {
    currentSelection = 0;
    menuItems = {
        L"[1] 祝福语滚动",
        L"[2] 测卦象",
        L"[3] 缅怀先烈",
        L"[4] 互动广场",
        L"[5] 退出"
    };
    fireflies.Init(30, SCREEN_W, SCREEN_H);   // 萤火虫覆盖整个物理屏幕
}

GameState MenuScene::Update() {
    // =====================================================================
    //  让出前台期间（正在浏览中华英烈网）
    //  图形窗口已被 ShowWindow(SW_MINIMIZE) 最小化，浏览器绝对在最前面。
    //
    //  恢复条件只有两个，绝不响应其它按键：
    //    1) 用户自己把窗口还原了（点任务栏 / Alt+Tab）—— IsIconic 变 FALSE
    //    2) 只按 ESC 键
    //  这样玩家在浏览器地址栏、搜索框里打字（回车、字母、空格）时，
    //  不会被全局按键轮询强行拉回本程序。
    // =====================================================================
    if (IsTopmostYielded()) {
        const HWND  hWnd = GetHWnd();
        const DWORD held = GetTickCount() - yieldTick;

        // ---- 条件一：用户自行还原了窗口（任务栏点击 / Alt+Tab）----
        if (hWnd && !IsIconic(hWnd)) {
            RestoreTopmost();
            Input::Flush();
            return STATE_MENU;
        }

        // ---- 条件二：只认 ESC，且最小化满 1 秒（避开触发打开的那一下按键）----
        if (held > 1000 && Input::IsKeyPressed(VK_ESCAPE)) {
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            RestoreTopmost();
            Input::Flush();   // 吞掉这次 ESC，避免恢复后又被当成菜单操作
        }
        return STATE_MENU;
    }

    if (Input::IsKeyPressed(VK_UP)) {
        currentSelection--;
        if (currentSelection < 0) currentSelection = (int)menuItems.size() - 1;
    }
    else if (Input::IsKeyPressed(VK_DOWN)) {
        currentSelection++;
        if (currentSelection >= (int)menuItems.size()) currentSelection = 0;
    }
    else if (Input::IsKeyPressed(VK_RETURN)) {
        if (currentSelection == 0) return STATE_BLESS;
        if (currentSelection == 1) return STATE_DIVINATION;

        // ---- [3] 缅怀先烈：打开中华英烈网 ----
        if (currentSelection == 2) {
            YieldTopmost();                            // 先取消置顶
            ShowWindow(GetHWnd(), SW_MINIMIZE);        // 再最小化，浏览器必然在前台
            yieldTick = GetTickCount();
            ShellExecuteW(NULL, L"open", L"https://www.chinamartyrs.gov.cn/",
                          NULL, NULL, SW_SHOWNORMAL);
            // 窗口保持最小化，由上面的分支在用户按任意键时恢复。
            return STATE_MENU;
        }

        // ---- [4] 互动广场：孔明灯祈福 ----
        if (currentSelection == 3) return STATE_INTERACTIVE;

        if (currentSelection == 4) return STATE_EXIT;
    }
    return STATE_MENU;
}

void MenuScene::Render() {
    const COLORREF bg    = NightBg();
    const COLORREF kGold = RGB(255, 215, 0);

    const double t = GetTickCount() / 1000.0;

    // =====================================================================
    //  第一层：全屏背景（物理屏幕绝对坐标）
    //  主循环在调用 Render 前把原点平到了设计区中心，所以这里必须先
    //  setorigin(0,0) 回到绝对坐标，四角装饰才能真正贴住屏幕边缘。
    // =====================================================================
    setorigin(0, 0);

    // 以 1080p 为基准的等比缩放，小屏也不会溢出
    const int    base = (SCREEN_W > 0 && SCREEN_H > 0)
                      ? ((SCREEN_W < SCREEN_H ? SCREEN_W : SCREEN_H)) : 1080;
    const double sc   = base / 1080.0;

    // 萤火虫：整个屏幕飘动
    fireflies.Draw(t);

    // 四角装饰：贴着物理屏幕的四条边
    const int mx = (int)(170 * sc);      // 左右边距
    const int my = (int)(130 * sc);      // 上边距
    const int bx = (int)(170 * sc);      // 下角水平位置
    const int by = (int)(140 * sc);      // 下角垂直位置

    DrawCornerCloudBamboo(mx, my, t, bg);                          // 左上：祥云 + 竹叶
    DrawCornerPlum(SCREEN_W - mx, my, t, bg);                      // 右上：梅枝 + 梅花
    DrawCornerIncenseBurner(bx, SCREEN_H - by, t, bg);             // 左下：香炉 + 轻烟
    DrawCornerBambooSlips(SCREEN_W - bx, SCREEN_H - by, bg);       // 右下：堆叠竹简

    // =====================================================================
    //  第二层：卷轴菜单（回到 1000x700 设计区坐标，卷轴本身居中于屏幕）
    // =====================================================================
    setorigin(UI_OFFSET_X(), UI_OFFSET_Y());

    // ---- 左右中段：折扇（扇动） + 酒壶（冒水汽）----
    DrawFoldingFan(150, 420, 118, 5.0 * std::sin(t * 1.1), bg);
    DrawWinePot(858, 420, bg);
    DrawSmoke(858 - 50 - 26, 420 - 30, t, bg);

    const int x1 = 280, y1 = 330, x2 = 720, y2 = 654;
    const int rad = 10;

    // ---- 半透明古书卷轴：暗红 -> 暗金渐变底 ----
    FillGradientRoundRect(x1, y1, x2, y2, rad,
                          BlendColor(bg, RGB(70, 34, 36), 0.60),
                          BlendColor(bg, RGB(44, 24, 26), 0.68));

    // 宣纸纤维纹理：极淡的横向细线
    for (int i = 0; i < 26; ++i) {
        const int y = y1 + 12 + i * 12;
        if (y > y2 - 12) break;
        setlinecolor(BlendColor(bg, RGB(255, 226, 180), 0.016 + (i % 3) * 0.006));
        setlinestyle(PS_SOLID, 1);
        line(x1 + 14, y, x2 - 14, y);
    }

    // 玉质侧光
    setfillcolor(BlendColor(bg, RGB(255, 220, 170), 0.045));
    solidrectangle(x1 + 12, y1 + 14, x1 + 96, y2 - 14);
    setfillcolor(BlendColor(bg, RGB(0, 0, 0), 0.10));
    solidrectangle(x2 - 104, y1 + 14, x2 - 12, y2 - 14);

    // 外框 + 内框
    setlinecolor(BlendColor(bg, RGB(198, 160, 78), 0.92));
    setlinestyle(PS_SOLID, 3);
    roundrect(x1, y1, x2, y2, rad, rad);
    setlinecolor(BlendColor(bg, kGold, 0.42));
    setlinestyle(PS_SOLID, 1);
    roundrect(x1 + 8, y1 + 8, x2 - 8, y2 - 8, rad, rad);

    // ---- 卷轴轴杆 ----
    setfillcolor(BlendColor(bg, RGB(92, 60, 32), 0.98));
    setlinecolor(BlendColor(bg, RGB(198, 160, 78), 0.92));
    setlinestyle(PS_SOLID, 2);
    solidroundrect(x1 - 16, y1 - 16, x2 + 16, y1 + 8, 10, 10);
    roundrect(x1 - 16, y1 - 16, x2 + 16, y1 + 8, 10, 10);
    solidroundrect(x1 - 16, y2 - 8, x2 + 16, y2 + 16, 10, 10);
    roundrect(x1 - 16, y2 - 8, x2 + 16, y2 + 16, 10, 10);

    // ---- 菜单上方：两盏红色古风大灯笼 ----
    setlinecolor(BlendColor(bg, kGold, 0.35));
    setlinestyle(PS_SOLID, 1);
    line(x1 + 60, 208, x2 - 60, 208);
    DrawLantern(x1 + 100, 208, 246, 17, 0.58);
    DrawLantern(x2 - 100, 208, 246, 17, 0.58);

    // ---- 菜单项 ----
    settextstyle(24, 0, L"微软雅黑");
    for (int i = 0; i < (int)menuItems.size(); i++) {
        const int yPos = 372 + i * 50;
        const int tw   = textwidth(menuItems[i].c_str());
        const int tx   = 500 - tw / 2;

        if (i == currentSelection) {
            setfillcolor(BlendColor(bg, kGold, 0.10));
            setlinecolor(BlendColor(bg, kGold, 0.10));
            solidroundrect(tx - 44, yPos - 12, tx + tw + 44, yPos + 36, 16, 16);

            DrawBrushBar(tx - 30, yPos - 8, tx + tw + 30, yPos + 32,
                         RGB(255, 238, 175), RGB(190, 140, 50), bg);

            settextcolor(RGB(255, 250, 232));
        }
        else {
            settextcolor(BlendColor(bg, RGB(214, 200, 168), 0.84));
        }
        outtextxy(tx, yPos, menuItems[i].c_str());
    }

    // ---- 底部小字提示 ----
    settextstyle(16, 0, L"微软雅黑");
    settextcolor(BlendColor(bg, RGB(178, 166, 140), 0.75));
    const wchar_t* hint = L"↑↓ 选择 · Enter 确认";
    outtextxy(500 - textwidth(hint) / 2, 686, hint);

    // =====================================================================
    //  让出前台的提示层（绝对坐标）：正在浏览中华英烈网
    // =====================================================================
    if (IsTopmostYielded()) {
        setorigin(0, 0);
        const int W = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
        const int H = (SCREEN_H > 0 ? SCREEN_H : WIN_H);

        // 半透明黑纱把菜单压暗，突出中央提示面板
        setfillcolor(BlendColor(bg, RGB(0, 0, 0), 0.58));
        setlinecolor(BlendColor(bg, RGB(0, 0, 0), 0.58));
        solidrectangle(0, 0, W, H);

        const int pw = (int)(560 * sc), ph = (int)(200 * sc);
        const int px1 = W / 2 - pw / 2, py1 = H / 2 - ph / 2;
        DrawGlassPanel(px1, py1, px1 + pw, py1 + ph, 18, RGB(255, 215, 0), 0.45);

        settextstyle(32, 0, L"楷体");
        DrawGlowText(W / 2 - textwidth(L"正在浏览中华英烈网") / 2, py1 + (int)(36 * sc),
                     L"正在浏览中华英烈网", RGB(255, 238, 180), RGB(255, 215, 0), 0.5, bg);

        settextstyle(20, 0, L"微软雅黑");
        settextcolor(BlendColor(bg, RGB(220, 208, 180), 0.92));
        const wchar_t* tip1 = L"浏览器已在前台打开";
        outtextxy(W / 2 - textwidth(tip1) / 2, py1 + (int)(104 * sc), tip1);

        const wchar_t* tip2 = L"按 [ESC] 返回本程序";
        outtextxy(W / 2 - textwidth(tip2) / 2, py1 + (int)(142 * sc), tip2);
    }
}
