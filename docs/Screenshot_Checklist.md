# Screenshot Verification Checklist

## Screenshot 1: Basic Partition Overlay (Zoom 1.0×)

**File**: `partition_basic.png`

**Setup**:
- Video: variable_qp_cbr.mp4
- Frame: P-frame (around frame 10-20)
- Overlay: Partitions enabled (ALT+2)
- Zoom: 1.0× (fit to window)

**Expected Elements**:
- ✅ Red blocks (Intra-predicted) - should be visible
- ✅ Blue blocks (Inter-predicted) - should be majority
- ✅ Gray blocks (Skip blocks) - should be visible in static areas
- ✅ Statistics legend in top-left corner showing:
  ```
  Prediction Modes:
  🔴 Intra: XXX
  🔵 Inter: XXX
  ⚪ Skip: XXX
  ```
- ✅ Yellow/orange block borders visible
- ✅ Color intensity variations (light vs dark blocks)
- ✅ Video dimensions and frame info visible in bottom status bar

**What to Verify**:
- [ ] Blocks are filled with colors (not just borders)
- [ ] Three distinct colors: red, blue, gray
- [ ] Legend shows non-zero counts for all three types
- [ ] Some blocks appear lighter, some darker (QP intensity working)

---

## Screenshot 2: Reference Frame Indices (Zoom 1.2×)

**File**: `partition_reference_indices.png`

**Setup**:
- Same video and frame as Screenshot 1
- Zoom: 1.2× (zoom in once or twice)
- Overlay: Partitions still enabled

**Expected Elements**:
- ✅ White numbers in center of blue blocks (reference indices)
- ✅ Most numbers should be "0" (nearest reference)
- ✅ Occasional "1" or "2" (older references)
- ✅ No numbers in red blocks (intra, no reference)
- ✅ No numbers in gray blocks (skip, but uses default reference)
- ✅ Statistics legend still visible
- ✅ Block colors still visible with intensity variations

**What to Verify**:
- [ ] Can clearly read white numbers inside blocks
- [ ] Numbers are centered in blocks
- [ ] Most common number is "0"
- [ ] Red blocks definitely have no numbers

---

## Screenshot 3: QP Values (Zoom 1.5×+)

**File**: `partition_qp_labels.png`

**Setup**:
- Same video and frame
- Zoom: 1.5× or higher (zoom in several times)
- Overlay: Partitions enabled

**Expected Elements**:
- ✅ "Q##" labels in top-left corner of blocks
- ✅ QP values range from Q15 to Q40
- ✅ Darker blocks have higher Q values (Q35-Q40)
- ✅ Lighter blocks have lower Q values (Q15-Q25)
- ✅ Reference indices still visible in blue blocks
- ✅ Can see individual block details clearly

**What to Verify**:
- [ ] QP labels are readable (Q followed by number)
- [ ] QP values match block color intensity (high QP = dark color)
- [ ] Labels positioned in top-left corner of blocks
- [ ] Both QP labels and reference indices visible simultaneously

---

## Screenshot 4: Block Statistics Panel Integration

**File**: `partition_with_statistics.png`

**Setup**:
- Same video and frame
- Zoom: Back to 1.0× (fit to window)
- Overlays: Partitions enabled (ALT+2)
- Panel: Block Statistics panel visible (View → Block Statistics)

**Expected Elements**:
- ✅ Left side: Video with colored partition overlay
- ✅ Right side: Block Statistics panel showing:
  - Overall Video Statistics (frame type distribution)
  - Current Frame Statistics (block counts)
  - Quantization Parameter (QP) section
  - Motion Statistics
  - Reference Frame Usage
  - Block Size Distribution
- ✅ Statistics legend on video matches panel counts
- ✅ QP values in panel match visible block intensities

**What to Verify**:
- [ ] Intra/Inter/Skip counts in legend match panel "Current Frame Statistics"
- [ ] QP Min/Max/Avg in panel correlate with visible color intensity
- [ ] Panel shows "Overall Video Statistics" section at top
- [ ] Reference Frame Usage section in panel shows ref_idx_l0[0], [1], etc.

---

## Screenshot 5: Multiple Overlays (Partitions + QP Heatmap)

**File**: `partition_plus_qp_heatmap.png`

**Setup**:
- Same video and frame
- Zoom: 1.0×
- Overlays: Both Partitions (ALT+2) AND QP Heatmap (ALT+5) enabled

**Expected Elements**:
- ✅ Partitions show prediction mode colors with borders
- ✅ QP Heatmap shows green→yellow→red gradient
- ✅ Both overlays visible simultaneously
- ✅ QP legend in bottom-right (from heatmap)
- ✅ Prediction mode legend in top-left (from partitions)
- ✅ Colors should correlate: dark partition blocks = red/yellow heatmap

**What to Verify**:
- [ ] Can distinguish both overlays
- [ ] Two separate legends visible
- [ ] High QP areas (red/yellow heatmap) have darker partition colors
- [ ] Low QP areas (green heatmap) have lighter partition colors

---

## Screenshot 6: I-Frame vs P-Frame Comparison

**File**: `partition_iframe_vs_pframe.png`

**Setup**:
- Navigate to I-frame (frame 0) - take left half screenshot
- Navigate to P-frame (frame 10+) - take right half screenshot
- Both with Partitions overlay enabled
- Side-by-side comparison

**Expected Elements**:
- ✅ I-frame: Standard yellow 16×16 grid, no colors
- ✅ P-frame: Red/blue/gray colored blocks
- ✅ Clear visual difference between frame types

**What to Verify**:
- [ ] I-frame shows only grid lines (no fill colors)
- [ ] P-frame shows filled colored blocks
- [ ] Legend on P-frame shows counts, I-frame legend shows "I frame" info

---

## Screenshot 7: Different Quality Levels

**File**: `partition_quality_comparison.png`

**Setup**:
- Open three videos side by side (or sequential screenshots):
  1. high_quality_crf18.mp4 - mostly light colors
  2. medium_quality_crf23.mp4 - medium colors
  3. low_quality_crf35.mp4 - mostly dark colors
- All at same P-frame position
- Partitions overlay enabled

**Expected Elements**:
- ✅ High quality: Blocks mostly light/translucent
- ✅ Medium quality: Mixed light and medium colors
- ✅ Low quality: Blocks mostly dark/opaque
- ✅ QP values visible at zoom 1.5× correlate with quality

**What to Verify**:
- [ ] Clear visual progression from light to dark
- [ ] High quality has more intra blocks (red)
- [ ] Low quality has more skip blocks (gray)
- [ ] Color intensity directly relates to CRF/quality

---

## Additional Screenshots (Optional)

### Standard Grid Mode Toggle
**File**: `partition_standard_grid_mode.png`
- Press ALT+G to enable Standard Grid Mode
- Shows uniform 16×16 grid even on P-frame
- Useful for comparison

### Cursor Mode with Block Info
**File**: `partition_cursor_info.png`
- Press ALT+C to enable Cursor Mode
- Hover over different colored blocks
- Should show block details in tooltip

---

## Verification Summary

After taking all screenshots, verify:

1. **Colors are correct**:
   - Red = Intra
   - Blue = Inter
   - Gray = Skip

2. **Intensity correlates with QP**:
   - Light colors = Low QP (high quality)
   - Dark colors = High QP (low quality)

3. **Zoom levels work**:
   - 1.0×: Colors + legend
   - 1.2×: + Reference indices
   - 1.5×: + QP labels

4. **Integration works**:
   - Statistics panel counts match overlay legend
   - QP heatmap correlates with partition colors
   - Multiple overlays can coexist

5. **Documentation matches reality**:
   - All features described in docs are visible
   - Color codes match documentation
   - Keyboard shortcuts work as documented

---

## Screenshot Naming Convention

```
partition_basic.png
partition_reference_indices.png
partition_qp_labels.png
partition_with_statistics.png
partition_plus_qp_heatmap.png
partition_iframe_vs_pframe.png
partition_quality_comparison.png
partition_standard_grid_mode.png (optional)
partition_cursor_info.png (optional)
```

Save to: `/Users/i/Code/VideoStudio/docs/screenshots/`
