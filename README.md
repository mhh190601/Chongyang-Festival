<div align="center">

# 🏮 重阳节 · 全屏沉浸式交互程序 (ChongYang)

![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![EasyX](https://img.shields.io/badge/Library-EasyX-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

**一款结合了广府文化、传统节气与玄学排盘的 C++ 桌面视觉大作。**
**纯代码渲染，无任何外部图片素材。**
</div>

## 📖 项目简介

本项目是一款基于 **C++20** 和 **EasyX 图形库** 开发的纯原生 Windows 桌面全屏程序。以“重阳节”为主题，带你沉浸式体验传统佳节的氛围，感受中式玄学的魅力。

## ✨ 核心功能

- 🏮 **唯美开场**：满天孔明灯缓缓升起、金菊绽放、水墨远山与星空交织，重阳二字如火焰点燃般浮现。
- 🏛️ **古风主菜单**：四周环绕动态折扇、袅袅香炉、梅花与竹简，UI 采用半透明玻璃拟态与流萤粒子。
- ✍️ **祈福祝愿**：调用 Win32 原生弹窗输入自定义祝福语，全屏跑马灯滚动展示。
- ☯️ **八字测卦**：输入生辰后，太极八卦阵狂转，最终砸下“12片竹简+宣纸内衬”的巨型竹签，展示真实的排盘四柱、本卦、变卦与专属签文。
- 🕯️ **孔明灯祈福**：按空格键即可放飞带有物理摇摆和呼吸光晕的孔明灯，升空消散。
- 🕊️ **缅怀先烈**：一键无缝唤起浏览器，访问中华英烈网。

## 🛠️ 技术亮点（纯代码手搓）

1. **纯代码渲染 UI**：没有使用任何 PNG/JPG 图片切图。折扇、香炉、孔明灯、竹简全部由 `fillpolygon`、`solidcircle` 等基础几何图形结合数学函数计算生成。
2. **自研视觉动效**：孔明灯拥有独立的上升速度与正弦左右摇摆；竹简落定后具有阻尼摇摆效果；金光扫过时采用了 30 层切片余弦渐变算法实现平滑光带。
3. **真太阳时八字排盘**：内置天文历法算法，计算二十四节气、真太阳时，生成准确的四柱八字。
4. **Win32 模态对话框**：为了解决 EasyX 无法输入中文的问题，底层手写了一套支持输入法联动、Enter 确认、ESC 取消的原生单行输入框。

## 🚀 如何运行

### 方式一：直接体验（推荐普通用户）
1. 点击本仓库右侧的 **[Releases]** 标签页。
2. 下载最新版本的 `ChongYang_v1.0.0.zip` 压缩包。
3. 解压后，双击 `ChongYang.exe` 即可直接运行（请确保 `bgm.mp3` 与 exe 在同一目录下）。

### 方式二：本地编译（开发者）
**环境要求：**
- 操作系统：Windows 10 / 11
- 编译器：Visual Studio 2022 (需安装 C++ 桌面开发工作负载)
- 图形库：[EasyX 图形库](https://easyx.cn/)（请提前安装至 VS 中）
- 构建工具：CMake 3.20+

**编译指令：**
```bash
# 1. 克隆仓库
git clone https://github.com/你的用户名/你的仓库名.git
cd 你的仓库名

# 2. 生成构建文件 (注意使用 x64 架构)
mkdir build
cd build
cmake -A x64 ..

# 3. 编译 Release 版本
cmake --build . --config Release

# 4. 运行程序
cd Release
./ChongYang.exe
⚠️ 注意事项
背景音乐：程序使用 mciSendString 播放 BGM。请将下载的音乐文件重命名为 bgm.mp3，并放置在可执行文件（.exe）同级的目录下。

操作说明：

菜单切换：↑ ↓ 选择，Enter 确认。

通用返回：按 ESC 返回主菜单。

安全退出：在主菜单选择 [5] 退出 即可安全关闭全屏并退出程序。

📄 开源协议
本项目基于 MIT License 协议开源。欢迎学习与交流。
