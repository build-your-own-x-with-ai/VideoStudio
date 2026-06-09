# QP Heatmap Visualization

## Overview

The **QP (Quantization Parameter) Heatmap** feature provides visual analysis of video compression quality at the macroblock level. By displaying per-block QP values as a color-coded overlay, it enables engineers to identify compression artifacts, quality variations, and encoding behavior patterns.

## What is QP?

**Quantization Parameter (QP)** controls the compression level for each macroblock in H.264/H.265 video:
- **Range**: 0-51
- **Lower QP** (0-20): High quality, less compression, larger file size
- **Higher QP** (35-51): Low quality, aggressive compression, smaller file size
- **Typical values**: 18-28 for high-quality content, 30-40 for streaming video

## Visual Representation

### Color Mapping

The heatmap uses a **green → yellow → red** gradient:

| QP Range | Color | Quality Level | Typical Use Case |
|----------|-------|---------------|------------------|
| 0-12 | **Dark Green** | Excellent | Lossless/near-lossless, archive |
| 13-20 | **Green** | Very High | Professional mastering, Blu-ray |
| 21-28 | **Yellow-Green** | High | Streaming HD, broadcast TV |
| 29-35 | **Yellow** | Medium | Standard streaming, web video |
| 36-42 | **Orange** | Low | Low-bitrate mobile, poor network |
| 43-51 | **Red** | Very Low | Extreme compression, emergency fallback |

### Legend Display

The heatmap includes a real-time statistics panel in the bottom-right corner:
- **Min QP**: Lowest QP value in current frame (highest quality blocks)
- **Max QP**: Highest QP value in current frame (lowest quality blocks)
- **Range**: QP variation (larger range = more quality variation)
- **Color Scale**: Visual gradient bar from QP 0 (green) to QP 51 (red)

### Zoom Behavior

- **Normal view** (zoom < 1.5×): Color-coded blocks only
- **Zoomed view** (zoom ≥ 1.5×): QP values displayed as numbers inside blocks

## How to Enable

### Method 1: Menu
1. Open a video file (MP4, MKV, AVI, or raw H.264/H.265)
2. Go to **View → Overlay → QP Heatmap**
3. Or use keyboard shortcut: **ALT+5**

### Method 2: Overlay Panel
1. Open the **Overlay Panel** (dock on the right side)
2. Check **"QP Heatmap (ALT+5)"**

## Technical Requirements

### Decoder Requirements

QP heatmap requires the video codec to export encoding parameters:

**FFmpeg Decoder Flags** (automatically enabled in VideoStudio):
```c
codec_context->export_side_data |= AV_CODEC_EXPORT_DATA_VIDEO_ENC_PARAMS;
```

### Supported Codecs

✅ **Fully Supported:**
- H.264/AVC (all profiles: Baseline, Main, High, High 10)
- H.265/HEVC (all profiles: Main, Main 10, Rext)

⚠️ **Partial Support:**
- VP9 (may work if encoder exports parameters)
- AV1 (experimental, depends on encoder)

❌ **Not Supported:**
- MPEG-2 (does not export QP data)
- VP8 (no standardized QP export)
- MJPEG (frame-level QP only)

### Data Availability

QP data availability depends on:

1. **Encoding Method:**
   - ✅ **Software encoders** (x264, x265, libvpx): Usually export QP data
   - ⚠️ **Hardware encoders** (Intel QSV, NVIDIA NVENC, Apple VideoToolbox): May not export QP data
   - ❌ **Unknown encoders**: No guarantee

2. **Container Format:**
   - ✅ **MP4/MOV**: Best support
   - ✅ **MKV/WebM**: Good support
   - ⚠️ **AVI**: Limited support
   - ❌ **Transport Streams (.ts)**: No QP data in container

3. **Decoding Mode:**
   - ✅ **Frame decoding**: Full QP data available
   - ❌ **Packet inspection only**: No QP data

## Interpretation Guide

### Uniform Green (Low QP)
- **Good**: Consistent high quality
- **Typical**: I-frames, static scenes, high-bitrate content
- **Use case**: Archival, mastering, reference video

### Yellow-Green Gradient (Medium QP)
- **Good**: Adaptive bitrate control working well
- **Typical**: P/B-frames, normal motion scenes
- **Use case**: Standard streaming, broadcast

### Red Blocks (High QP)
- **Warning**: Potential quality issues
- **Typical**: Fast motion, complex textures, low bitrate
- **Issues**: Blocking artifacts, blurring, banding
- **Action**: Increase bitrate or use 2-pass encoding

### Extreme Variations (Green + Red in same frame)
- **Warning**: Aggressive rate control or scene complexity
- **Typical**: Scene changes, sudden motion, fixed-bitrate mode
- **Issues**: Visible quality inconsistency within frame
- **Action**: Use CRF mode or increase buffer size

## Use Cases

### 1. Quality Analysis
**Scenario**: Verify video encoding quality before delivery

**Steps:**
1. Open encoded video in VideoStudio
2. Enable QP Heatmap (ALT+5)
3. Navigate through frames (especially I-frames and P-frames)
4. Check QP statistics:
   - **Max QP < 35**: Good quality
   - **Max QP 35-40**: Acceptable for streaming
   - **Max QP > 40**: Poor quality, re-encode recommended

### 2. Encoder Tuning
**Scenario**: Optimize encoder settings for quality/bitrate balance

**Steps:**
1. Encode same video with different QP settings:
   - `ffmpeg -i input.mp4 -c:v libx264 -crf 18 high_quality.mp4`
   - `ffmpeg -i input.mp4 -c:v libx264 -crf 28 medium_quality.mp4`
   - `ffmpeg -i input.mp4 -c:v libx264 -crf 35 low_quality.mp4`
2. Open each in VideoStudio with QP heatmap
3. Compare QP distributions and visual quality
4. Choose optimal CRF value

### 3. Bitrate Spike Investigation
**Scenario**: Understand why certain frames have high bitrate

**Steps:**
1. Use Bar Chart panel to identify high-bitrate frames
2. Navigate to those frames
3. Enable QP Heatmap
4. **Low QP (green) + high bitrate**: Complex scene, detail preservation
5. **High QP (red) + high bitrate**: Scene change, I-frame, or encoder struggling

### 4. Compression Artifact Detection
**Scenario**: Find and fix blocking artifacts

**Steps:**
1. Enable QP Heatmap
2. Look for **red blocks** in smooth areas (sky, skin, gradients)
3. These blocks likely have visible artifacts
4. Re-encode with lower CRF or higher bitrate target

### 5. Hardware Encoder Validation
**Scenario**: Compare software vs hardware encoder quality

**Steps:**
1. Encode with software: `ffmpeg -c:v libx264 -preset slow -crf 23`
2. Encode with hardware: `ffmpeg -c:v h264_videotoolbox -b:v 5M`
3. Open both in VideoStudio
4. Compare QP distributions:
   - **Software**: Usually more uniform QP, better quality
   - **Hardware**: May have higher QP variance, faster encoding

## Advanced Analysis

### QP Per Frame Type

**I-Frames (Keyframes):**
- Typically **lowest QP** (greenest in heatmap)
- Highest quality, largest size
- **Target**: QP 18-24 for high quality

**P-Frames:**
- **Medium QP** (yellow-green)
- Moderate quality, moderate size
- **Target**: QP 20-28 for high quality

**B-Frames:**
- **Highest QP** (orange-red)
- Lowest quality, smallest size
- **Target**: QP 24-32 for high quality

### CRF (Constant Rate Factor) Mapping

| CRF | Typical QP Range | Heatmap Colors | Quality | Use Case |
|-----|------------------|----------------|---------|----------|
| 0 | 0-5 | Dark Green | Lossless | Archive |
| 18 | 15-22 | Green | Excellent | Mastering |
| 23 | 20-28 | Yellow-Green | High | Streaming HD |
| 28 | 25-33 | Yellow-Orange | Medium | Web video |
| 35 | 32-40 | Orange-Red | Low | Mobile |
| 51 | 45-51 | Red | Very Low | Emergency |

### Bitrate Modes

**CRF (Constant Rate Factor):**
- **QP variation**: Low to medium
- **Heatmap**: Relatively uniform colors
- **Best for**: Quality-focused encoding

**CBR (Constant Bitrate):**
- **QP variation**: High (adapts to scene complexity)
- **Heatmap**: Wide color range (green to red)
- **Best for**: Live streaming, broadcast

**VBR (Variable Bitrate):**
- **QP variation**: Medium
- **Heatmap**: Moderate color range
- **Best for**: File-based delivery, VOD

## Limitations

1. **No QP Data Available**
   - **Symptom**: Yellow message "No QP data available"
   - **Causes**:
     - Hardware-encoded video (NVENC, QSV, VideoToolbox)
     - Encoder did not export QP tables
     - Container stripped metadata
   - **Solution**: Re-encode with software encoder (libx264, libx265)

2. **Coarse Block Granularity**
   - **Issue**: Heatmap shows 16×16 macroblocks, not finer sub-partitions
   - **Reason**: FFmpeg exports macroblock-level QP only
   - **Impact**: Cannot visualize 4×4 or 8×8 block-level QP variation

3. **Frame Types Not Distinguished**
   - **Issue**: Heatmap shows QP only, not whether it's I/P/B frame QP
   - **Solution**: Enable "Frame Type Info" overlay (ALT+4) simultaneously

4. **Performance Impact**
   - **Issue**: Drawing heatmap for 4K video may be slow
   - **Solution**: Reduce zoom level or toggle off when not needed

## Combining with Other Overlays

**QP Heatmap + Frame Type Info:**
- Understand QP variation across frame types
- Verify I-frames have lowest QP

**QP Heatmap + Partitions:**
- See block boundaries overlaid on QP colors
- Identify small partitions with high QP

**QP Heatmap + Motion Vectors:**
- Correlate motion with QP
- High motion areas often have higher QP

## Example Scenarios

### Scenario A: Professional Archival
```
✅ Goal: Lossless or near-lossless quality
🎯 Target: All blocks green (QP < 20)
📊 Settings: CRF 0-10, x264 -preset veryslow
🖼️ Heatmap: Solid green across entire frame
```

### Scenario B: Streaming HD
```
✅ Goal: High quality, reasonable file size
🎯 Target: Mostly yellow-green (QP 20-28)
📊 Settings: CRF 20-23, x264 -preset medium
🖼️ Heatmap: Yellow-green gradient, occasional orange in complex areas
```

### Scenario C: Mobile Video
```
✅ Goal: Small file size, acceptable quality
🎯 Target: Yellow-orange (QP 28-35)
📊 Settings: CRF 28-32, x264 -preset fast
🖼️ Heatmap: Orange dominant, red in high-motion scenes
```

### Scenario D: Emergency Low Bitrate
```
⚠️ Goal: Minimum file size, quality compromised
🎯 Target: Orange-red (QP 35-42)
📊 Settings: CRF 35-40, x264 -preset veryfast
🖼️ Heatmap: Mostly red, severe quality loss expected
```

## Troubleshooting

### "No QP data available" Message

**Check 1**: Is this a software-encoded video?
```bash
ffprobe -v error -show_streams -select_streams v:0 input.mp4 | grep encoder
# If encoder=h264_nvenc or h264_videotoolbox, re-encode with libx264
```

**Check 2**: Re-encode with QP export forced
```bash
ffmpeg -i input.mp4 -c:v libx264 -crf 23 -x264-params "aq-mode=2" output.mp4
```

### Heatmap Shows Wrong Colors

**Issue**: All blocks are red even though video looks good
**Solution**: QP values may be offset. Check codec:
- H.264: QP range 0-51
- H.265: QP range 0-51
- VP9: QP range 0-255 (different scale)

### Heatmap Too Slow on 4K Video

**Solution 1**: Use lower zoom level (QP values not drawn)
**Solution 2**: Resize video window smaller
**Solution 3**: Toggle heatmap off when not actively analyzing

## Feature Status

- **Version Added**: 2026-06-09
- **VideoStudio Version**: 1.0.0+
- **Supported Codecs**: H.264/AVC, H.265/HEVC
- **Platform**: macOS ✅, Windows (planned)

## Related Features

- **Motion Vectors Overlay** (ALT+3): Visualize motion prediction
- **Partitions Overlay** (ALT+2): Show block boundaries
- **Frame Type Info** (ALT+4): Display I/P/B frame type
- **Bar Chart Panel**: Frame size and bitrate visualization
- **NAL Unit List**: Detailed slice QP values

## References

- ITU-T H.264: Section 7.4.2.1.1 - Quantization parameter
- ITU-T H.265: Section 7.3.4 - Slice segment header
- x264 Documentation: CRF and QP relationship
- FFmpeg AVFrame: AVVideoEncParams side data structure

---

**Note**: QP heatmap is a powerful tool for video quality analysis. Use it alongside subjective visual inspection and objective metrics (PSNR, SSIM, VMAF) for comprehensive quality assessment.
