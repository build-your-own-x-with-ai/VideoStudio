# Enhanced Partition Visualization - Quick Reference

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `ALT+2` | Toggle Partitions overlay |
| `ALT+5` | Toggle QP Heatmap overlay |
| `ALT+G` | Toggle Standard Grid Mode |
| Mouse Wheel | Zoom in/out |

## Color Codes

### Block Fill Colors (Prediction Modes)

```
🔴 RED    = Intra-predicted (frame-internal)
🔵 BLUE   = Inter-predicted (uses reference frames)
⚪ GRAY   = Skip blocks (zero motion)
```

### Color Intensity (QP Quality)

```
Light (translucent) = Low QP (15-25)  → High quality
Medium              = Mid QP (25-35)  → Medium quality  
Dark (opaque)       = High QP (35-51) → Low quality
```

### Border Colors (Block Sizes)

```
🟡 Yellow (thick)  = 32×32 or larger
🟠 Orange (thin)   = 16×16
🟡 Dim Yellow      = 8×8 or smaller
```

## What You See at Different Zoom Levels

| Zoom | Features Visible |
|------|-----------------|
| 0.8× | ✓ Block fills ✓ Borders ✓ Legend |
| 1.2× | ✓ All above ✓ Reference indices (white numbers) |
| 1.5× | ✓ All above ✓ QP values (Q## labels) |

## Statistics Legend (Top-Left Corner)

```
Prediction Modes:
🔴 Intra: 885    ← Red blocks
🔵 Inter: 908    ← Blue blocks  
⚪ Skip: 886     ← Gray blocks
```

## Reference Frame Indices (Zoom 1.2×+)

White numbers in blue blocks show which reference frame is used:

```
0 = Most recent reference (common)
1 = Previous reference
2 = Older reference (long-term memory)
```

## QP Labels (Zoom 1.5×+)

Top-left corner of blocks shows QP value:

```
Q18 = High quality (light color)
Q28 = Medium quality
Q38 = Low quality (dark color)
```

## Typical Patterns by Frame Type

### I-Frame (Keyframe)
- Shows standard 16×16 grid
- No prediction mode colors (all intra by definition)
- Yellow borders only

### P-Frame
- 50-70% blue (inter)
- 20-40% gray (skip)
- 10-20% red (intra)
- Most reference indices = 0

### B-Frame
- 60-80% blue/gray (inter/skip)
- 10-30% red (intra)
- Reference indices vary (0, 1, 2)

## Common Interpretations

### Mostly Light Blue
✅ Good quality inter prediction  
✅ Efficient motion compensation

### Mostly Dark Blue  
⚠️ High compression on inter blocks  
⚠️ Possible motion blur

### Mostly Red
⚠️ Scene change or I-frame  
⚠️ Encoder not using motion compensation

### Mostly Gray
✅ Static scene  
✅ Skip blocks working efficiently

### Mixed Light/Dark
⚠️ Variable quality (rate control active)  
⚠️ Spatial quality adaptation

## Combining Overlays

### Partitions + QP Heatmap
- Partitions: Prediction + QP intensity
- QP Heatmap: Pure QP gradient
- Use together: Complete quality picture

### Partitions + Motion Vectors
- Partitions: Prediction type
- Motion Vectors: Motion direction/magnitude
- Use together: Verify skip blocks have zero MVs

### Partitions + Block Statistics
- Partitions: Visual representation
- Block Statistics: Numerical data
- Use together: Verify counts match

## Troubleshooting

| Problem | Solution |
|---------|----------|
| All blocks yellow | Navigate to P/B frame (not I-frame) |
| No color variations | Use software-encoded video (not hardware) |
| No reference indices | Zoom to at least 1.2× |
| No QP labels | Zoom to at least 1.5× |
| Legend shows zeros | Move to P or B frame |

## Best Test Videos

```
✅ variable_qp_cbr.mp4     → Most varied (best for testing)
✅ high_quality_crf18.mp4  → Light colors (low QP)
✅ low_quality_crf35.mp4   → Dark colors (high QP)
```

## Tips

1. **Start with P-frames** (I-frames show grid only)
2. **Zoom in gradually** to see more details
3. **Compare with Block Statistics** panel for numbers
4. **Use QP Heatmap together** for complete view
5. **Test with different videos** to see encoder variations

---

**Quick Start**: Press `ALT+2`, navigate to P-frame, zoom in!
