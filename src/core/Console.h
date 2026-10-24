#pragma once
#include <windows.h>
#include <string>

namespace ConsoleApp {
    void Init();                    // 初始化控制台并彻底隐藏（日志已重定向到文件）
    void SetMainWindow(HWND hwnd);  // 注册图形主窗口（输入结束后归还键盘焦点）
    void Show();                    // 弹出控制台并强制置顶（解决全屏窗口抢焦点）
    void Hide();                    // 隐藏控制台，并把焦点还给图形窗口
    std::wstring GetInput();        // 弹出控制台 -> 读一行 -> 隐藏控制台
    std::wstring GetLine();         // 只读一行（不负责显示/隐藏，供循环输入使用）

    void ForceShowCursor();         // 强制显示鼠标指针（退出前调用）
    void ForceHideCursor();         // 强制隐藏鼠标指针（全屏动画时调用）

    // 把 stdout 从日志文件切回控制台（调用 Show 时自动切换），
    // 这样控制台可见时 printf 的提示语用户能看见，隐藏时则只写日志。
    void UseConsoleOutput();
    void UseLogOutput();
}
