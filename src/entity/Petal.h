#pragma once
#include <graphics.h>

// 飘落的花瓣（背景装饰）
class Petal {
public:
    Petal();
    void Update();
    void Draw() const;

private:
    float    x, y;        // 位置
    float    vx, vy;      // 速度
    float    size;        // 大小（3 ~ 8 像素）
    float    sway;        // 左右摆动相位
    COLORREF color;       // 柔和的暗红 / 橙色
};
