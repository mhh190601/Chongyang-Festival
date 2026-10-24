#pragma once
#include "../core/Config.h"

// 场景基类
class Scene {
public:
    virtual ~Scene() {}
    // 返回值为下一个要跳转的状态，如果返回当前状态则不变
    virtual GameState Update() = 0;
    virtual void Render() = 0;
};
