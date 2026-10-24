#pragma once
#include <graphics.h>
#include <string>

// 背景中漂浮的诗句
class FloatingPoem {
public:
    FloatingPoem(float startX, float startY, float speed, std::wstring text);
    void Update();
    void Draw() const;

private:
    float       x, y;      // 位置
    float       speed;     // 左移速度
    std::wstring text;     // 诗句内容
    COLORREF    color;     // 颜色
};
