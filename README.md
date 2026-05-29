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
- **Bitrate Visualization**: Interactive bar chart showing frame sizes and types
- **Frame-by-Frame Analysis**: Step through video frame by frame
- **Video Playback**: Play, pause, seek with accurate frame positioning
- **Frame Metadata**: Extract PTS, DTS, frame type, size, QP values
- **Professional UI**: Qt-based interface with dockable panels

### Planned Features

- Quality metrics (PSNR, SSIM, VMAF)
- Motion vector visualization
- Block/partition information display
- Reference stream comparison
- YUV export and CSV metrics export
- Compliance verification messages
- Buffer analysis (CPB)
- Hex viewer for raw bitstream

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
3. Select a video file (MP4, MKV, AVI, MOV, H.264, H.265, etc.)
4. The video will be indexed and the first frame displayed

### Navigation

- **Play/Pause**: Click the Play button or press **Space**
- **Step Forward**: Click Step Forward or press **Alt+Right**
- **Step Backward**: Click Step Backward or press **Alt+Left**
- **Seek**: Click on the bar chart to jump to a specific frame

### Bar Chart

The bar chart at the top shows:
- **Bar Height**: Frame size in bytes
- **Bar Color**: Frame type (Red=I-frame, Blue=P-frame, Green=B-frame)
- **Yellow Line**: Key frame indicator
- **Red Vertical Line**: Current frame position

## Architecture

### Core Components

- **VideoDecoder** (`src/core/videodecoder.cpp`): FFmpeg integration for decoding
- **FrameIndex** (`src/core/framedata.cpp`): Frame metadata storage and statistics
- **VideoOutput** (`src/widgets/videooutput.cpp`): Video display with YUV to RGB conversion
- **BarChart** (`src/widgets/barchart.cpp`): Bitrate visualization widget
- **MainWindow** (`src/mainwindow.cpp`): Main application window and UI

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

### Version 1.1
- [ ] Stream info panel
- [ ] Thumbnail navigation bar
- [ ] Quality metrics (PSNR, SSIM)
- [ ] Export YUV frames

### Version 1.2
- [ ] Motion vector overlay
- [ ] Block/partition visualization
- [ ] Reference stream comparison
- [ ] CSV metrics export

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
