#pragma once
#include "Scene.h"
#include <string>
#include <vector>

// 祝福语场景：
//   进入时弹出控制台让玩家自己输入祝福语（EasyX 无法调出中文输入法，用控制台兜底）；
//   直接回车则使用默认祝福语。之后仍可按空格在「自定义 + 5 句内置」之间循环切换。
class BlessScene : public Scene {
public:
    BlessScene();
    GameState Update() override;
    void Render() override;

private:
    void SwitchBless(int delta);

private:
    static const int kBuiltInCount = 5;

    std::vector<std::wstring> blessings;   // 候选列表（自定义在前）
    std::wstring customText;               // 玩家输入的内容
    std::wstring blessText;                // 当前滚动的那一句
    int          index      = 0;
    float        blessX     = 0.0f;
    float        blessSpeed = 4.0f;
};
