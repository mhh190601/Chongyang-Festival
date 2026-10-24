#pragma once
#include <graphics.h>

// ============================================================================
// 分辨率说明
//   WIN_W / WIN_H  —— 设计分辨率（原有 1000x700 窗口布局）
//   SCREEN_W / SCREEN_H —— 实际屏幕分辨率（由 main() 写入）
// 程序启动时窗口会铺满整个屏幕，原有场景仍按 1000x700 布局绘制，
// 主循环通过 setorigin() 把这块设计区整体平移到屏幕正中央，
// 因此现有场景无需改动坐标。
// ============================================================================
inline int WIN_W = 1000;
inline int WIN_H = 700;

inline int SCREEN_W = 0;
inline int SCREEN_H = 0;

// 把设计区平移到屏幕中央所需的偏移量
inline int UI_OFFSET_X() { return (SCREEN_W > WIN_W) ? (SCREEN_W - WIN_W) / 2 : 0; }
inline int UI_OFFSET_Y() { return (SCREEN_H > WIN_H) ? (SCREEN_H - WIN_H) / 2 : 0; }

// 所有的游戏状态（后面加卦象就加在这里）
enum GameState {
    STATE_OPENING,     // 全屏开场动画
    STATE_MENU,
    STATE_BLESS,
    STATE_DIVINATION,
    STATE_INTERACTIVE, // 互动广场：孔明灯祈福
    STATE_EXIT
};
