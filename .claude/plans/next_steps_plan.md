# VideoStudio - Next Development Steps Plan

## Current State Analysis

### ✅ Completed Features (Version 1.2)
- **Core Functionality**: 17,442 lines of C++ code across 77 source files
- **Container Parsers**: TS, MP4, MKV, AVI, FLV (all complete)
- **Raw YUV Viewer**: Format detection, playback, export
- **Analysis Panels**: 11 panels (Bitrate, Buffer, TR 101-290, Time Dynamics, Hex Viewer, Explorer, Property, Messages, EPG, Graphics, Comments)
- **Quality Metrics**: PSNR/SSIM comparison dialog
- **Export Features**: YUV frames, CSV metrics, stream info
- **Context Menus**: Dump elementary streams, extract atoms/elements/chunks
- **UI**: Professional dockable interface with dark theme

### 🔍 Identified TODOs
1. **PropertyPanel** (4 TODOs):
   - Compare mode for MP4 atoms (line 698)
   - Compare mode for MKV elements (line 862)
   - Compare mode for AVI chunks (line 1074)
   - Compare mode for FLV tags (line 1260)
   - PSI/SI table parsing (line 365)

2. **GraphicsPanel** (3 TODOs):
   - Parameter removal dialog (line 553)
   - CSV export functionality (line 567)
   - Parameter list display update (line 572)

## 📋 Recommended Next Steps

### Priority 1: Complete Existing Features (1-2 days)

#### Task 1.1: Property Panel Compare Mode
**Effort**: 3-4 hours  
**Value**: High - enhances debugging workflow

**Implementation**:
- Store previous selected item (atom/element/chunk/tag) in PropertyPanel
- Create side-by-side comparison view
- Highlight differences in fields (offset, size, values)
- Add "Clear Comparison" button
- Support comparison across different items of same type

**Files to modify**:
- `src/panels/propertypanel.h` - Add comparison state members
- `src/panels/propertypanel.cpp` - Implement 4 compare functions

#### Task 1.2: Graphics Panel Enhancements
**Effort**: 2-3 hours  
**Value**: Medium - improves usability

**Implementation**:
- Parameter removal dialog with list selection
- CSV export for chart data (time series)
- Update parameter list display in UI

**Files to modify**:
- `src/panels/graphicspanel.cpp` - Complete 3 TODO functions

#### Task 1.3: PSI/SI Table Parsing
**Effort**: 4-6 hours  
**Value**: High - professional TS analysis feature

**Implementation**:
- Parse PAT (Program Association Table)
- Parse PMT (Program Map Table)
- Parse SDT (Service Description Table)
- Parse EIT (Event Information Table) - already in EPG panel
- Display table fields in Property Panel

**Files to modify**:
- `src/panels/propertypanel.cpp` - Implement `addPSITableFields()`
- `src/core/tsparser.h/cpp` - Add table parsing helpers

### Priority 2: Motion Vector Visualization (3-5 days)

**Effort**: 6-8 hours  
**Value**: Very High - flagship professional feature

**Why this is important**:
- Differentiates VideoStudio from basic players
- Essential for codec debugging and optimization
- Requested feature in roadmap
- Leverages existing FFmpeg integration

**Implementation Plan**:

1. **Extract Motion Vectors from FFmpeg** (2-3 hours)
   - Use `av_frame_get_side_data()` with `AV_FRAME_DATA_MOTION_VECTORS`
   - Store motion vector data per frame
   - Parse motion vector structure (source, destination, motion scale)

2. **Create Motion Vector Overlay Widget** (2-3 hours)
   - Extend `VideoOutput` widget
   - Draw arrows on video frame
   - Support different visualization modes:
     - Arrows (direction + magnitude)
     - Heatmap (motion intensity)
     - Grid overlay (block boundaries)

3. **Add Graphics Panel Controls** (1-2 hours)
   - Toggle motion vector display on/off
   - Scale factor slider (arrow length)
   - Color scheme selector
   - Filter by motion threshold
   - Block size selector (4x4, 8x8, 16x16)

**Files to create/modify**:
- `src/core/videodecoder.cpp` - Extract motion vectors
- `src/widgets/videooutput.cpp` - Render motion vector overlay
- `src/panels/graphicspanel.cpp` - Add MV controls
- `src/core/motionvector.h` - Data structure for MV storage

### Priority 3: Block/Partition Visualization (2-3 days)

**Effort**: 4-6 hours  
**Value**: High - complements motion vector feature

**Implementation**:
- Extract block/partition information from FFmpeg
- Visualize coding tree units (CTU) for HEVC
- Visualize macroblocks for H.264
- Color-code by prediction mode (Intra/Inter)
- Show transform block boundaries

**Files to create/modify**:
- `src/core/videodecoder.cpp` - Extract block data
- `src/widgets/videooutput.cpp` - Render block overlay
- `src/panels/graphicspanel.cpp` - Add block visualization controls

### Priority 4: UI/UX Improvements (1-2 days)

#### Task 4.1: Recent Files List
**Effort**: 1-2 hours  
**Value**: High - improves workflow

**Implementation**:
- Store recent files in QSettings
- Add "File → Open Recent" submenu
- Limit to 10 most recent files
- Clear recent files option

#### Task 4.2: Drag & Drop Support
**Effort**: 1-2 hours  
**Value**: High - modern UX expectation

**Implementation**:
- Accept file drops on main window
- Support multiple file types
- Show loading indicator

#### Task 4.3: Keyboard Shortcuts Customization
**Effort**: 2-3 hours  
**Value**: Medium - power user feature

**Implementation**:
- Settings dialog for shortcuts
- Save/load from QSettings
- Reset to defaults option

### Priority 5: Performance Optimizations (1-2 days)

#### Task 5.1: Large File Handling
**Effort**: 3-4 hours  
**Value**: High - enables professional use cases

**Improvements**:
- Lazy loading for packet view (virtual scrolling)
- Streaming parser for large TS files
- Memory-mapped file access for hex viewer
- Progressive thumbnail generation

#### Task 5.2: Multi-threading
**Effort**: 2-3 hours  
**Value**: Medium - improves responsiveness

**Improvements**:
- Background thumbnail generation
- Parallel quality metrics calculation
- Async file parsing

## 🎯 Recommended Implementation Order

### Week 1: Complete Existing Features
1. **Day 1-2**: Property Panel Compare Mode (Task 1.1)
2. **Day 2**: Graphics Panel Enhancements (Task 1.2)
3. **Day 3**: PSI/SI Table Parsing (Task 1.3)
4. **Day 3**: Recent Files + Drag & Drop (Tasks 4.1, 4.2)

### Week 2: Motion Vector Visualization
5. **Day 4-5**: Extract motion vectors from FFmpeg
6. **Day 6**: Create motion vector overlay widget
7. **Day 7**: Add Graphics Panel controls and polish

### Week 3: Block Visualization & Polish
8. **Day 8-9**: Block/partition visualization
9. **Day 10**: Performance optimizations
10. **Day 10**: Testing, bug fixes, documentation

## 📊 Effort vs Value Matrix

```
High Value, Low Effort (Do First):
- Recent Files List ⭐⭐⭐⭐⭐
- Drag & Drop Support ⭐⭐⭐⭐⭐
- Property Panel Compare Mode ⭐⭐⭐⭐⭐
- Graphics Panel Enhancements ⭐⭐⭐⭐

High Value, High Effort (Strategic):
- Motion Vector Visualization ⭐⭐⭐⭐⭐
- PSI/SI Table Parsing ⭐⭐⭐⭐⭐
- Block/Partition Visualization ⭐⭐⭐⭐
- Performance Optimizations ⭐⭐⭐⭐

Medium Value, Low Effort (Nice to Have):
- Keyboard Shortcuts Customization ⭐⭐⭐
```

## 🚀 Version Roadmap

### Version 1.3 (Current Plan - 2-3 weeks)
- ✅ Complete all TODOs
- ✅ Property Panel Compare Mode
- ✅ Graphics Panel enhancements
- ✅ PSI/SI table parsing
- ✅ Recent files + drag & drop
- ✅ Motion vector visualization
- ✅ Block/partition visualization

### Version 1.4 (Future - 1 month)
- VMAF quality metric integration
- Batch processing dialog
- Advanced export features (PDF reports, HTML)
- Command-line tool
- Performance optimizations for large files

### Version 2.0 (Future - 3 months)
- Plugin system
- Multi-platform support (Windows, Linux)
- Cloud integration
- AI-assisted analysis

## 💡 Key Insights

1. **Current State**: VideoStudio is feature-complete for basic analysis with 17K+ LOC
2. **Low-hanging Fruit**: 7 TODOs can be completed in 1-2 days
3. **Differentiator**: Motion vector visualization is the key feature to compete with commercial tools
4. **User Experience**: Recent files and drag & drop are essential modern UX features
5. **Professional Use**: PSI/SI parsing and performance optimizations enable professional workflows

## 🎬 Immediate Next Action

**Recommended**: Start with **Property Panel Compare Mode** (Task 1.1)

**Why**:
- Completes existing feature (not new development)
- High user value for debugging workflows
- Moderate effort (3-4 hours)
- Builds on existing infrastructure
- Demonstrates attention to detail

**Alternative**: Start with **Recent Files + Drag & Drop** (Tasks 4.1, 4.2)

**Why**:
- Quick wins (2-3 hours total)
- Immediate UX improvement
- Modern application expectation
- Easy to implement and test
