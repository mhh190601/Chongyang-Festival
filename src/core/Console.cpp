#include "Console.h"

#include <iostream>
#include <conio.h>
#include <cstdio>

namespace ConsoleApp {
    static HWND  hConsole  = NULL;
    static HWND  hMainWnd  = NULL;
    static char  g_logPath[MAX_PATH] = "ChongYang_Log.txt";   // 日志文件路径
    static bool  g_consoleOut = false;                        // stdout 当前是否指向控制台

    // 控制台回调：把「关闭控制台」事件改为最小化，
    // 否则用户误点控制台的 X 会把整个程序一起 ExitProcess 掉
    static BOOL WINAPI OnConsoleCtrlEvent(DWORD dwCtrlType) {
        if (dwCtrlType == CTRL_CLOSE_EVENT ||
            dwCtrlType == CTRL_C_EVENT      ||
            dwCtrlType == CTRL_BREAK_EVENT) {
            if (hConsole) ShowWindow(hConsole, SW_MINIMIZE);
            return TRUE;   // 返回 TRUE = 阻止默认处理（不退出进程）
        }
        return FALSE;
    }

    void Init() {
        SetConsoleOutputCP(65001);
        hConsole = GetConsoleWindow();
        // 日志已重定向到文件，黑框彻底隐藏；需要时可由 Show() 唤出
        if (hConsole) ShowWindow(hConsole, SW_HIDE);
        SetConsoleCtrlHandler(OnConsoleCtrlEvent, TRUE);
    }

    void SetMainWindow(HWND hwnd) {
        hMainWnd = hwnd;
    }

    // 把 stdout 切到控制台（控制台可见时用户才能看到提示语）
    void UseConsoleOutput() {
        if (g_consoleOut) return;
        FILE* fp = nullptr;
        freopen_s(&fp, "CON", "w", stdout);
        g_consoleOut = true;
    }

    // 把 stdout 切回日志文件（追加写，不覆盖已有日志）
    void UseLogOutput() {
        if (!g_consoleOut) return;
        FILE* fp = nullptr;
        freopen_s(&fp, g_logPath, "a", stdout);
        g_consoleOut = false;
    }

    // 把指定窗口强制提到最前。
    // 全屏无边框窗口容易抢占焦点，直接 SetForegroundWindow 常被系统忽略，
    // 这里先通过 AttachThreadInput 挂到当前前台线程，再临时置顶一次。
    static void ForceForeground(HWND hwnd) {
        if (hwnd == NULL) return;

        ShowWindow(hwnd, SW_RESTORE);

        const DWORD foreThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        const DWORD thisThread = GetCurrentThreadId();
        const bool attached = (foreThread != 0 && foreThread != thisThread);

        if (attached) AttachThreadInput(thisThread, foreThread, TRUE);
        SetForegroundWindow(hwnd);
        BringWindowToTop(hwnd);

        // 临时置顶再取消，确保压在全屏窗口之上（不需要长期霸占 TOPMOST）
        SetWindowPos(hwnd, HWND_TOPMOST,    0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        if (attached) AttachThreadInput(thisThread, foreThread, FALSE);
    }

    void ForceShowCursor() {
        while (ShowCursor(TRUE) < 0) {}   // 强制显示（ShowCursor 是计数器，要循环到 >= 0）
    }

    void ForceHideCursor() {
        while (ShowCursor(FALSE) >= 0) {} // 强制隐藏
    }

    void Show() {
        UseConsoleOutput();               // 提示语打到控制台
        // 让出最顶层：否则 TOPMOST 的全屏窗口会把控制台压在下面看不见
        if (hMainWnd) {
            SetWindowPos(hMainWnd, HWND_TOP, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        ForceForeground(hConsole);
        ForceShowCursor();                // 打字时把鼠标指针显示出来
    }

    void Hide() {
        ForceHideCursor();
        if (hConsole) ShowWindow(hConsole, SW_HIDE);
        UseLogOutput();                   // 之后的日志回到文件
        ForceForeground(hMainWnd);        // 把键盘焦点还给图形窗口
        // 恢复最顶层：遮住任务栏，保证全屏录屏画面完整
        if (hMainWnd) {
            SetWindowPos(hMainWnd, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }

    // 只读取一行，不负责控制台的显示 / 隐藏（供循环输入场景使用）
    std::wstring GetLine() {
        // 上一次输入若异常（EOF / failbit，例如控制台被关掉过），
        // getline 会立即返回空串而不等待输入，导致调用方无限重弹。
        // 这里先恢复流状态，保证每次都会真正等待用户输入。
        if (!std::cin) {
            std::cin.clear();
        }
        std::string inputStr;
        std::getline(std::cin, inputStr);

        if (inputStr.empty()) return L"";

        // 将 UTF-8 转宽字符
        int len = MultiByteToWideChar(CP_UTF8, 0, inputStr.c_str(), -1, NULL, 0);
        wchar_t* wstr = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, inputStr.c_str(), -1, wstr, len);
        std::wstring result = wstr;
        delete[] wstr;
        return result;
    }

    std::wstring GetInput() {
        Show();
        std::wstring result = GetLine();
        Hide();
        return result;
    }
}
