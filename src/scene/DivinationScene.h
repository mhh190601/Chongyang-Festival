#pragma once
#include "Scene.h"
#include "../core/Bazi.h"
#include <string>
#include <vector>

// ============================================================================
//  测卦象场景 —— 三阶段状态机（全程图形化交互，不再使用控制台输入）
//
//    INPUT   阵法起局：金色八卦阵缓缓自转，中央卷轴面板用方向键输入生辰
//    ROLLING 天机推演：输入面板消失，阵法转速暴增，持续 1.5 秒
//    RESULT  卦象揭晓：阵法骤停，金色竹签从天而降，展示八字 / 卦象 / 签文
// ============================================================================
enum class DivinationPhase {
    INPUT,
    ROLLING,
    RESULT
};

class DivinationScene : public Scene {
public:
    DivinationScene();
    GameState Update() override;
    void Render() override;

private:
    // ---- 状态切换 ----
    void AdjustField(int delta);
    void TryStartRolling();
    void EnterResult();
    void ResetToInput();

    // ---- 绘制 ----
    void DrawBaguaArray() const;      // 旋转的金色八卦阵
    void DrawInputPanel() const;      // 阶段一：生辰输入卷轴
    void DrawResultSlip() const;      // 阶段三：金色竹签
    void DrawTitleAndHints() const;   // 标题与底部提示
    void BuildHexagram();             // 由八字起卦（梅花易数·时间卦）

    // ---- 工具 ----
    static double EaseOutCubic(double t);
    static int    ClampInt(int v, int lo, int hi);

private:
    DivinationPhase phase    = DivinationPhase::INPUT;
    DWORD           phaseTick = 0;
    DWORD           lastTick  = 0;

    // 输入
    int            field = 0;                 // 0=年 1=月 2=日 3=时
    int            year = 2000, month = 1, day = 1, hour = 12;
    std::wstring   errorMsg;
    DWORD          errorTick = 0;

    // 阵法
    double spin      = 0.0;
    double spinSpeed = 0.35;                  // 弧度 / 秒
    bool   hasResult = false;

    // 结果
    BirthInfo  birth;
    BaziResult bazi;
    int         upperTrigram = 1;
    int         lowerTrigram = 1;
    int         movingYao    = 1;
    int         hexIndex     = -1;
    int         changedIndex = -1;
    int         fortuneIndex = 0;
    std::wstring fortuneNo;      // 签号，如 L"059"
    std::wstring fortuneLevel;   // 吉凶，如 L"中  吉"
    std::wstring fortunePoem;    // 签诗（两句，\n 分行）
    std::wstring fortuneNote;    // 解签（两句，\n 分行）
    std::wstring guaName;
    std::wstring guaCi;
    std::vector<std::wstring> yaoLines;        // 六爻符号，[0] 为最底爻
};
