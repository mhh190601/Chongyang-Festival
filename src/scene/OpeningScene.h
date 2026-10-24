#pragma once
#include "Scene.h"
#include "../core/DrawUtil.h"
#include <vector>

// ============================================================================
//  OpeningScene —— 全屏开场动画（水墨国风）
//
//  0.0 ~ 1.3s   满天金色孔明灯缓缓升腾，一道光芒扫过屏幕
//  1.3s         “重 阳”二字被火焰点燃般浮现（扩散光环 + 暖橙转金的光晕）
//  背景：星光 / 萤火虫 / 六层水墨远山 / 云海 / 灯笼 / 菊花簇
//  持续 6 秒自动进入主菜单，或按回车 / ESC 跳过（1.5 秒防抖）
// ============================================================================
class OpeningScene : public Scene {
public:
    OpeningScene();
    GameState Update() override;
    void Render() override;

    // 孔明灯（天灯）
    struct SkyLantern {
        float x0        = 0.0f;   // 水平基准位置 0..1
        float period    = 16.0f;  // 上升一周期的秒数
        float phase     = 0.0f;   // 相位 0..1
        float size      = 14.0f;  // 灯体半宽（像素）
        float swayPhase = 0.0f;   // 左右摆动相位
    };

private:
    static double EaseOutCubic(double t);
    static double Progress(double elapsed, double start, double duration);

    void InitLanterns(int count);

private:
    DWORD       startTick = 0;
    double      elapsed   = 0.0;
    FireflyField fireflies;

    std::vector<SkyLantern> m_lanterns;   // 满天孔明灯
};
