# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

VideoStudio is a professional video stream analysis tool built with C++ Qt and FFmpeg. It provides comprehensive codec analysis capabilities including:
- **Video Analysis**: Bitrate visualization, frame-level inspection, GOP structure, YUV export
- **Transport Stream Analysis**: MPEG-TS packet inspection, TR 101-290 compliance, PSI/SI table parsing
- **Quality Metrics**: PSNR/SSIM comparison, frame metadata, timing analysis (PTS/DTS/PCR)

Inspired by Elecard StreamEye and EStreamAnalyzer, VideoStudio provides professional-grade video analysis as an open foundation.

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

## Architecture

### Dual-Mode Design

VideoStudio operates in two modes based on file type:

1. **Video Mode** (MP4, MKV, AVI, MOV, raw H.264/H.265):
   - Uses VideoDecoder (FFmpeg) for frame-by-frame decoding
   - Displays video output, bar chart, GOP viewer, thumbnails
   - Frame navigation and playback controls

2. **Transport Stream Mode** (.ts, .m2ts, .mts):
   - Uses TSParser for packet-level analysis
   - Displays TS Explorer, Packet List, Property Panel, Hex Viewer
   - TR 101-290 compliance checking
   - If video decoder succeeds, both modes are active

### Core Components

**Video Analysis:**
- `VideoDecoder` (src/core/videodecoder.cpp): FFmpeg integration for decoding
- `FrameIndex` (src/core/framedata.cpp): Frame metadata storage and statistics
- `VideoOutput` (src/widgets/videooutput.cpp): Video display with YUV to RGB conversion
- `BarChart` (src/widgets/barchart.cpp): Bitrate visualization
- `GOPViewer` (src/widgets/gopviewer.cpp): GOP structure visualization
- `ThumbnailBar` (src/widgets/thumbnailbar.cpp): Frame thumbnail navigation
- `QualityMetricsDialog` (src/dialogs/qualitymetricsdialog.cpp): PSNR/SSIM comparison tool

**Transport Stream Analysis:**
- `TSParser` (src/core/tsparser.cpp): MPEG-TS packet parser, PSI/SI table extraction
- `TR101290Data` (src/core/tr101290data.h): TR 101-290 error type definitions
- `ExplorerPanel` (src/panels/explorerpanel.cpp): Hierarchical stream structure tree
- `PacketView` (src/widgets/packetview.cpp): Packet list with color-coded types
- `PropertyPanel` (src/panels/propertypanel.cpp): Packet details viewer with multiple modes:
  - Sync mode: Display current packet/atom properties
  - Compare mode: Compare with previous packet/atom
  - Dump mode: Export elementary stream data
  - Detailed PSI/SI table parsing (PAT, PMT, SDT with full field extraction)
- `HexViewerPanel` (src/panels/hexviewerpanel.cpp): Hex/binary viewer with search
- `TR101290Panel` (src/panels/tr101290panel.cpp): Compliance validation (3 priority levels)
- `TimeDynamicsPanel` (src/panels/timedynamicspanel.cpp): PTS/DTS/PCR timing analysis
- `MessagesPanel` (src/panels/messagespanel.cpp): Aggregated error/warning viewer
- `BitratePanel` (src/panels/bitratepanel.cpp): Per-PID bitrate charts

**Main Window:**
- `MainWindow` (src/mainwindow.cpp): Application window, manages all panels and widgets
- Dockable panels for flexible layout
- Signal/slot connections between components

### Key Data Flow

**Opening a TS File:**
1. User opens .ts file → `MainWindow::openFile()`
2. Attempt video decode with `VideoDecoder::openFile()` (may fail for corrupted streams)
3. If TS file detected, parse with `TSParser::parseFile()`
4. Update all TS panels: Explorer, Property, Hex Viewer, Packet View, Messages, TR 101-290, Time Dynamics, Bitrate
5. Connect signals: packet selection → property/hex viewer updates

**Packet Selection Flow:**
1. User clicks packet in PacketView → `PacketView::packetSelected(int)` signal
2. PropertyPanel receives signal → displays packet details
3. HexViewerPanel receives signal → displays hex dump of packet
4. User can search hex bytes, toggle binary mode, adjust bytes per row

**Error Detection Flow:**
1. TSParser analyzes stream → stores packets, PIDs, programs
2. TR101290Panel runs compliance checks → generates error list
3. MessagesPanel fetches errors from TR101290Panel → displays with filtering
4. User double-clicks error → jumps to packet in PacketView

**Quality Metrics Comparison:**
1. User opens reference video → `MainWindow::openFile()`
2. Tools → Quality Metrics (Ctrl+Q) → `QualityMetricsDialog`
3. User selects distorted video for comparison
4. Dialog decodes both videos frame-by-frame
5. Calculates PSNR (MSE on Y plane) and SSIM (8x8 block analysis)
6. Displays average metrics with quality interpretation

## Current Implementation Status

### ✅ Completed

**Video Analysis (MVP):**
- Core video decoding with FFmpeg
- Frame-by-frame navigation and playback
- Bitrate visualization (bar chart)
- GOP structure viewer
- Thumbnail navigation bar
- Video output with zoom
- Frame metadata extraction
- YUV frame export (single frame and frame range)
- Quality metrics analysis (PSNR/SSIM comparison between reference and distorted videos)
- CSV metrics export (frame list, statistics, GOP structure, bitrate data)

**Transport Stream Analysis (EStreamAnalyzer):**
- TS packet parser with PSI/SI table extraction
- Explorer panel (hierarchical stream structure)
- Packet view (color-coded packet list, 1000 packet limit for performance)
- Property panel (Sync/Compare/Dump modes)
- Hex viewer panel (hex/binary modes, search functionality)
- TR 101-290 compliance validation (3 priority levels, 18+ error types)
- Time Dynamics panel (PTS/DTS/PCR analysis with charts)
- Messages panel (aggregated errors with filtering)
- Bitrate panel (per-PID bitrate charts)

**Container Format Support:**
- MP4/MOV parser with atom/box hierarchy
- MKV/WebM parser with EBML structure
- AVI parser with RIFF chunks
- FLV parser with tag structure

**User Interface:**
- About dialog with help documentation and README viewer
- Dockable panels for flexible layout
- Real-time log viewer with animations
- YUV Viewer dialog (open and analyze raw YUV files with format parameters)
- Graphics panel (multi-parameter comparison charts with Line/Bar/Transition modes)
  - Zoom in/out and fit controls
  - Pan/drag navigation after zoom
  - Hover tooltip showing parameter values at data points
  - Independent Y-axis scaling per parameter (Global/Independent modes)
- Comments panel (collaborative annotations with XML export)
- EPG panel (Electronic Program Guide from EIT tables)
- Explorer panel context menu (Dump ES, Compare mode, Sync mode)

**Quality Metrics:**
- VMAF (Video Multimethod Assessment Fusion) integration

**Advanced Video Analysis:**
- Motion vector overlay visualization

### 📋 Planned

- Block/partition visualization (requires codec-specific block information)
- Reference stream comparison with difference modes
- Command line tool for batch processing

## Development Guidelines

### Code Style
- Follow Qt coding conventions
- Use Qt signals/slots for event handling
- Namespace: `VideoStudio`
- C++17 standard
- Smart pointers for resource management (`std::unique_ptr`, `std::make_unique`)
- Use Qt containers (QVector, QString, QMap) for Qt integration

### Adding New Features

**New Widget:**
1. Create `src/widgets/mywidget.h` and `src/widgets/mywidget.cpp`
2. Inherit from QWidget, add Q_OBJECT macro
3. Add to CMakeLists.txt SOURCES and HEADERS
4. Instantiate in MainWindow, add to layout

**New Panel:**
1. Create `src/panels/mypanel.h` and `src/panels/mypanel.cpp`
2. Inherit from QWidget
3. Add to CMakeLists.txt
4. Create QDockWidget in MainWindow, set panel as widget
5. Add to View menu for show/hide

**New Core Component:**
1. Create `src/core/mycomponent.h` and `src/core/mycomponent.cpp`
2. Add to CMakeLists.txt
3. Instantiate in MainWindow or appropriate owner

### Signal/Slot Connections

Use `Qt::UniqueConnection` flag when connecting signals in MainWindow to prevent duplicate connections when reopening files:

```cpp
connect(m_packetView, &PacketView::packetSelected,
        m_propertyPanel, &PropertyPanel::displayPacket, Qt::UniqueConnection);
```

### FFmpeg Integration
- Always check return values from FFmpeg functions
- Use av_frame_alloc/av_frame_free for frames
- Use av_packet_alloc/av_packet_free for packets
- Call av_packet_unref after processing packets
- Flush decoder buffers when seeking

### Transport Stream Parsing

**TSParser Usage:**
```cpp
TSParser* parser = new TSParser();
if (parser->parseFile(filename)) {
    const auto& packets = parser->getPackets();  // QVector<TSPacket>
    const auto& pids = parser->getPIDs();        // QMap<uint16_t, PIDInfo>
    const auto& programs = parser->getPrograms(); // QVector<ProgramInfo>
}
```

**TR 101-290 Compliance:**
- First Priority: Critical errors (sync loss, PAT/PMT errors, continuity count)
- Second Priority: Recommended checks (transport errors, CRC, PCR timing)
- Third Priority: Application level (NIT/SDT/EIT presence, SI repetition)

### Performance Considerations

- **Packet View**: Limited to 1000 packets for UI performance (configurable)
- **Thumbnail Generation**: Background thread to avoid blocking UI
- **Large Files**: TS parser loads entire file into memory - consider streaming for very large files
- **Hex Viewer**: Only displays current packet (188 bytes), not entire stream

### Qt Best Practices
- Use layouts for responsive UI (QVBoxLayout, QHBoxLayout, QSplitter)
- Call update() to trigger repaints
- Use QTimer for playback timing
- Emit signals for cross-component communication
- Use QTreeWidget for hierarchical data (Explorer, TR 101-290, Messages)
- Use QTextEdit with monospace font for hex viewer

## Common Tasks

### Adding a TR 101-290 Check

1. Add error type to `TR101290ErrorType` enum in `src/core/tr101290data.h`
2. Implement check method in `TR101290Panel` (e.g., `checkMyError()`)
3. Call check method in `analyzeStream()`
4. Add error type to Messages Panel filter list in `messagespanel.cpp`

### Adding a New Panel to Main Window

```cpp
// In MainWindow::createDockWidgets()
m_myPanel = new MyPanel(this);
m_myPanelDock = new QDockWidget("My Panel", this);
m_myPanelDock->setWidget(m_myPanel);
addDockWidget(Qt::RightDockWidgetArea, m_myPanelDock);

// Add to View menu
QAction* myPanelAction = m_myPanelDock->toggleViewAction();
viewMenu->addAction(myPanelAction);
```

### Connecting Panel to TS Parser

```cpp
// In MainWindow::openFile() after TS parsing
if (isTSFile && m_tsParser->parseFile(fileName)) {
    m_myPanel->setTSParser(m_tsParser.get());
}
```

### Debugging

- Use qDebug() for logging: `qDebug() << "Packet count:" << packets.size();`
- Check FFmpeg error codes: `char errbuf[128]; av_strerror(ret, errbuf, sizeof(errbuf));`
- Use Qt Creator's debugger with breakpoints
- Enable verbose FFmpeg logging: `av_log_set_level(AV_LOG_DEBUG);`
- Check TS packet sync byte (0x47) when debugging parser issues

### CSV Export Usage

Export frame-level metrics and statistics:

```cpp
// In MainWindow::exportCSVMetrics()
CSVExportDialog dialog(m_decoder.get(), this);
dialog.exec();
```

**Exported data includes:**
- Frame list: frame number, type, size, PTS, DTS, offset, QP, keyframe flag, bitrate, timestamp
- Statistics: total frames, I/P/B counts, average bitrate, max/min frame size
- GOP structure: GOP number, start/end frames, length, frame type distribution
- Bitrate data: per-frame instantaneous bitrate

**Format options:**
- Delimiter: comma, semicolon, or tab
- Decimal separator: dot or comma
- Frame range selection

### Quality Metrics Testing

To test PSNR/SSIM functionality, create a compressed version of a reference video:

```bash
# Create low-quality compressed version (CRF 35)
ffmpeg -i reference.mp4 -c:v libx264 -crf 35 -preset fast compressed.mp4

# Expected results:
# - PSNR: 25-35 dB (lower CRF = higher quality)
# - SSIM: 0.85-0.95 (structural similarity)
```

**Important**: Compare the same video content at different quality levels. Comparing different videos will produce meaningless metrics (PSNR <15 dB, SSIM <0.3).

## Reference Documentation

Located in project root:
- **EStreamEye_UG_Mac.pdf** - Feature reference for Elecard StreamEye (video analysis)
- **EStreamAnalyzer_UG_Mac.pdf** - Feature reference for EStreamAnalyzer (TS analysis)
- **Elecard_Analyzers_zh_Datasheet.pdf** - Chinese datasheet

## Known Limitations

- Packet View displays max 1000 packets (performance limit)
- TS Parser loads entire file into memory
- Video decoder may fail on corrupted TS files (expected - use TS analysis mode)
- Hex Viewer search only highlights in current packet view
- No undo/redo functionality
- No project save/load (session state not persisted)
