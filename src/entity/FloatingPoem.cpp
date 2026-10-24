#include "FloatingPoem.h"
#include "../core/Config.h"
#include <cstdlib>

FloatingPoem::FloatingPoem(float startX, float startY, float spd, std::wstring txt) {
    x = startX;
    y = startY;
    speed = spd;
    text = txt;
    color = RGB(70, 60, 90); // 半透明的暗紫灰色
}

void FloatingPoem::Update() {
    x -= speed;
    if (x < -500) {
        const int boundW = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
        const int boundH = (SCREEN_H > 0 ? SCREEN_H : WIN_H);
        // 限制在屏幕上半屏：避免诗句飘到山水与菜单区域背后
        const int halfH  = (boundH > 240) ? (boundH / 2) : 120;
        x = (float)(boundW + rand() % 200);
        y = (float)(rand() % halfH);
    }
}

void FloatingPoem::Draw() const {
    settextstyle(26, 0, L"楷体");
    settextcolor(color);
    outtextxy((int)x, (int)y, text.c_str());
}
