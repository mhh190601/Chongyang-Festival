#include "Petal.h"
#include "../core/Config.h"

#include <cstdlib>
#include <cmath>

Petal::Petal() {
    const int boundW = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int boundH = (SCREEN_H > 0 ? SCREEN_H : WIN_H);

    x = (float)(rand() % boundW);
    y = (float)(rand() % boundH);

    vx = 0.35f + (float)(rand() % 10) / 14.0f;    // 轻微的右向风力
    vy = 0.9f + (float)(rand() % 14) / 10.0f;     // 斜向下飘落
    size = 6.0f + (float)(rand() % 9);            // 6 ~ 14 像素（更大更清晰）
    sway = (float)(rand() % 628) / 100.0f;        // 摆动初相位

    // 柔和的暗红 / 橙红色系（比纯红更耐看，亮度略高以便远看也清晰）
    static const COLORREF kPalette[4] = {
        RGB(192, 70, 58),
        RGB(210, 100, 64),
        RGB(170, 56, 52),
        RGB(218, 128, 74)
    };
    color = kPalette[rand() % 4];
}

void Petal::Update() {
    const int boundW = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int boundH = (SCREEN_H > 0 ? SCREEN_H : WIN_H);

    sway += 0.06f;
    x += vx + std::sin(sway) * 0.55f;   // 随风左右摆动，斜向下飘
    y += vy;

    if (y > boundH) { y = -10.0f; x = (float)(rand() % boundW); }
    if (x > boundW + 10) x = -10.0f;
    if (x < -10) x = (float)(boundW + 10);
}

void Petal::Draw() const {
    const int w = (int)size;
    const int h = (int)(size * 0.72f);          // 扁一点，更像花瓣

    setfillcolor(color);
    setlinecolor(color);                        // 与填充同色，避免描边
    solidellipse((int)x - w / 2, (int)y - h / 2,
                 (int)x + w / 2, (int)y + h / 2);
}
