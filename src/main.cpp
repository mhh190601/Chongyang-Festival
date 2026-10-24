#include <graphics.h>
#include <windows.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdlib>
#include <ctime>

#include "core/Config.h"
#include "core/Console.h"
#include "core/Input.h"
#include "core/Audio.h"

#include "entity/Petal.h"
#include "entity/FloatingPoem.h"

#include "scene/Scene.h"
#include "scene/OpeningScene.h"
#include "scene/MenuScene.h"
#include "scene/BlessScene.h"
#include "scene/DivinationScene.h"
#include "scene/InteractiveScene.h"

// 全屏初始化：按系统分辨率建窗口，并去掉标题栏 / 边框
static void InitFullScreen() {
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    if (screenW <= 0 || screenH <= 0) {   // 兜底
        screenW = WIN_W;
        screenH = WIN_H;
    }
    SCREEN_W = screenW;
    SCREEN_H = screenH;

    initgraph(SCREEN_W, SCREEN_H);
    SetWindowTextW(GetHWnd(), L"重阳节 · 交互程序");

    HWND hWnd = GetHWnd();
    if (hWnd == NULL) return;

    // 去掉标题栏、可调边框、系统菜单等，让画面铺满整个屏幕
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
               WS_MAXIMIZEBOX | WS_SYSMENU | WS_BORDER | WS_DLGFRAME);
    SetWindowLongPtr(hWnd, GWL_STYLE, style);

    LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
                 WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_TOOLWINDOW);
    SetWindowLongPtr(hWnd, GWL_EXSTYLE, exStyle);

    // 用 HWND_TOPMOST 铺满全屏并遮住任务栏（录屏时画面完整）。
    // 需要弹控制台时，ConsoleApp::Show() 会临时把它降级为 HWND_TOP，
    // 关闭后 ConsoleApp::Hide() 再恢复 TOPMOST。
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, SCREEN_W, SCREEN_H,
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

int main() {
    // 0. 把所有日志重定向到项目根目录的 ChongYang_Log.txt（覆盖模式），
    //    这样录屏时后台不会再冒出黑框刷日志。
    FILE* logFile = nullptr;
    if (freopen_s(&logFile, "ChongYang_Log.txt", "w", stdout) != 0) {
        // 重定向失败则保留控制台输出，保证出错时能看到提示
        MessageBoxW(NULL, L"无法创建日志文件 ChongYang_Log.txt，日志将输出到控制台。", L"提示", MB_OK);
    }
    FILE* errFile = nullptr;
    freopen_s(&errFile, "ChongYang_Log.txt", "a", stderr);   // stderr 失败不影响主流程

    // 1. 初始化基础环境
    SetConsoleOutputCP(65001);
    ConsoleApp::Init(); // 初始化控制台并彻底隐藏
    InitFullScreen();   // 全屏图形窗口

    // 控制台输入结束后，把键盘焦点归还给图形窗口
    ConsoleApp::SetMainWindow(GetHWnd());
    ConsoleApp::ForceHideCursor();   // 全屏动画期间隐藏鼠标指针

    // 背景音乐（文件在项目根目录 bgm.mp3；缺失时静默跳过，不影响其他功能）
    Audio::Init(L"bgm.mp3");
    Audio::PlayBGM();   // 循环播放，贯穿整个程序

    srand((unsigned)time(NULL));

    // 2. 初始化背景实体
    std::vector<Petal> petals;
    for (int i = 0; i < 50; i++) {
        petals.push_back(Petal());
    }

    std::vector<FloatingPoem> poems;
    std::vector<std::wstring> texts = {
        L"独在异乡为异客，每逢佳节倍思亲",
        L"遥知兄弟登高处，遍插茱萸少一人",
        L"尘世难逢开口笑，菊花须插满头归",
        L"江涵秋影雁初飞，与客携壶上翠微",
        L"九日龙山饮，黄花笑逐臣",
        L"满园花菊郁金黄，中有孤丛色似霜",
        L"莫道不消魂，帘卷西风，人比黄花瘦",
        L"重阳独酌杯中酒，抱病起登江上台"
    };
    // 诗句只出现在屏幕上半屏（与 FloatingPoem::Update 的回收逻辑保持一致）
    const int spawnW     = (SCREEN_W > 0 ? SCREEN_W : WIN_W);
    const int spawnHalfH = (SCREEN_H > 0 ? SCREEN_H : WIN_H) / 2;
    for (int i = 0; i < (int)texts.size(); i++) {
        poems.push_back(FloatingPoem(
            (float)(rand() % spawnW),
            (float)(rand() % spawnHalfH),
            0.6f + (float)(rand() % 14) / 10.0f,
            texts[i]
        ));
    }

    // 3. 初始化场景（从全屏开场动画开始）
    GameState currentState = STATE_OPENING;
    std::unique_ptr<Scene> currentScene = std::make_unique<OpeningScene>();

    BeginBatchDraw();

    // 4. 游戏主循环
    bool isRunning = true;
    while (isRunning) {
        // 检查窗口是否被关闭
        if (GetHWnd() == NULL) {
            break;
        }

        // --- 逻辑更新 ---
        Input::Update(); // 更新键盘状态

        // 更新场景，并获取场景返回的下一步状态
        GameState nextState = currentScene->Update();

        // 处理场景切换
        if (nextState != currentState) {
            currentState = nextState;

            // 退出程序
            if (currentState == STATE_EXIT) {
                isRunning = false;
                break;
            }
            // 切换场景
            else if (currentState == STATE_OPENING) {
                currentScene = std::make_unique<OpeningScene>();
            }
            else if (currentState == STATE_MENU) {
                currentScene = std::make_unique<MenuScene>();
            }
            else if (currentState == STATE_BLESS) {
                currentScene = std::make_unique<BlessScene>();
            }
            else if (currentState == STATE_DIVINATION) {
                currentScene = std::make_unique<DivinationScene>();
            }
            else if (currentState == STATE_INTERACTIVE) {
                currentScene = std::make_unique<InteractiveScene>();
            }

            // 防止上一个场景的按键状态“串场”到新场景
            //（例如回车跳过后还按着，导致新场景瞬间触发一次）
            Input::Flush();
        }

        // 更新背景实体
        for (auto& p : petals) p.Update();
        for (auto& p : poems) p.Update();

        // --- 画面渲染 ---
        setbkcolor(RGB(15, 10, 25));
        cleardevice();

        // 画背景层
        for (const auto& p : petals) p.Draw();
        for (const auto& p : poems) p.Draw();

        // 画 UI 层
        // 开场动画使用全屏坐标；其余场景仍按 1000x700 设计区布局，
        // 这里把坐标系原点平移到屏幕中央，让原有 UI 在大屏上居中显示
        if (currentState == STATE_OPENING) {
            currentScene->Render();
        }
        else {
            setorigin(UI_OFFSET_X(), UI_OFFSET_Y());
            currentScene->Render();
            setorigin(0, 0);
        }

        // --- 画面刷新 ---
        FlushBatchDraw();
        Sleep(16); // 60 FPS
    }

    EndBatchDraw();
    closegraph();
    Audio::Shutdown();   // 关闭背景音乐，避免后台继续播放
    ConsoleApp::ForceShowCursor();   // 退出前恢复鼠标指针

    printf("=== 系统正常退出 ===\n");
    fflush(stdout);
    fclose(stdout);      // 刷新并关闭日志文件
    return 0;
}
