# VideoStudio v2.0.0 Release Notes

🎉 **Major Release: Cross-Platform Support & Enhanced Analysis**

## Download

**Pre-built binaries for all platforms:**
- [VideoStudio-Windows-x64.zip](https://github.com/build-your-own-x-with-ai/VideoStudio/releases/download/v2.0.0/VideoStudio-Windows-x64.zip)
- [VideoStudio-macOS-x64.tar.gz](https://github.com/build-your-own-x-with-ai/VideoStudio/releases/download/v2.0.0/VideoStudio-macOS-x64.tar.gz)
- [VideoStudio-Linux-x64.tar.gz](https://github.com/build-your-own-x-with-ai/VideoStudio/releases/download/v2.0.0/VideoStudio-Linux-x64.tar.gz)

**Backup download (Baidu Netdisk):**
链接: https://pan.baidu.com/s/1hjvHDnAHsCuQhaC5K1LMeQ?pwd=3kxq 提取码:3kxq

## What's New in v2.0

### 🌍 Cross-Platform Support
- ✅ **Windows 10/11** - Full native support with MSVC build
- ✅ **macOS 10.15+** - Universal binary with optimized performance
- ✅ **Linux (Ubuntu 20.04+)** - Qt6 and FFmpeg 4.x support
- ✅ **Automated CI/CD** - GitHub Actions builds for all platforms

### 🔍 Compliance Validation
- ✅ **H.264/H.265 Bitstream Validation** - Syntax verification against ITU-T specifications
- ✅ **Profile & Level Detection** - Automatic codec profile identification
- ✅ **Buffer Model Verification** - HRD (Hypothetical Reference Decoder) validation
- ✅ **Real-time Error Detection** - Bitstream violations flagged during analysis

### 🎵 Audio Analysis
- ✅ **Waveform Visualization** - Real-time audio waveform display
- ✅ **Spectrum Analyzer** - Frequency spectrum with FFT analysis
- ✅ **Loudness Meter (LUFS)** - ITU-R BS.1770 loudness measurement
- ✅ **Phase Meter** - Stereo phase correlation analysis
- ✅ **Multi-stream Support** - Analyze multiple audio tracks

### 🔌 Plugin System
- ✅ **Extensible Architecture** - C++ plugin API for custom analyzers
- ✅ **Sample Plugin** - Example plugin demonstrating frame analysis
- ✅ **Hot Loading** - Load/unload plugins without restart
- ✅ **Plugin Manager** - UI for managing installed plugins

### 🛠️ Enhanced Stability
- ✅ **FFmpeg Compatibility Layer** - Automatic support for FFmpeg 4.x, 5.x, 6.x, and 7.x
- ✅ **Cross-Version API** - Handles channel layout, resampler, and profile constant differences
- ✅ **Robust Error Handling** - Improved error recovery and user feedback
- ✅ **Memory Management** - Fixed memory leaks and optimized performance

### 📊 Buffer Analysis Improvements
- ✅ **CPB Monitoring** - Coded Picture Buffer visualization
- ✅ **HRD Verification** - Hypothetical Reference Decoder compliance checking
- ✅ **VBV Analysis** - Video Buffering Verifier for MPEG standards
- ✅ **Underflow/Overflow Detection** - Real-time buffer state monitoring

## System Requirements

### All Platforms
- Qt 6.5.3+ (6.11.0 recommended for development)
- FFmpeg 4.x, 5.x, 6.x, or 7.x
- CMake 3.16+
- C++17 compiler

### Platform-Specific
- **Windows**: Windows 10 or later, Visual Studio 2019+
- **macOS**: macOS 10.15 (Catalina) or later
- **Linux**: Ubuntu 20.04+ or equivalent (glibc 2.31+)

## Installation

### Windows
1. Download `VideoStudio-Windows-x64.zip`
2. Extract to any folder
3. Run `VideoStudio.exe`

### macOS
1. Download `VideoStudio-macOS-x64.tar.gz`
2. Extract: `tar -xzf VideoStudio-macOS-x64.tar.gz`
3. Run `VideoStudio.app`

### Linux
1. Download `VideoStudio-Linux-x64.tar.gz`
2. Extract: `tar -xzf VideoStudio-Linux-x64.tar.gz`
3. Run: `./VideoStudio`

## Breaking Changes

None - v2.0 maintains full backward compatibility with v1.x project files and workflows.

## Bug Fixes

- Fixed FFmpeg 4.x compatibility issues (channel layout API)
- Fixed Qt 6.5.3 signal/slot compatibility (checkStateChanged)
- Fixed Windows linking issues (swresample library)
- Fixed profile constant definitions for older FFmpeg versions
- Fixed int64_t to QVariant conversion ambiguities
- Fixed AVFrame::pkt_size removal in FFmpeg 5.1+
- Fixed key frame flag detection across FFmpeg versions

## Known Issues

- None reported for v2.0.0

## Migration Guide

Users upgrading from v1.x can use v2.0 without any changes. All existing features continue to work as before.

## Contributors

- Claude Code (AI-assisted development)
- Community feedback and testing

## Credits

- Inspired by [Elecard StreamEye](https://www.elecard.com/products/video-analysis/stream-eye)
- Built with [Qt](https://www.qt.io/) and [FFmpeg](https://ffmpeg.org/)
- Automated builds via GitHub Actions

## Next Steps (v2.1 Planned)

- Reference stream comparison with side-by-side difference modes
- Advanced motion vector analysis tools
- Scene change detection and analysis
- Batch processing improvements
- Additional quality metrics (MS-SSIM, VIF)

---

For documentation, see [README.md](https://github.com/build-your-own-x-with-ai/VideoStudio/blob/main/README.md)

For development guidelines, see [CLAUDE.md](https://github.com/build-your-own-x-with-ai/VideoStudio/blob/main/CLAUDE.md)
