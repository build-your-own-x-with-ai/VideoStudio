# VideoStudio

Professional video stream analysis tool built with C++ Qt and FFmpeg.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-macOS-lightgrey.svg)
![Qt](https://img.shields.io/badge/Qt-6.11.0-green.svg)
![FFmpeg](https://img.shields.io/badge/FFmpeg-7.1.1-orange.svg)

## Overview

VideoStudio is a comprehensive video codec analysis application inspired by Elecard StreamEye. It provides professional-grade tools for debugging encoding issues, validating codec compliance, analyzing quality metrics, and inspecting frame-level details.

### Key Features

- **Multi-Codec Support**: H.264/AVC, H.265/HEVC, VP9, AV1, MPEG-1/2
- **Container Format Support**: MP4, MKV, AVI, FLV, TS/M2TS
- **Transport Stream Analysis**: MPEG-TS packet inspection, TR 101-290 compliance, PSI/SI tables
- **Bitrate Visualization**: Interactive bar chart showing frame sizes and types
- **Frame-by-Frame Analysis**: Step through video frame by frame
- **Video Playback**: Play, pause, seek with accurate frame positioning
- **Frame Metadata**: Extract PTS, DTS, frame type, size, QP values
- **Quality Metrics**: PSNR/SSIM comparison between reference and distorted videos
- **YUV Export**: Export single frames or frame ranges as raw YUV files
- **Real-time Log Viewer**: Monitor file loading and parsing progress with live log display
- **Container Structure Analysis**: Detailed MP4 atom, MKV element, AVI chunk, and TS packet inspection
- **Professional UI**: Qt-based interface with dockable panels and smooth animations

### Planned Features

- VMAF quality metric integration
- Motion vector visualization
- Block/partition information display
- Reference stream comparison with difference modes
- CSV metrics export
- Compliance verification messages
- Buffer analysis (CPB)
- Command-line tool for batch processing

## Screenshots

*Coming soon*

## Requirements

- **macOS**: 10.15 or later
- **Qt**: 6.11.0 or later
- **FFmpeg**: 7.1.1 or later
- **CMake**: 3.16 or later
- **Compiler**: Clang with C++17 support

## Installation

### Install Dependencies

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install FFmpeg
brew install ffmpeg

# Download and install Qt 6.11.0 from https://www.qt.io/download
```

### Build from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/VideoStudio.git
cd VideoStudio

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_PREFIX_PATH=~/Qt/6.11.0/macos

# Build
cmake --build .

# Run
./VideoStudio.app/Contents/MacOS/VideoStudio
```

## Usage

### Opening a Video File

1. Launch VideoStudio
2. Click **File → Open** or press **Cmd+O**
3. Select a video file (MP4, MKV, AVI, MOV, TS, H.264, H.265, etc.)
4. Watch the **Log Viewer** in the center of the window showing real-time loading progress
5. Once loaded, the Log Viewer animates to the bottom and the first frame is displayed

### Log Viewer

The Log Viewer provides real-time feedback during file operations:
- **Loading Progress**: Shows frame indexing progress (e.g., "Building frame index... 350 / 700 frames (50%)")
- **Container Parsing**: Displays MP4 atom structure, MKV elements, or TS packet analysis
- **Codec Information**: Shows video codec, resolution, frame rate, and stream details
- **Auto-scroll**: Automatically scrolls to show the latest log messages
- **Dockable**: After loading, the Log Viewer moves to the bottom dock area
- **Access**: Click the "Log Viewer" tab at the bottom to view logs anytime

### Navigation

- **Play/Pause**: Click the Play button or press **Space**
- **Step Forward**: Click Step Forward or press **Alt+Right**
- **Step Backward**: Click Step Backward or press **Alt+Left**
- **Seek**: Click on the bar chart or thumbnails to jump to a specific frame

### Quality Metrics Analysis

Compare video quality between reference and distorted versions:

1. Open a reference video file (File → Open)
2. Go to **Tools → Quality Metrics (PSNR/SSIM)** or press **Ctrl+Q**
3. Select the distorted/compressed video to compare
4. Configure frame range and metrics to calculate
5. Click "Start Analysis" to compare frame-by-frame
6. View results:
   - **PSNR**: >40 dB (excellent), 30-40 dB (good), <30 dB (poor)
   - **SSIM**: >0.95 (excellent), 0.85-0.95 (good), <0.85 (poor)

**Note**: Both videos must have the same resolution and should be the same content at different quality levels.

### Export YUV Frames

Export raw YUV frames for external analysis:

1. Open a video file
2. Navigate to the desired frame
3. **File → Export Frame as YUV** (Ctrl+E) - Export current frame
4. **File → Export Frame Range as YUV** (Ctrl+Shift+E) - Export multiple frames
5. Select output location and format

### Export CSV Metrics

Export frame-level metrics and statistics to CSV format:

1. Open a video file
2. **File → Export CSV Metrics** (Ctrl+M)
3. Select output file location
4. Choose data to export:
   - **Frame List**: Frame number, type, size, PTS, DTS, QP, bitrate, timestamp
   - **Statistics**: Total frames, I/P/B frame counts, average bitrate, max/min frame size
   - **GOP Structure**: GOP number, start/end frames, length, frame type distribution
   - **Bitrate Data**: Per-frame instantaneous bitrate
5. Configure frame range (start frame to end frame)
6. Select format options:
   - Delimiter: Comma, Semicolon, or Tab
   - Decimal separator: Dot or Comma
7. Click "Export" to save the CSV file

**Use cases:**
- Import into Excel/Google Sheets for custom analysis
- Generate reports and visualizations
- Compare encoding results across different settings
- Track quality metrics over time

### Bar Chart

The bar chart at the top shows:
- **Bar Height**: Frame size in bytes
- **Bar Color**: Frame type (Red=I-frame, Blue=P-frame, Green=B-frame)
- **Yellow Line**: Key frame indicator
- **Red Vertical Line**: Current frame position

## Architecture

### Core Components

- **VideoDecoder** (`src/core/videodecoder.cpp`): FFmpeg integration for decoding with real-time progress signals
- **FrameIndex** (`src/core/framedata.cpp`): Frame metadata storage and statistics
- **VideoOutput** (`src/widgets/videooutput.cpp`): Video display with YUV to RGB conversion
- **BarChart** (`src/widgets/barchart.cpp`): Bitrate visualization widget
- **GOPViewer** (`src/widgets/gopviewer.cpp`): GOP structure visualization
- **ThumbnailBar** (`src/widgets/thumbnailbar.cpp`): Frame thumbnail navigation
- **LogViewer** (`src/widgets/logviewer.cpp`): Real-time log display with auto-scroll and animations
- **QualityMetricsDialog** (`src/dialogs/qualitymetricsdialog.cpp`): PSNR/SSIM comparison tool
- **CSVExportDialog** (`src/dialogs/csvexportdialog.cpp`): CSV metrics export tool
- **AboutDialog** (`src/dialogs/aboutdialog.cpp`): About and help dialog
- **MP4Parser** (`src/core/mp4parser.cpp`): MP4 container structure analysis
- **TSParser** (`src/core/tsparser.cpp`): MPEG-TS stream analysis with TR 101-290 compliance
- **MKVParser** (`src/core/mkvparser.cpp`): Matroska container analysis
- **AVIParser** (`src/core/aviparser.cpp`): AVI RIFF chunk analysis
- **FLVParser** (`src/core/flvparser.cpp`): FLV tag structure analysis
- **MainWindow** (`src/mainwindow.cpp`): Main application window and UI coordination

### Technology Stack

- **Qt 6**: Cross-platform GUI framework
- **FFmpeg**: Video decoding and codec support
- **CMake**: Build system
- **C++17**: Modern C++ features

## Development

### Project Structure

```
VideoStudio/
├── CMakeLists.txt              # Build configuration
├── src/
│   ├── main.cpp                # Entry point
│   ├── mainwindow.h/cpp        # Main window
│   ├── core/                   # Core engine
│   │   ├── videodecoder.h/cpp  # FFmpeg wrapper
│   │   └── framedata.h/cpp     # Data structures
│   └── widgets/                # Custom widgets
│       ├── videooutput.h/cpp   # Video display
│       └── barchart.h/cpp      # Bar chart
├── resources/                  # Icons and resources
└── tests/                      # Unit tests
```

### Building in Qt Creator

```bash
# Open project in Qt Creator
~/Qt/Qt\ Creator.app/Contents/MacOS/Qt\ Creator CMakeLists.txt
```

### Code Style

- Follow Qt coding conventions
- Use Qt containers (QVector, QString, etc.)
- Namespace: `VideoStudio`
- Use signals/slots for event handling
- Smart pointers for resource management

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Inspired by [Elecard StreamEye](https://www.elecard.com/products/video-analysis/stream-eye)
- Built with [Qt](https://www.qt.io/) and [FFmpeg](https://ffmpeg.org/)
- Thanks to the open-source community

## Roadmap

### Version 1.0 (Current - MVP)
- [x] Basic video decoding with FFmpeg
- [x] Frame-by-frame navigation
- [x] Bitrate visualization (bar chart)
- [x] Video playback controls
- [x] Frame metadata extraction
- [x] Real-time log viewer with auto-scroll
- [x] MP4 container structure analysis
- [x] TS stream packet analysis
- [x] MKV container element analysis
- [x] Multi-threaded file loading with progress feedback

### Version 1.1
- [x] Stream info panel
- [x] Thumbnail navigation bar
- [x] GOP structure viewer
- [x] Quality metrics (PSNR, SSIM)
- [x] Export YUV frames
- [x] Transport stream analysis (TR 101-290 compliance)
- [x] Container parsers (MP4, MKV, AVI, FLV)
- [x] Hex viewer panel
- [x] Time dynamics panel (PTS/DTS/PCR)
- [x] Messages and error reporting

### Version 1.2
- [ ] Motion vector overlay
- [ ] Block/partition visualization
- [ ] Reference stream comparison
- [x] CSV metrics export

### Version 2.0
- [ ] VMAF quality metric
- [ ] Compliance verification
- [ ] Buffer analysis
- [ ] Command-line tool
- [ ] Plugin system

## Support

For questions, issues, or feature requests, please open an issue on GitHub.

## Contact

- **Project**: [VideoStudio](https://github.com/yourusername/VideoStudio)
- **Documentation**: See [CLAUDE.md](CLAUDE.md) for development guidelines

---

**Note**: This is an early-stage project under active development. Features and APIs may change.
