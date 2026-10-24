#pragma once
#include <string>

// ============================================================================
//  Audio —— 背景音乐（winmm / mciSendString）
//
//  用法：main() 里 Init + PlayBGM，退出前 Shutdown。
//  音乐文件默认放在项目根目录（bgm.mp3），找不到时静默失败，不影响程序运行。
// ============================================================================
namespace Audio {
    // 打开背景音乐文件；返回是否成功（文件缺失时返回 false 并打印提示）
    bool Init(const std::wstring& fileName = L"bgm.mp3");

    void PlayBGM(bool loop = true);   // 循环播放（从头开始）
    void StopBGM();
    void Shutdown();                  // 关闭设备，退出程序前调用

    bool IsPlaying();
    void SetVolume(int percent);      // 0 - 100
}
