# Test Files for VideoStudio

This directory contains test video files in various formats for testing VideoStudio's container parsing and video decoding capabilities.

## Quick Start

Generate all test files:
```bash
./generate_test_files.sh
```

This will create a `test_files/` directory with sample videos in all supported formats.

## Supported Formats

### Container Formats
- **AVI** - Audio Video Interleave (RIFF-based)
- **MP4** - MPEG-4 Part 14 (ISO Base Media)
- **MKV** - Matroska (EBML-based)
- **MOV** - QuickTime (ISO Base Media)
- **TS** - MPEG Transport Stream
- **WebM** - WebM (Matroska subset)

### Video Codecs
- **H.264/AVC** - Baseline, Main, High profiles
- **H.265/HEVC** - Main, Main10 profiles
- **AV1** - AOMedia Video 1
- **VP9** - Google VP9
- **MJPEG** - Motion JPEG
- **MPEG-4** - MPEG-4 Part 2
- **ProRes** - Apple ProRes (if available)

### Audio Codecs
- **AAC** - Advanced Audio Coding
- **MP3** - MPEG Audio Layer 3
- **Opus** - Opus Interactive Audio
- **PCM** - Pulse Code Modulation

## Test File Categories

### 1. AVI Files
- `test_mjpeg.avi` - MJPEG video + PCM audio
- `test_h264.avi` - H.264 video + AAC audio
- `test_mpeg4.avi` - MPEG-4 video + MP3 audio
- `test_uncompressed.avi` - Raw video (BGR24)

### 2. H.264 Raw Streams
- `test_h264_baseline.h264` - Baseline profile, level 3.0
- `test_h264_main.h264` - Main profile, level 3.1
- `test_h264_high.h264` - High profile, level 4.0, 720p
- `test_h264_bframes.h264` - With B-frames, GOP size 30

### 3. H.265/HEVC Streams
- `test_h265_main.h265` - Main profile, 720p
- `test_h265_main10.h265` - Main10 profile (10-bit), 720p
- `test_h265_4k.h265` - 4K resolution (3840x2160)

### 4. AV1 Files
- `test_av1.mp4` - AV1 in MP4 container
- `test_av1.webm` - AV1 in WebM container

### 5. VP9 Files
- `test_vp9.webm` - VP9 + Opus in WebM
- `test_vp9.mkv` - VP9 + Opus in MKV

### 6. MP4 Files
- `test_h264.mp4` - H.264 + AAC
- `test_h265.mp4` - H.265 + AAC
- `test_multi_audio.mp4` - Multiple audio tracks (English, Spanish)

### 7. MKV Files
- `test_h264.mkv` - H.264 + AAC
- `test_h265.mkv` - H.265 + AAC
- `test_chapters.mkv` - With chapter markers

### 8. Transport Stream Files
- `test_h264.ts` - H.264 + AAC in MPEG-TS
- `test_h265.ts` - H.265 + AAC in MPEG-TS
- `test_multi_program.ts` - Multiple programs

### 9. MOV Files
- `test_h264.mov` - H.264 + AAC
- `test_prores.mov` - ProRes (if encoder available)

### 10. Special Test Cases
- `test_single_frame.mp4` - Single frame video
- `test_60fps.mp4` - High frame rate (60fps)
- `test_vfr.mp4` - Variable frame rate
- `test_16x9.mp4` - 16:9 aspect ratio (1920x1080)
- `test_5x4.mp4` - 5:4 aspect ratio (1280x1024)

## Testing Workflow

### 1. Container Parsing
Test Explorer panel with different container formats:
```bash
# Open AVI file
./VideoStudio.app/Contents/MacOS/VideoStudio test_files/test_h264.avi

# Open MP4 file
./VideoStudio.app/Contents/MacOS/VideoStudio test_files/test_h264.mp4

# Open MKV file
./VideoStudio.app/Contents/MacOS/VideoStudio test_files/test_h264.mkv

# Open TS file
./VideoStudio.app/Contents/MacOS/VideoStudio test_files/test_h264.ts
```

### 2. Video Decoding
Test video playback and frame navigation:
- Open any video file
- Use Play/Pause controls
- Step forward/backward through frames
- Click on bar chart or thumbnails to seek

### 3. Property Panel
Test detailed property display:
- Click on elements in Explorer panel
- Verify property details in Property panel
- Test Sync/Compare/Dump modes

### 4. Hex Viewer
Test hex viewer synchronization:
- Select element in Explorer panel
- Verify hex data in Hex Viewer panel
- Test search functionality
- Try different bytes per row settings

### 5. TR 101-290 Compliance (TS files only)
Test transport stream validation:
- Open TS file
- Check TR 101-290 panel for errors
- Verify error detection and navigation

## File Specifications

All test files use:
- **Video**: Test pattern (color bars with moving elements)
- **Audio**: 1kHz sine wave tone
- **Duration**: 5 seconds (unless specified otherwise)
- **Frame Rate**: 30fps (unless specified otherwise)
- **Resolution**: 640x480 or 1280x720 (unless specified otherwise)

## Requirements

- **FFmpeg** with the following encoders:
  - libx264 (H.264)
  - libx265 (H.265)
  - libvpx-vp9 (VP9)
  - libaom-av1 (AV1, optional)
  - prores_ks (ProRes, optional)

Install FFmpeg on macOS:
```bash
brew install ffmpeg
```

## Troubleshooting

### Missing Encoders
If certain encoders are not available, the script will skip those formats:
- AV1: Install with `brew install ffmpeg --with-libaom`
- ProRes: Usually included in standard FFmpeg builds

### Generation Time
Generating all test files takes approximately 5-10 minutes depending on your system.

### Disk Space
The complete test file set requires approximately 100-200 MB of disk space.

## Manual Testing Checklist

- [ ] AVI files open and display correctly
- [ ] MP4 files show atom hierarchy
- [ ] MKV files show EBML structure
- [ ] TS files show PSI/SI tables
- [ ] Video playback works for all codecs
- [ ] Frame navigation (step forward/backward)
- [ ] Seeking via bar chart and thumbnails
- [ ] Property panel displays correct information
- [ ] Hex viewer synchronizes with selection
- [ ] Search functionality works in hex viewer
- [ ] TR 101-290 validation for TS files
- [ ] Multiple audio tracks detected
- [ ] Chapter markers displayed (MKV)
- [ ] Different resolutions handled correctly
- [ ] High frame rate videos play smoothly

## Cleanup

Remove all test files:
```bash
rm -rf test_files/
```
