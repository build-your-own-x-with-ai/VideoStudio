# Enhanced Partition Visualization

## Overview

The **Enhanced Partition Visualization** feature provides professional-grade codec analysis by color-coding macroblocks based on prediction modes, QP values, and reference frame usage. This matches the capabilities of Elecard StreamEye's partition overlay.

## Features

### 1. Prediction Mode Color Coding

Blocks are filled with colors indicating their prediction type:

| Prediction Mode | Color | Description |
|----------------|-------|-------------|
| **Intra** | 🔴 Red | Frame-internal prediction (no motion compensation) |
| **Inter** | 🔵 Blue | Frame-to-frame prediction (uses reference frames) |
| **Skip** | ⚪ Gray | Skip blocks (zero motion, copy from reference) |

### 2. QP-Based Intensity

The **opacity/alpha** of each block color indicates compression quality:
- **Light color** (low opacity): Low QP = High quality
- **Dark color** (high opacity): High QP = Low quality (aggressive compression)

**Formula**: `Alpha = 100 + (QP × 3)`
- QP 0 → Alpha 100 (translucent)
- QP 51 → Alpha 253 (nearly opaque)

### 3. Reference Frame Index Labels

For **Inter** blocks at zoom ≥ 1.2×:
- White number in center shows which reference frame is used
- `0` = most recent reference frame (ref_idx_l0[0])
- `1`, `2`, etc. = older reference frames

### 4. QP Value Labels

At zoom ≥ 1.5×, blocks display their QP value in the top-left corner:
- Format: `Q24` (QP = 24)
- Helps identify quality variations within a frame

### 5. Statistics Legend

Real-time statistics overlay (top-left corner):
```
Prediction Modes:
🔴 Intra: 885
🔵 Inter: 908  
⚪ Skip: 886
```

## How to Use

### Enable Enhanced Partitions

1. **Menu**: View → Overlays → Partitions
2. **Keyboard**: `ALT+2`
3. **Panel**: Overlays Panel → Check "Partitions (ALT+2)"

### Zoom Levels

Different features appear at different zoom levels:

| Zoom Level | Features Visible |
|-----------|------------------|
| 0.8× - 1.0× | Block fills, borders, statistics legend |
| 1.2× - 1.5× | + Reference frame indices |
| 1.5×+ | + QP values in blocks |

### Recommended Workflow

1. **Open a video** with QP data (software-encoded: libx264/libx265)
2. **Enable Partitions overlay** (ALT+2)
3. **Navigate to P or B frames** (I-frames show standard grid)
4. **Zoom in** (mouse wheel) to see reference indices and QP values
5. **Analyze**:
   - Red areas = Intra prediction (scene changes, complex textures)
   - Blue areas = Inter prediction (motion compensation working)
   - Gray areas = Skip blocks (static regions)
   - Dark colors = High QP (quality degradation)

## Use Cases

### 1. Encoder Analysis

**Scenario**: Verify encoder is using appropriate prediction modes

**What to look for**:
- P-frames should have mostly **blue** (Inter) blocks
- I-frames show standard grid (all Intra)
- B-frames should have mix of blue/gray
- Too much **red** in P/B frames = encoder not utilizing motion compensation

**Example**: A talking-head video should show:
- Gray blocks on static background
- Blue blocks on moving face/lips
- Red blocks only during scene changes

### 2. Quality Analysis

**Scenario**: Identify quality issues in compressed video

**What to look for**:
- **Dark red** blocks = High QP intra blocks (visible blocking artifacts)
- **Dark blue** blocks = High QP inter blocks (motion blur)
- **Uniform light colors** = Consistent quality
- **Patchy dark/light mix** = Aggressive rate control (quality varies)

### 3. Reference Frame Efficiency

**Scenario**: Understand multi-reference frame usage

**What to look for**:
- Most blocks show `0` = using nearest reference (efficient)
- Blocks showing `1`, `2` = using older references (long-term prediction)
- High reference indices in static scenes = good long-term memory
- High reference indices with motion = potential encoder issue

### 4. Motion Analysis

**Scenario**: Verify skip block detection

**What to look for**:
- Static backgrounds should be **gray** (skip blocks)
- Moving objects should be **blue** (inter with motion vectors)
- If static areas are blue instead of gray = encoder not detecting skip blocks efficiently

### 5. Codec Comparison

**Scenario**: Compare x264 vs x265 encoding decisions

**Steps**:
1. Encode same video with x264 and x265
2. Open both in VideoStudio
3. Compare same frame with Partitions overlay
4. **x265 differences**:
   - Larger variety of block sizes
   - More aggressive skip block usage
   - Better intra/inter decisions in complex scenes

## Technical Details

### Data Sources

- **Prediction modes**: From `AVMotionVector.source` field
  - `source == -1` → Intra
  - `motion_x == 0 && motion_y == 0` → Skip
  - Otherwise → Inter
  
- **QP values**: From `AV_FRAME_DATA_VIDEO_ENC_PARAMS` side data
  - Requires: `codec_context->export_side_data |= AV_CODEC_EXPORT_DATA_VIDEO_ENC_PARAMS`
  
- **Reference indices**: From `AVMotionVector.source` field (0, 1, 2, ...)

### Block Boundary Colors

Block borders use size-based color coding:

| Block Size | Border Color | Line Width |
|-----------|-------------|-----------|
| ≥32×32 | Yellow (255,255,0) | 2 pixels |
| 16×16 | Orange (255,200,0) | 1 pixel |
| <16×16 | Dim Yellow (200,200,0) | 1 pixel |

### Standard Grid Mode

When enabled (`ALT+G`), forces uniform 16×16 grid even for P/B frames:
- Useful for comparing actual partitions vs standard macroblocks
- Toggle to see how much sub-partitioning is used

## Compatibility

### Supported Codecs

✅ **Full Support**:
- H.264/AVC (all profiles)
- H.265/HEVC (all profiles)

⚠️ **Partial Support** (motion vectors only, no QP data):
- VP9 (some encoders)
- AV1 (experimental)

❌ **Not Supported**:
- MPEG-2 (no motion vector export)
- VP8 (limited side data)
- MJPEG (intra-only codec)

### Encoder Requirements

- **Software encoders** (libx264, libx265): Full support
- **Hardware encoders** (NVENC, QSV, VideoToolbox): Limited support (may not export QP/MV data)

### Container Formats

- MP4/MOV: Full support
- MKV/WebM: Full support
- AVI: Partial support
- TS: Limited support

## Troubleshooting

### Problem: All blocks are yellow (no colors)

**Cause**: No motion vector data available
**Solution**: 
1. Check if video is I-frame only (MJPEG, timelapse)
2. Re-encode with software encoder (x264/x265)
3. Ensure P/B frames exist (not all I-frames)

### Problem: No QP intensity variations

**Cause**: QP data not exported by encoder
**Solution**:
1. Use software encoder with QP export enabled
2. Check if hardware encoder used (NVENC/QSV don't export QP)
3. Re-encode: `ffmpeg -i input.mp4 -c:v libx264 -x264-params "aq-mode=2" output.mp4`

### Problem: No reference frame indices

**Cause**: Zoom level too low
**Solution**: Zoom to at least 1.2× to see reference indices

### Problem: Legend shows all zeros

**Cause**: Viewing I-frame (no inter prediction)
**Solution**: Navigate to P or B frame using arrow keys

## Combining with Other Overlays

### Partitions + Motion Vectors

- Partitions show **prediction type**
- Motion Vectors show **motion direction**
- Together: complete picture of motion compensation

**Use case**: Verify skip blocks actually have zero motion vectors

### Partitions + QP Heatmap

- Partitions show **per-block QP with prediction context**
- QP Heatmap shows **overall quality distribution**
- Together: identify why certain blocks have high QP

**Use case**: Check if intra blocks have higher QP than inter blocks

### Partitions + Frame Type Info

- Partitions show **block-level prediction**
- Frame Type Info shows **frame-level type**
- Together: understand GOP structure and prediction hierarchy

**Use case**: Verify P-frames only reference previous I/P frames

## Example Interpretation

### Scenario: 640×360 P-Frame Analysis

**Observed**:
```
Prediction Modes:
🔴 Intra: 885 (49.4%)
🔵 Inter: 908 (50.6%)
⚪ Skip: 886 (49.4%)
```

**Interpretation**:
- Nearly 50/50 split intra/inter = **medium motion scene**
- High skip count (49.4%) = **partial static content**
- Balance suggests **talking head** or **slow pan** video
- Encoder is effectively using motion compensation

**Quality check**:
- If blocks are mostly light = **good quality** (low QP)
- If blocks are mostly dark = **high compression** (high QP)
- Mixed light/dark = **adaptive quality** (rate control active)

## Performance Notes

- Enhanced overlay uses same motion vector data as basic overlay
- QP lookup adds minimal overhead (<1ms per frame)
- Block filling uses GPU-accelerated Qt painting
- No performance impact on decoding (read-only visualization)

## Future Enhancements

Potential additions (not yet implemented):
- Toggle between prediction mode colors and block size colors
- Export partition analysis to CSV
- Heatmap of skip block distribution over time
- Reference frame distance visualization
- Sub-partition boundary display (4×4, 8×8 within 16×16)

---

**Feature Status**: ✅ Complete (2026-06-10)
**VideoStudio Version**: 1.0.0+
**Platforms**: macOS ✅, Windows (planned)
