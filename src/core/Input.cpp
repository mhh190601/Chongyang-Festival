#include "Input.h"
#include <map>

namespace Input {
    std::map<int, bool> lastState;

    void Update() {
        // 在当前版本不需要额外更新，GetAsyncKeyState 是实时的
    }

    bool IsKeyDown(int vKey) {
        return (GetAsyncKeyState(vKey) & 0x8000) != 0;
    }

    bool IsKeyPressed(int vKey) {
        bool current = IsKeyDown(vKey);
        bool last = lastState[vKey];
        lastState[vKey] = current; // 记录当前状态
        return current && !last;   // 只有上一帧没按，这一帧按了，才算 Pressed
    }

    void Flush() {
        // 场景切换时调用：把“上一帧状态”对齐到当前真实按键状态。
        // 这样即便玩家还按着某个键，也不会在新场景里被判定为“刚按下”，
        // 避免出现“跳过后立刻选中菜单某一项”的连发误触。
        for (int vKey = 0; vKey < 256; ++vKey) {
            lastState[vKey] = IsKeyDown(vKey);
        }
    }
}
