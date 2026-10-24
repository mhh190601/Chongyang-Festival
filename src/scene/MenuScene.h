#pragma once
#include "Scene.h"
#include "../core/DrawUtil.h"
#include <vector>
#include <string>

class MenuScene : public Scene {
public:
    MenuScene();
    GameState Update() override;
    void Render() override;

private:
    int currentSelection;
    std::vector<std::wstring> menuItems;
    FireflyField fireflies;      // 菜单周围飘动的萤火虫
    DWORD yieldTick = 0;         // 让出前台（最小化）的起始时刻
};
