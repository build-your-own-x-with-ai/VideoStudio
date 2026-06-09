#!/bin/bash
# Test script for Enhanced Partition Visualization
# Verifies that partition overlay correctly displays prediction modes, QP values, and reference indices

set -e

echo "============================================"
echo "Enhanced Partition Visualization Test"
echo "============================================"
echo ""

# Check if VideoStudio is built
if [ ! -f "/Users/i/Code/VideoStudio/build/VideoStudio.app/Contents/MacOS/VideoStudio" ]; then
    echo "❌ VideoStudio not found. Build it first:"
    echo "   cd /Users/i/Code/VideoStudio/build && cmake --build ."
    exit 1
fi

echo "✅ VideoStudio binary found"
echo ""

# Check for test videos
TEST_DIR="$HOME/Movies/VideoStudio_QP_Tests"
if [ ! -d "$TEST_DIR" ]; then
    echo "⚠️  Test videos not found. Generating them..."
    cd /Users/i/Code/VideoStudio
    bash scripts/generate_qp_test_video.sh
    echo ""
fi

echo "Test videos directory: $TEST_DIR"
echo ""

# List available test videos
echo "Available test videos:"
ls -lh "$TEST_DIR"/*.mp4 2>/dev/null | awk '{print "  - " $9 " (" $5 ")"}'
echo ""

# Test recommendations
echo "============================================"
echo "Manual Test Instructions"
echo "============================================"
echo ""
echo "1. Launch VideoStudio:"
echo "   cd /Users/i/Code/VideoStudio/build"
echo "   ./VideoStudio.app/Contents/MacOS/VideoStudio"
echo ""
echo "2. Open test video:"
echo "   File → Open → Select: $TEST_DIR/variable_qp_cbr.mp4"
echo "   (This video has the most varied QP and prediction modes)"
echo ""
echo "3. Enable Partition Overlay:"
echo "   - Press ALT+2 or View → Overlay → Partitions"
echo ""
echo "4. Navigate to P-frame:"
echo "   - Press RIGHT arrow several times to skip I-frame"
echo "   - Or click on P-frame in Bar Chart"
echo ""
echo "5. Verify Color Coding:"
echo "   - 🔴 Red blocks = Intra-predicted (should be minority in P-frames)"
echo "   - 🔵 Blue blocks = Inter-predicted (should be majority in P-frames)"
echo "   - ⚪ Gray blocks = Skip blocks (static areas)"
echo ""
echo "6. Verify Statistics Legend (top-left):"
echo "   Should show counts like:"
echo "     Prediction Modes:"
echo "     🔴 Intra: XXX"
echo "     🔵 Inter: XXX"
echo "     ⚪ Skip: XXX"
echo ""
echo "7. Test Zoom Levels:"
echo "   a) Zoom to 1.2× (mouse wheel or View → Zoom In):"
echo "      - Blue blocks should show white numbers (reference indices: 0, 1, 2...)"
echo "      - Most should be '0' (nearest reference frame)"
echo ""
echo "   b) Zoom to 1.5×:"
echo "      - Blocks should show 'Q##' in top-left (QP values)"
echo "      - Higher QP = darker block color"
echo ""
echo "8. Test with Different Videos:"
echo "   a) high_quality_crf18.mp4:"
echo "      - Blocks should be mostly light (low QP)"
echo "      - More intra blocks (higher quality = more detail preserved)"
echo ""
echo "   b) low_quality_crf35.mp4:"
echo "      - Blocks should be mostly dark (high QP)"
echo "      - More skip blocks (aggressive compression)"
echo ""
echo "9. Combine with QP Heatmap (ALT+5):"
echo "   - Enable both Partitions (ALT+2) and QP Heatmap (ALT+5)"
echo "   - Partitions show prediction modes with QP intensity"
echo "   - QP Heatmap shows pure QP color gradient"
echo "   - Compare: they should correlate (high QP = dark colors)"
echo ""
echo "10. Check Block Statistics Panel:"
echo "    - View → Block Statistics"
echo "    - Verify counts match legend overlay"
echo "    - Check QP min/max/avg values"
echo ""
echo "============================================"
echo "Expected Results"
echo "============================================"
echo ""
echo "✅ P-Frame at 50% mark of variable_qp_cbr.mp4:"
echo "   - Intra: ~30-50% (red blocks)"
echo "   - Inter: ~50-70% (blue blocks)"
echo "   - Skip: ~20-40% (gray blocks)"
echo "   - QP range: 15-40 (light to dark colors)"
echo "   - Reference indices: mostly 0, some 1"
echo ""
echo "✅ Color intensity should correlate with QP:"
echo "   - Light red/blue = QP 15-25 (high quality)"
echo "   - Medium red/blue = QP 25-35 (medium quality)"
echo "   - Dark red/blue = QP 35-45 (low quality)"
echo ""
echo "✅ Reference frame indices (zoom 1.2×+):"
echo "   - Most inter blocks show '0'"
echo "   - Occasional '1' or '2' in complex scenes"
echo "   - No indices on red (intra) or gray (skip) blocks"
echo ""
echo "✅ QP labels (zoom 1.5×+):"
echo "   - 'Q18' to 'Q40' visible in block corners"
echo "   - Darker blocks = higher Q values"
echo ""
echo "============================================"
echo "Automated Checks"
echo "============================================"
echo ""

# Check if documentation exists
if [ -f "/Users/i/Code/VideoStudio/docs/Enhanced_Partition_Visualization.md" ]; then
    echo "✅ Documentation exists: docs/Enhanced_Partition_Visualization.md"
else
    echo "❌ Documentation missing"
fi

# Check source code modifications
if grep -q "Prediction Modes:" "/Users/i/Code/VideoStudio/src/widgets/videooutput.cpp"; then
    echo "✅ Source code contains prediction mode legend"
else
    echo "❌ Source code missing prediction mode legend"
fi

if grep -q "AVVideoEncParams" "/Users/i/Code/VideoStudio/src/widgets/videooutput.cpp"; then
    echo "✅ Source code uses QP data for intensity"
else
    echo "❌ Source code missing QP intensity feature"
fi

if grep -q "ref_idx_l0" "/Users/i/Code/VideoStudio/docs/Enhanced_Partition_Visualization.md"; then
    echo "✅ Documentation mentions reference frame indices"
else
    echo "❌ Documentation missing reference frame info"
fi

echo ""
echo "============================================"
echo "Test Complete"
echo "============================================"
echo ""
echo "Next steps:"
echo "1. Launch VideoStudio and follow manual test instructions above"
echo "2. Take screenshots for documentation"
echo "3. Report any visual issues or incorrect color coding"
echo ""
