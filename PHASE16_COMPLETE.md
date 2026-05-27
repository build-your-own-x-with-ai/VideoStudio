# Phase 16: Quality Heatmap Implementation - COMPLETE

## Overview
Successfully integrated quality heatmap visualization into VideoStudio, allowing users to visualize quality metrics overlaid on video frames.

## Features Implemented

### 1. Quality Heatmap Analyzer (`QualityHeatmapAnalyzer`)
- **PSNR per Macroblock**: Calculates Peak Signal-to-Noise Ratio for each 16x16 block
- **SSIM per Macroblock**: Calculates Structural Similarity Index for each 16x16 block
- **Temperature Map**: Generates pixel-level absolute difference heatmap
- **Subtraction Map**: Generates pixel-level signed difference heatmap

### 2. Quality Heatmap Overlay (`QualityHeatmapOverlay`)
- Transparent overlay widget that sits on top of video display
- Four visualization modes:
  - **PSNR**: Green (high quality) → Yellow → Red (low quality), range 20-50 dB
  - **SSIM**: Green (high quality) → Yellow → Red (low quality), range 0-1
  - **Temperature**: Blue (no difference) → Green → Yellow → Red (high difference)
  - **Subtraction**: Blue (negative diff) → Gray (zero) → Red (positive diff)
- Block-based rendering for PSNR/SSIM
- Image-based rendering for Temperature/Subtraction

### 3. VideoPlayer Integration
- Added `QualityHeatmapAnalyzer` and `QualityHeatmapOverlay` members
- Added reference video decoder for comparison
- New methods:
  - `setShowQualityHeatmap(bool show, HeatmapMode mode)`: Enable/disable heatmap
  - `setReferenceVideo(const QString& filePath)`: Load reference video for PSNR/SSIM
  - `updateQualityHeatmap()`: Recalculate and update heatmap for current frame
- Automatic heatmap update on frame change

### 4. MainWindow UI Controls
- Added "质量热力图" submenu under "视图" menu with four options:
  - PSNR 热力图
  - SSIM 热力图
  - Temperature 模式
  - Subtraction 模式
- Mutual exclusion: only one heatmap mode can be active at a time
- Actions enabled when video is loaded
- Reference video selection integrated with existing quality analysis

## Technical Details

### Color Mapping Algorithms

**PSNR (20-50 dB range):**
```
normalized = (value - 20.0) / 30.0
if normalized > 0.5:
    r = 255 * (1.0 - normalized) * 2  // Green to Yellow
    g = 255
    b = 0
else:
    r = 255                            // Yellow to Red
    g = 255 * normalized * 2
    b = 0
```

**SSIM (0-1 range):**
```
Same as PSNR but with normalized = value directly
```

**Temperature (0-255 pixel difference):**
```
if diff < 64:   Blue → Green
if diff < 128:  Green → Yellow
else:           Yellow → Red
```

**Subtraction (-128 to +128 signed difference):**
```
if diff < 0:    Blue (intensity = -diff * 2)
if diff > 0:    Red (intensity = diff * 2)
if diff == 0:   Gray (128, 128, 128)
```

### Performance Considerations

1. **Frame Seeking**: For Temperature/Subtraction modes, decoder is reopened and seeks to previous frame
2. **Reference Video**: For PSNR/SSIM modes, reference decoder seeks to matching frame
3. **Block Size**: Default 16x16 macroblock size for PSNR/SSIM
4. **Y-plane Only**: Quality metrics calculated on luminance channel only

## Usage

1. **Open a video file**
2. **For PSNR/SSIM modes**:
   - Go to "质量评估" tab
   - Click "选择参考视频" to load reference video
3. **Enable heatmap**:
   - Menu: 视图 → 质量热力图 → Select mode
4. **Navigate frames**:
   - Heatmap updates automatically as you navigate

## Files Modified

### New Files
- `src/core/QualityHeatmapAnalyzer.h`
- `src/core/QualityHeatmapAnalyzer.cpp`
- `src/ui/QualityHeatmapOverlay.h`
- `src/ui/QualityHeatmapOverlay.cpp`

### Modified Files
- `src/ui/VideoPlayer.h` - Added heatmap support
- `src/ui/VideoPlayer.cpp` - Integrated heatmap overlay and analyzer
- `src/ui/MainWindow.h` - Added menu actions
- `src/ui/MainWindow.cpp` - Added menu items and toggle handlers
- `CMakeLists.txt` - Added new source files

## Testing Checklist

- [x] Build succeeds without errors
- [ ] PSNR heatmap displays correctly with reference video
- [ ] SSIM heatmap displays correctly with reference video
- [ ] Temperature mode shows frame-to-frame differences
- [ ] Subtraction mode shows signed differences
- [ ] Only one heatmap mode active at a time
- [ ] Heatmap updates when navigating frames
- [ ] Overlay scales correctly with window resize
- [ ] Performance acceptable for real-time playback

## Known Limitations

1. **Decoder Reopening**: Temperature/Subtraction modes reopen decoder for each frame (inefficient)
2. **Reference Sync**: Reference video must have same frame count and resolution
3. **Y-plane Only**: Color information not included in quality metrics
4. **No Caching**: Heatmap recalculated for every frame visit

## Future Improvements

1. **Frame Caching**: Cache decoded frames to avoid reopening decoder
2. **Multi-threaded**: Calculate heatmaps in background thread
3. **Color Channels**: Add option to analyze U/V planes
4. **Custom Ranges**: Allow user to adjust PSNR/SSIM color mapping ranges
5. **Heatmap Export**: Save heatmap images to disk
6. **Batch Mode**: Generate heatmaps for entire video sequence

## Integration with Plan

This completes **Phase 16** of the implementation plan. The quality heatmap feature provides visual quality analysis capabilities comparable to professional tools like Elecard StreamEye.

**Next Phase**: Phase 17 - Stream Viewer (NAL unit tree structure)
