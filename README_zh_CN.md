# VideoStudio

[English](README.md) | [简体中文](README_zh_CN.md)

使用 C++ Qt 和 FFmpeg 构建的专业视频流分析工具。

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey.svg)
![Qt](https://img.shields.io/badge/Qt-6.5.3+-green.svg)
![FFmpeg](https://img.shields.io/badge/FFmpeg-4.x%20%7C%205.x%20%7C%206.x%20%7C%207.x-orange.svg)
![Version](https://img.shields.io/badge/version-2.0.0-blue.svg)

## 概述

VideoStudio 是一款综合性视频编解码分析应用，灵感来自 Elecard StreamEye。它提供专业级工具用于调试编码问题、验证编解码器合规性、分析质量指标以及检查帧级细节。

**🎉 版本 2.0 发布亮点：**
- ✅ **跨平台支持**：现已支持 Windows、macOS 和 Linux
- ✅ **自动化 CI/CD**：通过 GitHub Actions 提供所有平台的预构建二进制文件
- ✅ **合规性验证**：H.264/H.265 比特流语法验证
- ✅ **缓冲分析**：HRD/VBV 验证和 CPB 监控
- ✅ **插件系统**：可扩展架构，支持自定义分析器
- ✅ **音频分析**：全面的音频流分析和波形可视化
- ✅ **增强稳定性**：FFmpeg 4.x-7.x 兼容层

**提供两种模式：**
- **GUI 应用程序**：功能齐全的基于 Qt 的桌面应用程序，支持交互式分析
- **CLI 工具**：用于批处理和自动化的命令行界面（参见 [CLI 文档](docs/CLI.md)）

## 下载

**预构建二进制文件（推荐）：**

访问 [Releases](https://github.com/build-your-own-x-with-ai/VideoStudio/releases) 页面下载预构建包：
- **Windows**: VideoStudio-Windows-x64.zip
- **macOS**: VideoStudio-macOS-x64.tar.gz  
- **Linux**: VideoStudio-Linux-x64.tar.gz

**备用下载：**

链接: <https://pan.baidu.com/s/1hjvHDnAHsCuQhaC5K1LMeQ?pwd=3kxq> 提取码:3kxq

### 核心特性

#### 视频分析
- **多编解码器支持**: H.264/AVC、H.265/HEVC、VP9、AV1、MPEG-1/2、MPEG-4、MJPEG、ProRes
- **容器格式支持**: MP4、MKV、AVI、FLV、TS/M2TS、MOV、WebM
- **原始YUV查看器**: 打开和分析原始YUV文件，自动检测格式（I420、NV12、YV12等）
- **逐帧分析**: 精确的逐帧导航
- **视频播放**: 播放、暂停、精确定位
- **帧元数据**: 提取PTS、DTS、帧类型、大小、QP值、GOP结构

#### 质量与导出
- **质量指标**: 参考视频与压缩视频之间的PSNR/SSIM/VMAF比较
- **重复帧检测**: 识别和分析连续重复帧，可配置相似度阈值
- **YUV导出**: 导出单帧或帧范围为原始YUV文件
- **CSV导出**: 导出帧指标、统计数据、GOP结构和码率数据
- **流信息导出**: 保存详细的流信息到文本文件

#### 分析面板
- **码率面板**: 交互式可视化显示帧大小、类型和码率分布
- **传输流分析**: MPEG-TS包检查、TR 101-290合规性、PSI/SI表
- **容器结构**: 详细的MP4原子、MKV元素、AVI块和TS包检查
- **十六进制查看器**: 原始字节级别文件检查
- **时间动态**: PTS/DTS/PCR时间线分析
- **GOP查看器**: GOP结构可视化，带依赖关系箭头
- **缩略图栏**: 快速帧导航，带视觉缩略图
- **消息面板**: 编解码器警告、错误和合规问题
- **EPG面板**: 来自传输流的电子节目指南数据
- **缓冲分析**: CPB（编码图像缓冲）监控，支持 HRD/VBV 验证
- **音频分析**: 波形、频谱、响度（LUFS）和相位表可视化
- **块统计**: 运动矢量叠加和分区可视化

#### 合规性与验证（2.0 新增）
- **H.264/H.265 合规性**: 根据 ITU-T 规范进行比特流语法验证
- **配置与级别验证**: 自动检测和验证编解码器配置
- **缓冲模型验证**: HRD（假设参考解码器）验证
- **语法错误检测**: 实时识别比特流违规

#### 用户界面
- **专业界面**: 基于Qt的界面，带可停靠面板和流畅动画
- **国际化**: 完全支持英文和简体中文
- **双主题支持**: 为视频分析优化的现代深色主题和清爽浅色主题
- **实时日志查看器**: 实时显示文件加载和解析进度
- **上下文菜单**: 右键点击帧分析和导出选项
- **可自定义布局**: 多种预定义工作区布局，适用于不同的分析工作流程
- **插件系统**（2.0 新增）：可扩展架构，支持使用 C++ API 的自定义分析插件

### 计划功能

- 参考流比较，支持并排差异模式
- 高级运动矢量分析工具
- 场景变化检测和分析

## 系统要求

### 所有平台
- **Qt**: 6.5.3 或更高版本（推荐 6.11.0 用于开发）
- **FFmpeg**: 4.x、5.x、6.x 或 7.x（自动兼容层）
- **CMake**: 3.16 或更高版本
- **C++17 编译器**: Clang、GCC 或 MSVC

### 平台特定要求
- **macOS**: 10.15 (Catalina) 或更高版本
- **Windows**: Windows 10 或更高版本，Visual Studio 2019+
- **Linux**: Ubuntu 20.04+ 或同等版本（glibc 2.31+）

## 安装

### 方式 1：下载预构建二进制文件（推荐）

从 [GitHub Releases](https://github.com/build-your-own-x-with-ai/VideoStudio/releases/tag/v2.0.0) 下载最新版本：

- **Windows**: 解压 `VideoStudio-Windows-x64.zip` 并运行 `VideoStudio.exe`
- **macOS**: 解压 `VideoStudio-macOS-x64.tar.gz` 并运行 `VideoStudio.app`
- **Linux**: 解压 `VideoStudio-Linux-x64.tar.gz` 并运行 `./VideoStudio`

### 方式 2：从源码构建

#### macOS

```bash
# 安装 Homebrew（如果尚未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装 FFmpeg
brew install ffmpeg

# 从 https://www.qt.io/download 下载并安装 Qt 6.11.0

# 克隆并构建
git clone https://github.com/build-your-own-x-with-ai/VideoStudio.git
cd VideoStudio
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=~/Qt/6.11.0/macos
cmake --build .
./VideoStudio.app/Contents/MacOS/VideoStudio
```

#### Linux (Ubuntu/Debian)

```bash
# 安装依赖
sudo apt update
sudo apt install -y build-essential cmake git
sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
sudo apt install -y qt6-base-dev qt6-multimedia-dev qt6-tools-dev

# 克隆并构建
git clone https://github.com/build-your-own-x-with-ai/VideoStudio.git
cd VideoStudio
mkdir build && cd build
cmake ..
cmake --build .
./VideoStudio
```

#### Windows

```bash
# 通过 Chocolatey 安装依赖
choco install cmake git

# 下载 Qt 和 FFmpeg：
# - Qt: https://www.qt.io/download
# - FFmpeg: https://github.com/BtbN/FFmpeg-Builds/releases

# 克隆并构建
git clone https://github.com/build-your-own-x-with-ai/VideoStudio.git
cd VideoStudio
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64" -DFFMPEG_DIR="C:/ffmpeg"
cmake --build . --config Release
./Release/VideoStudio.exe
```

```bash
# 克隆仓库
git clone https://github.com/build-your-own-x-with-ai/VideoStudio.git
cd VideoStudio

# 创建构建目录
mkdir build && cd build

# 使用 CMake 配置
cmake .. -DCMAKE_PREFIX_PATH=~/Qt/6.11.0/macos

# 构建
cmake --build .

# 运行
./VideoStudio.app/Contents/MacOS/VideoStudio
```

## 使用方法

### 打开文件

#### 视频文件
1. 启动 VideoStudio
2. 点击 **文件 → 打开** 或按 **Cmd+O**
3. 选择视频文件（MP4、MKV、AVI、MOV、TS、FLV、WebM、H.264、H.265等）
4. 在 **日志查看器** 中观察实时加载进度
5. 加载完成后，显示第一帧并提供分析面板

### 导航

- **播放/暂停**: 点击播放按钮或按 **空格键**
- **前进一帧**: 点击前进按钮或按 **Alt+右箭头**
- **后退一帧**: 点击后退按钮或按 **Alt+左箭头**
- **定位**: 点击条形图或缩略图跳转到特定帧

### 质量指标分析

比较参考视频和压缩版本之间的视频质量：

1. 打开参考视频文件（文件 → 打开）
2. 转到 **工具 → 质量指标（PSNR/SSIM）** 或按 **Ctrl+Q**
3. 选择要比较的压缩视频
4. 配置帧范围和要计算的指标
5. 配置要计算的指标：
   - **PSNR**: 峰值信噪比（传统质量指标）
   - **SSIM**: 结构相似性指数（感知质量）
   - **VMAF**: 视频多方法评估融合（Netflix的感知质量指标）
6. 点击"开始分析"进行逐帧比较
7. 查看结果：
   - **PSNR**: >40 dB（优秀），30-40 dB（良好），<30 dB（差）
   - **SSIM**: >0.95（优秀），0.85-0.95（良好），<0.85（差）
   - **VMAF**: >80（优秀），60-80（良好），<60（差）

### 导出YUV帧

导出原始YUV帧用于外部分析：

1. 打开视频文件
2. 导航到所需帧
3. **文件 → 导出帧为YUV**（Ctrl+E）- 导出当前帧
4. **文件 → 导出帧范围为YUV**（Ctrl+Shift+E）- 导出多帧
5. 选择输出位置和格式

### 导出CSV指标

将帧级别指标和统计信息导出为CSV格式：

1. 打开视频文件
2. **文件 → 导出CSV指标**（Ctrl+M）
3. 选择输出文件位置
4. 选择要导出的数据：
   - **帧列表**: 帧号、类型、大小、PTS、DTS、QP、码率、时间戳
   - **统计信息**: 总帧数、I/P/B帧计数、平均码率、最大/最小帧大小
   - **GOP结构**: GOP编号、起始/结束帧、长度、帧类型分布
   - **码率数据**: 每帧瞬时码率

## 架构

### 核心组件

**视频分析：**
- `VideoDecoder`: FFmpeg 集成解码
- `FrameIndex`: 帧元数据存储和统计
- `VideoOutput`: 视频显示，YUV到RGB转换
- `BarChart`: 码率可视化
- `GOPViewer`: GOP结构可视化
- `ThumbnailBar`: 帧缩略图导航

**传输流分析：**
- `TSParser`: MPEG-TS包解析器，PSI/SI表提取
- `TR101290Data`: TR 101-290错误类型定义
- `ExplorerPanel`: 层次流结构树
- `PacketView`: 包列表，颜色编码类型
- `PropertyPanel`: 包详细信息查看器
- `HexViewerPanel`: 十六进制/二进制查看器

## 版本路线图

### Version 1.2 (当前版本)
- [x] 原始YUV文件查看器，带格式检测
- [x] 所有分析面板（码率、缓冲、TR 101-290、时间动态等）
- [x] 流信息导出功能
- [x] 参考比较对话框
- [x] CSV指标导出
- [x] 上下文菜单快捷访问
- [x] 完整的可停靠面板UI
- [x] 国际化支持（英文、简体中文）
- [x] 双主题支持（深色/浅色主题）
- [x] GOP依赖关系箭头可视化
- [x] 重复帧检测，带相似度阈值
- [x] 运动矢量叠加
- [x] 块/分区可视化
- [x] MP4/MKV文件的NAL单元列表视图（H.264/H.265）
- [x] VMAF质量指标（视频多方法评估融合）
- [x] 用于批处理和自动化的命令行工具

### Version 2.0 (计划中)
- [x] 合规性验证（H.264/H.265比特流语法验证）
- [x] 高级缓冲分析（HRD/VBV验证）
- [x] 插件系统

## 贡献

欢迎贡献！请随时提交 Pull Request。

1. Fork 仓库
2. 创建功能分支（`git checkout -b feature/AmazingFeature`）
3. 提交更改（`git commit -m 'Add some AmazingFeature'`）
4. 推送到分支（`git push origin feature/AmazingFeature`）
5. 打开 Pull Request

## 许可证

本项目采用 MIT 许可证 - 详见 LICENSE 文件。

## 致谢

- 灵感来自 [Elecard StreamEye](https://www.elecard.com/products/video-analysis/stream-eye)
- 使用 [Qt](https://www.qt.io/) 和 [FFmpeg](https://ffmpeg.org/) 构建
- 感谢开源社区

## 联系方式

- **项目**: [VideoStudio](https://github.com/build-your-own-x-with-ai/VideoStudio)
- **文档**: 参见 [CLAUDE.md](CLAUDE.md) 了解开发指南

---

**注意**: 这是一个处于积极开发中的早期项目。功能和API可能会发生变化。
