#include "Audio.h"

#include <windows.h>
#include <mmsystem.h>
#include <cstdio>

#pragma comment(lib, "winmm.lib")

namespace {

const wchar_t* kAlias = L"bgm";

bool        g_opened    = false;
bool        g_playing   = false;
int         g_volume    = 70;    // 默认音量 70%
std::wstring g_tempFile;         // 若复制过临时副本，记录路径以便退出时删除

bool FileExists(const std::wstring& path) {
    const DWORD attr = GetFileAttributesW(path.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool HasNonAscii(const std::wstring& s) {
    for (const wchar_t ch : s) {
        if (ch > 127) return true;
    }
    return false;
}

// 依次在「可执行文件目录及其父目录（最多 4 级）」和「当前工作目录」中查找音乐文件
std::wstring FindMusic(const std::wstring& fileName) {
    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::wstring dir = exePath;
    const size_t slash = dir.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return fileName;
    dir.erase(slash + 1);                      // 保留结尾的反斜杠

    for (int i = 0; i < 5; ++i) {
        if (dir.size() <= 3) break;            // 已到盘根，如 "C:\"
        const std::wstring candidate = dir + fileName;
        if (FileExists(candidate)) return candidate;

        const size_t pos = dir.find_last_of(L"\\/", dir.size() - 2);
        if (pos == std::wstring::npos) break;
        dir.erase(pos + 1);
    }

    if (FileExists(fileName)) return fileName; // 当前工作目录
    return fileName;                           // 原样返回（交给上层判断）
}

// mciSendString 对非 ASCII 路径支持很差。先尝试转成 8.3 短路径；
// 但很多卷（尤其是较新系统上的 NTFS）默认关闭了 8.3 名称生成，
// 这时 GetShortPathNameW 会原样返回中文路径，因此还需备用方案。
std::wstring ToAsciiPath(const std::wstring& path) {
    if (!HasNonAscii(path)) return path;       // 纯 ASCII 路径无需处理

    // 方案 A：8.3 短路径
    wchar_t shortBuf[MAX_PATH] = { 0 };
    const DWORD n = GetShortPathNameW(path.c_str(), shortBuf, MAX_PATH);
    if (n > 0 && n < MAX_PATH && !HasNonAscii(shortBuf)) {
        return std::wstring(shortBuf);
    }

    // 方案 B：复制到 %TEMP% 下的纯 ASCII 路径
    wchar_t tmpDir[MAX_PATH] = { 0 };
    if (GetTempPathW(MAX_PATH, tmpDir) > 0) {
        const std::wstring dst = std::wstring(tmpDir) + L"ChongYang_bgm.mp3";
        if (CopyFileW(path.c_str(), dst.c_str(), FALSE)) {
            printf("[Audio] 检测到非 ASCII 路径，已复制到 %ls 播放\n", dst.c_str());
            g_tempFile = dst;
            return dst;
        }
    }

    printf("[Audio] 无法把音乐路径转为纯 ASCII（8.3 生成已关闭且临时副本失败），"
           "mciSendString 可能无法播放：%ls\n", path.c_str());
    return path;
}

void PrintMciError(MCIERROR err, const wchar_t* stage) {
    wchar_t msg[256] = { 0 };
    mciGetErrorStringW(err, msg, 256);
    printf("[Audio] %ls失败（错误 %lu）：%ls\n", stage, (unsigned long)err, msg);
}

} // namespace

bool Audio::Init(const std::wstring& fileName) {
    if (g_opened) return true;

    const std::wstring found = FindMusic(fileName);
    if (!FileExists(found) && found == fileName) {
        printf("[Audio] 未找到音乐文件 %ls，将静默运行（不影响其他功能）\n", fileName.c_str());
        return false;
    }

    const std::wstring full = ToAsciiPath(found);
    printf("[Audio] 背景音乐路径：%ls\n", full.c_str());

    // 第一次尝试：显式指定 MPEG 设备；失败则让 MCI 自行选择设备
    MCIERROR err = mciSendString((L"open \"" + full + L"\" type mpegvideo alias " + kAlias).c_str(),
                                 nullptr, 0, nullptr);
    if (err != 0) {
        err = mciSendString((L"open \"" + full + L"\" alias " + kAlias).c_str(),
                            nullptr, 0, nullptr);
    }
    if (err != 0) {
        PrintMciError(err, L"打开音乐");
        return false;
    }

    SetVolume(g_volume);
    g_opened = true;
    return true;
}

void Audio::PlayBGM(bool loop) {
    if (!g_opened) return;

    mciSendString((std::wstring(L"seek ") + kAlias + L" to start").c_str(), nullptr, 0, nullptr);
    const std::wstring cmd = loop ? (std::wstring(L"play ") + kAlias + L" repeat")
                                  : (std::wstring(L"play ") + kAlias);
    const MCIERROR err = mciSendString(cmd.c_str(), nullptr, 0, nullptr);
    if (err != 0) {
        PrintMciError(err, L"播放音乐");
        return;
    }
    g_playing = true;
}

void Audio::StopBGM() {
    if (!g_opened) return;
    mciSendString((std::wstring(L"stop ") + kAlias).c_str(), nullptr, 0, nullptr);
    g_playing = false;
}

void Audio::Shutdown() {
    if (g_opened) {
        mciSendString((std::wstring(L"close ") + kAlias).c_str(), nullptr, 0, nullptr);
        g_opened  = false;
        g_playing = false;
    }
    // 清掉临时副本，避免残留
    if (!g_tempFile.empty()) {
        DeleteFileW(g_tempFile.c_str());
        g_tempFile.clear();
    }
}

bool Audio::IsPlaying() {
    if (!g_opened) return false;

    wchar_t buf[32] = { 0 };
    const MCIERROR err = mciSendString((std::wstring(L"status ") + kAlias + L" mode").c_str(),
                                       buf, 32, nullptr);
    if (err != 0) return g_playing;

    g_playing = (std::wstring(buf) == L"playing");
    return g_playing;
}

void Audio::SetVolume(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    g_volume = percent;

    if (!g_opened) return;
    const int level = percent * 10;   // MCI 音量范围 0 - 1000
    mciSendString((std::wstring(L"setaudio ") + kAlias + L" volume to " + std::to_wstring(level)).c_str(),
                  nullptr, 0, nullptr);
}
