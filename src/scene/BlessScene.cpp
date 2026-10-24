#include "BlessScene.h"
#include "../core/Input.h"
#include "../core/Console.h"
#include "../core/DrawUtil.h"

#include <cstdio>
#include <string>
#include <windows.h>

namespace {

// 内置祝福语（玩家跳过 / 取消时使用，之后也可用空格切换）
const wchar_t* kBuiltIn[5] = {
    L"祝天下老人重阳节安康，福寿绵长！",
    L"九九重阳，登高望远，愿岁岁如今朝。",
    L"遥寄茱萸一枝，愿亲人平安顺遂。",
    L"菊花黄，桂花香，愿长辈身体康健。",
    L"敬老爱老，愿时光温柔以待。"
};

// ============================================================================
//  原生模态输入框
//
//  为什么不用 EasyX 自带的 InputBox：
//    它内部用的是多行 EDIT 控件，回车会被当成换行，必须 Ctrl+回车才能确认，
//    体验极差（用户已经反馈过）。
//
//  这里用 Win32 自己写一个：单行 EDIT + 「确定」为默认按钮（BS_DEFPUSHBUTTON），
//    · 单按 Enter 即确认并关闭
//    · ESC / [跳过] 取消
//    · 原生 EDIT 控件，中文输入法可正常调出
// ============================================================================
struct InputBoxState {
    HWND        hEdit = NULL;
    bool        ok    = false;
    std::wstring* out = nullptr;
};

InputBoxState g_ib;   // 同时只会有一个输入框

LRESULT CALLBACK InputBoxWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            const CREATESTRUCTW* cs = (const CREATESTRUCTW*)lParam;
            const wchar_t* prompt   = (const wchar_t*)cs->lpCreateParams;
            HINSTANCE hInst         = cs->hInstance;

            CreateWindowExW(0, L"STATIC", prompt, WS_CHILD | WS_VISIBLE | SS_LEFT,
                            18, 16, 444, 22, hWnd, (HMENU)1001, hInst, NULL);

            // 跳过提示（第二行小字）
            CreateWindowExW(0, L"STATIC",
                            L"若无想写的，直接点击 [确定] 或按回车跳过（将使用内置祝福语）",
                            WS_CHILD | WS_VISIBLE | SS_LEFT,
                            18, 42, 444, 20, hWnd, (HMENU)1002, hInst, NULL);

            // 单行输入框：绝不加 ES_MULTILINE / ES_WANTRETURN，回车才会走默认按钮
            g_ib.hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                         18, 74, 444, 30, hWnd, (HMENU)1003, hInst, NULL);

            // 「确定」设为默认按钮 -> 单按回车即触发
            CreateWindowExW(0, L"BUTTON", L"确定",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                            236, 124, 100, 32, hWnd, (HMENU)IDOK, hInst, NULL);
            CreateWindowExW(0, L"BUTTON", L"跳过",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                            348, 124, 100, 32, hWnd, (HMENU)IDCANCEL, hInst, NULL);

            SetFocus(g_ib.hEdit);
            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                wchar_t buf[512] = { 0 };
                if (g_ib.hEdit) GetWindowTextW(g_ib.hEdit, buf, 512);
                if (g_ib.out) *g_ib.out = buf;
                g_ib.ok = true;
                DestroyWindow(hWnd);
                return 0;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                g_ib.ok = false;
                DestroyWindow(hWnd);
                return 0;
            }
            break;

        case WM_CLOSE:
            g_ib.ok = false;
            DestroyWindow(hWnd);
            return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// 弹出模态输入框。返回 true 表示点了确定，text 接收输入内容（可能为空串）
bool NativeInputBox(const wchar_t* prompt, const wchar_t* title, std::wstring& text) {
    HINSTANCE hInst = GetModuleHandleW(NULL);
    static const wchar_t* kCls = L"ChongYangBlessInputBox";
    static bool s_reg = false;

    if (!s_reg) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = InputBoxWndProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = kCls;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
        RegisterClassExW(&wc);
        s_reg = true;
    }

    const int dlgW = 480, dlgH = 196;
    const int x = (GetSystemMetrics(SM_CXSCREEN) - dlgW) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - dlgH) / 2;

    HWND hParent = GetHWnd();

    g_ib.out   = &text;
    g_ib.ok    = false;
    g_ib.hEdit = NULL;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, kCls, title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH, hParent, NULL, hInst, (LPVOID)prompt);

    if (hDlg == NULL) {
        g_ib.out = nullptr;
        return false;
    }

    // 模态循环：禁用父窗口，抽干消息直到对话框被销毁
    EnableWindow(hParent, FALSE);

    MSG msg;
    bool quitSeen = false;
    while (!quitSeen && IsWindow(hDlg)) {
        WaitMessage();                       // 阻塞等消息，不占 CPU
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { quitSeen = true; break; }
            // IsDialogMessageW 负责 Tab 导航、ESC 取消、以及回车点默认按钮
            if (!IsDialogMessageW(hDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    if (quitSeen) PostQuitMessage(0);        // 不吞掉别人的退出消息
    g_ib.out = nullptr;
    return g_ib.ok;
}

} // namespace

BlessScene::BlessScene() {
    index      = 0;
    blessX     = (float)WIN_W;      // 从设计区右侧开始滚动
    blessSpeed = 4.0f;

    // ---- 让玩家自己输入祝福语 ----
    // 1) 图形窗口是 TOPMOST，先让位，否则对话框会被压在画面下面
    // 2) 全局鼠标是隐藏的，必须临时显示出来，否则用户看不见光标、点不了“确定”
    YieldTopmost();
    ConsoleApp::ForceShowCursor();

    std::wstring input;
    const bool ok = NativeInputBox(L"请输入你想滚动的祝福语：",
                                   L"重阳 · 祝福语", input);

    ConsoleApp::ForceHideCursor();
    RestoreTopmost();

    if (ok && !input.empty()) {
        customText = input;
        printf("[Log] 已接收自定义祝福语：%ls\n", customText.c_str());
    } else {
        printf("[Log] 未输入自定义祝福语，使用默认祝福语\n");
    }

    // 候选列表：自定义（如有）在前，内置 5 句在后
    blessings.clear();
    if (!customText.empty()) blessings.push_back(customText);
    for (int i = 0; i < kBuiltInCount; ++i) blessings.push_back(kBuiltIn[i]);

    blessText = blessings[0];
    printf("[Log] 祝福语候选共 %d 条\n", (int)blessings.size());
}

void BlessScene::SwitchBless(int delta) {
    if (blessings.empty()) return;
    index = (index + delta + (int)blessings.size()) % (int)blessings.size();
    blessText = blessings[index];
    blessX = (float)WIN_W;          // 切换后重新从右侧滚入
    printf("[Log] 切换祝福语 %d/%d\n", index + 1, (int)blessings.size());
}

GameState BlessScene::Update() {
    // 切换祝福语：空格 / 右键下一篇，左键上一篇
    if (Input::IsKeyPressed(VK_SPACE) || Input::IsKeyPressed(VK_RIGHT)) {
        SwitchBless(+1);
    }
    else if (Input::IsKeyPressed(VK_LEFT)) {
        SwitchBless(-1);
    }

    // 滚动（先设好文字样式再测宽，避免测到别的字号的宽度）
    settextstyle(52, 0, L"微软雅黑");
    const int tw = textwidth(blessText.c_str());
    blessX -= blessSpeed;
    if (blessX < -(float)tw) {
        blessX = (float)WIN_W;
    }

    if (Input::IsKeyPressed(VK_ESCAPE)) {
        return STATE_MENU;
    }
    return STATE_BLESS;
}

void BlessScene::Render() {
    const COLORREF bg = NightBg();
    const int W = WIN_W;
    const int H = WIN_H;

    // ---- 标题 ----
    settextstyle(34, 0, L"楷体");
    DrawGlowText(W / 2 - textwidth(L"祝福语滚动") / 2, 90, L"祝福语滚动",
                 RGB(255, 232, 170), RGB(255, 215, 0), 0.55);

    // ---- 滚动的主祝福语（带光晕）----
    settextstyle(52, 0, L"微软雅黑");
    DrawGlowText((int)blessX, 300, blessText.c_str(),
                 RGB(255, 226, 140), RGB(255, 215, 0), 0.50);

    // ---- 当前句的面板（完整可读）----
    DrawGlassPanel(120, 400, 880, 486, 16, RGB(255, 215, 0), 0.22);

    settextstyle(24, 0, L"楷体");
    settextcolor(BlendColor(bg, RGB(240, 228, 200), 0.94));
    const std::wstring cur = L"「 " + blessText + L" 」";
    outtextxy(W / 2 - textwidth(cur.c_str()) / 2, 430, cur.c_str());

    // ---- 指示点 ----
    const int total = (int)blessings.size();
    for (int i = 0; i < total; ++i) {
        const int dx = W / 2 + (i - total / 2) * 34;
        const int dy = 528;
        if (i == index) {
            for (int g = 3; g >= 1; --g) {
                setfillcolor(BlendColor(bg, RGB(255, 215, 0), 0.05 * g));
                setlinecolor(BlendColor(bg, RGB(255, 215, 0), 0.05 * g));
                solidcircle(dx, dy, 5 + g * 3);
            }
            setfillcolor(RGB(255, 226, 140));
            setlinecolor(RGB(255, 226, 140));
            solidcircle(dx, dy, 5);
        } else {
            setfillcolor(BlendColor(bg, RGB(190, 178, 150), 0.45));
            setlinecolor(BlendColor(bg, RGB(190, 178, 150), 0.45));
            solidcircle(dx, dy, 3);
        }
    }

    // ---- 底部提示 ----
    wchar_t hint[96];
    swprintf(hint, 96, L"按 [空格] 切换祝福语（%d/%d）　按 [ESC] 返回主菜单",
             index + 1, total);
    settextstyle(18, 0, L"微软雅黑");
    settextcolor(BlendColor(bg, RGB(206, 192, 160), 0.85));
    outtextxy(W / 2 - textwidth(hint) / 2, H - 60, hint);
}
