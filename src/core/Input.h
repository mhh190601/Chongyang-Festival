#pragma once
#include <windows.h>

namespace Input {
    void Update(); // 每帧调用，更新内部状态
    bool IsKeyDown(int vKey);     // 判断是否按住
    bool IsKeyPressed(int vKey);  // 判断是否刚按下（防连发）
    void Flush();                 // 用当前物理状态重置“上一帧状态”，防止跨场景按键边沿串场
}
