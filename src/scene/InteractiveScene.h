#pragma once
#include "Scene.h"
#include "../core/DrawUtil.h"

#include <vector>

// ============================================================================
//  InteractiveScene —— 互动广场 · 孔明灯祈福
//
//  玩法：按 [空格] 从屏幕底部随机位置放飞一盏孔明灯（最多同时 15 盏），
//        灯火摇曳上升、光晕呼吸闪烁，灯身上方浮着一句祝福；
//        按 [ESC] 返回主菜单。
//
//  坐标：全部使用物理屏幕绝对坐标（SCREEN_W / SCREEN_H），
//        不再经过 setorigin 的设计区平移，因此任何分辨率都铺满全屏。
// ============================================================================
class InteractiveScene : public Scene {
public:
    InteractiveScene();
    GameState Update() override;
    void Render() override;

private:
    // 一盏孔明灯
    struct Lantern {
        float baseX     = 0.0f;   // 水平基准位置（像素）
        float x         = 0.0f;   // 当前 x（= baseX + 正弦摇摆偏移）
        float y         = 0.0f;   // 当前 y（像素，向上递减）
        float speed     = 40.0f;  // 上升速度（像素 / 秒）
        float swayAmp   = 22.0f;  // 左右摇摆幅度（像素）
        float swayFreq  = 0.7f;   // 摇摆频率（Hz）
        float phase     = 0.0f;   // 摇摆相位
        float size      = 26.0f;  // 灯体大小（像素）
        float glow      = 1.0f;   // 光照大小系数
        float age       = 0.0f;   // 已飞行时间（秒），用于呼吸闪烁
        int   wish      = 0;      // 祝福语索引
    };

    void SpawnLantern();
    void DrawLanternBody(const Lantern& L) const;

private:
    std::vector<Lantern> m_lanterns;
    double  m_time      = 0.0;    // 场景时间（秒）
    DWORD   m_lastTick  = 0;

    // 达到上限时的提示（2 秒后自动消失）
    bool     m_fullHint = false;
    DWORD    m_fullTick = 0;
};
