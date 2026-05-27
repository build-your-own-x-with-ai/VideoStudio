# VideoStudio

专业视频编解码分析工具

## 简介

VideoStudio 是一个基于 Qt/C++ 和 FFmpeg 开发的专业视频编解码分析工具，面向视频工程师和质量分析师。它能够深度分析视频流的编解码参数、比特率、帧类型、GOP 结构等专业指标。

## 功能特性

### 当前版本 (v1.0)
- ✅ 打开和解析多种视频格式（MP4, MKV, AVI, MOV, FLV, WMV, WebM）
- ✅ 显示详细的视频流信息
  - 编码器名称和详细信息
  - 分辨率和帧率
  - 比特率
  - 时长和总帧数
  - 像素格式
  - 容器格式
- ✅ 视频预览（显示第一帧）
- ✅ **帧分析功能（Phase 2 新增）**
  - 帧列表视图（帧号、类型、大小、时间戳、PTS、DTS）
  - I/P/B 帧类型识别
  - 帧筛选功能（显示全部/仅 I 帧/仅 P 帧/仅 B 帧）
  - 帧统计信息（总帧数、各类型帧数量）
  - 帧选择功能（点击帧显示详细信息）
- ✅ 现代化的用户界面

### 计划功能
- 📊 比特率分析（Phase 3）
  - 实时比特率曲线图
  - 平均/峰值比特率统计
- 🎯 GOP 结构分析（Phase 4）
  - GOP 结构可视化
  - GOP 长度统计
- 🚀 高级功能（Phase 5）
  - 质量指标分析
  - 分析报告导出
  - 多视频对比

## 技术栈

- **UI 框架**: Qt 6.11.0
- **视频处理**: FFmpeg 7.1.1
- **构建系统**: CMake 4.0.2
- **编程语言**: C++17

## 系统要求

- macOS 10.15+（当前版本）
- Qt 6.x 或 Qt 5.15+
- FFmpeg 4.x 或更高版本
- CMake 3.16+

## 构建说明

### 安装依赖

```bash
# macOS
brew install ffmpeg qt6 cmake
```

### 编译项目

```bash
# 创建构建目录
mkdir build
cd build

# 配置 CMake
cmake ..

# 编译
make

# 运行
open VideoStudio.app
```

## 使用说明

1. 启动应用程序
2. 点击"文件" → "打开视频..."或使用工具栏按钮
3. 选择要分析的视频文件
4. 在"概览"标签页查看详细的流信息
5. 在"帧分析"标签页查看所有帧的详细信息
   - 使用筛选按钮查看特定类型的帧
   - 点击帧查看详细信息
6. 视频预览区域显示第一帧画面

## 项目结构

```
VideoStudio/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 项目说明
├── src/
│   ├── main.cpp                # 程序入口
│   ├── core/                   # 核心功能模块
│   │   ├── VideoDecoder.h/cpp  # FFmpeg 视频解码封装
│   │   ├── MetricsCollector.h/cpp # 指标收集器
│   │   ├── FrameInfo.h         # 帧信息数据结构
│   │   └── StreamInfo.h        # 流信息数据结构
│   └── ui/                     # UI 组件
│       ├── MainWindow.h/cpp    # 主窗口
│       ├── StreamInfoPanel.h/cpp # 流信息面板
│       └── FrameListView.h/cpp # 帧列表视图
├── resources/                  # 资源文件
└── build/                      # 构建目录
```

## 开发路线图

- [x] Phase 1: 基础框架（MVP）
- [x] Phase 2: 帧分析功能
- [ ] Phase 3: 比特率分析
- [ ] Phase 4: GOP 结构分析
- [ ] Phase 5: 高级功能

## 许可证

本项目仅供学习和研究使用。

## 致谢

- [Qt](https://www.qt.io/) - 跨平台 UI 框架
- [FFmpeg](https://ffmpeg.org/) - 多媒体处理库
