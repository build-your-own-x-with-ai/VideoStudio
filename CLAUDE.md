# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

VideoStudio is a professional video stream analysis tool built with C++ Qt and FFmpeg. It provides comprehensive codec analysis capabilities including bitrate visualization, frame-level inspection, and quality metrics for H.264/AVC, H.265/HEVC, VP9, AV1, and MPEG formats.

Inspired by Elecard StreamEye, VideoStudio aims to provide similar professional-grade video analysis capabilities as an open foundation.

## Project Structure

```
VideoStudio/
├── CMakeLists.txt              # CMake build configuration
├── src/
│   ├── main.cpp                # Application entry point
│   ├── mainwindow.h/cpp        # Main window
│   ├── core/                   # Core engine
│   │   ├── videodecoder.h/cpp  # FFmpeg decoder wrapper
│   │   └── framedata.h/cpp     # Frame data structures
│   └── widgets/                # Custom widgets
│       ├── videooutput.h/cpp   # Video display widget
│       └── barchart.h/cpp      # Bitrate chart visualization
├── build/                      # Build directory (generated)
└── resources/                  # Application resources
```

## Build Instructions

### Prerequisites
- Qt 6.11.0 installed at ~/Qt/6.11.0/macos
- FFmpeg 7.1.1+ via Homebrew
- CMake 3.16+
- C++17 compiler (Clang on macOS)

### Building

```bash
# Configure
cd /Users/i/Code/VideoStudio
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=~/Qt/6.11.0/macos

# Build
cmake --build .

# Run
./VideoStudio.app/Contents/MacOS/VideoStudio
```

### Opening in Qt Creator

```bash
~/Qt/Qt\ Creator.app/Contents/MacOS/Qt\ Creator /Users/i/Code/VideoStudio/CMakeLists.txt
```

## Current Implementation Status

### ✅ Completed (MVP)
- **Core Engine**: VideoDecoder with FFmpeg integration
- **Frame Management**: FrameData structures and FrameIndex
- **Video Display**: VideoOutput widget with YUV to RGB conversion
- **Bitrate Visualization**: BarChart widget showing frame sizes and types
- **Main Window**: Basic UI with menu bar, toolbar, and playback controls
- **Navigation**: Play, pause, step forward/backward, seek by clicking chart
- **File Support**: Open video files (MP4, MKV, AVI, MOV, H.264, H.265, etc.)

### 🚧 In Progress
- Stream info panel
- Thumbnail navigation bar
- Quality metrics (PSNR, SSIM, VMAF)

### 📋 Planned
- AreaChart widget for bitstream distribution
- Message panel for compliance verification
- Buffer panel for CPB analysis
- Hex viewer for raw bitstream
- Export capabilities (YUV, CSV, screenshots)
- Overlay rendering (motion vectors, block boundaries, partitions)
- Reference stream comparison
- Advanced navigation (key frame only, frame type filtering)
- Settings dialog with persistent configuration

## Key Features

### Video Decoder (src/core/videodecoder.h/cpp)
- Multi-codec support via FFmpeg (H.264, H.265, VP9, AV1, MPEG-1/2)
- Frame indexing during file open
- Seek to frame number or timestamp
- Extract frame metadata (type, size, PTS, DTS, QP, bitrate)

### Video Output (src/widgets/videooutput.h/cpp)
- Display decoded frames with aspect ratio preservation
- YUV to RGB conversion using swscale
- Automatic scaling to fit widget

### BarChart (src/widgets/barchart.h/cpp)
- Visual representation of frame sizes
- Color-coded by frame type (red=I, blue=P, green=B)
- Click to seek to specific frame
- Current frame indicator

### Main Window (src/mainwindow.h/cpp)
- File menu: Open video files
- Navigation menu: Play, pause, step forward/backward
- Toolbar with quick access buttons
- Status bar showing video info and current frame

## Development Guidelines

### Code Style
- Follow Qt coding conventions
- Use Qt signals/slots for event handling
- Namespace: `VideoStudio`
- C++17 standard
- Smart pointers for resource management

### Adding New Features
1. Create header/source files in appropriate directory (core/, widgets/, panels/, dialogs/)
2. Add files to CMakeLists.txt SOURCES and HEADERS lists
3. Use Qt's MOC for QObject-derived classes (Q_OBJECT macro)
4. Connect signals/slots in MainWindow or parent widget
5. Update this CLAUDE.md file

### FFmpeg Integration
- Always check return values from FFmpeg functions
- Use av_frame_alloc/av_frame_free for frames
- Use av_packet_alloc/av_packet_free for packets
- Call av_packet_unref after processing packets
- Flush decoder buffers when seeking

### Qt Best Practices
- Use Qt containers (QVector, QString, etc.) for Qt integration
- Emit signals for cross-component communication
- Use layouts for responsive UI
- Call update() to trigger repaints

## Reference Documentation

- **EStreamEye_UG_Mac.pdf** - Feature reference for Elecard StreamEye
- **EStreamAnalyzer_UG_Mac.pdf** - Additional analysis features
- **Elecard_Analyzers_zh_Datasheet.pdf** - Chinese datasheet

## Common Tasks

### Adding a New Widget
1. Create `src/widgets/mywidget.h` and `src/widgets/mywidget.cpp`
2. Inherit from QWidget and add Q_OBJECT macro
3. Add to CMakeLists.txt
4. Instantiate in MainWindow and add to layout

### Adding a New Panel
1. Create `src/panels/mypanel.h` and `src/panels/mypanel.cpp`
2. Inherit from QWidget or QDockWidget
3. Add to CMakeLists.txt
4. Add to MainWindow as dockable panel

### Debugging
- Use qDebug() for logging
- Check FFmpeg error codes with av_strerror()
- Use Qt Creator's debugger
- Enable verbose FFmpeg logging: av_log_set_level(AV_LOG_DEBUG)
