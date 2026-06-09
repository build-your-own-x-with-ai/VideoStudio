#!/bin/bash
# Generate test videos with QP data for VideoStudio QP Heatmap testing
# Author: VideoStudio Development Team
# Date: 2026-06-09

set -e

# Check if ffmpeg is installed
if ! command -v ffmpeg &> /dev/null; then
    echo "Error: ffmpeg is not installed"
    echo "Install with: brew install ffmpeg"
    exit 1
fi

# Output directory
OUTPUT_DIR="$HOME/Movies/VideoStudio_QP_Tests"
mkdir -p "$OUTPUT_DIR"

echo "============================================"
echo "VideoStudio QP Heatmap Test Video Generator"
echo "============================================"
echo ""
echo "Output directory: $OUTPUT_DIR"
echo ""

# Source video (use existing video or generate test pattern)
SOURCE="$HOME/Movies/TheaterSquare_640x360.mp4"

if [ ! -f "$SOURCE" ]; then
    echo "Source video not found. Generating test pattern..."
    # Generate 10-second test pattern with movement
    ffmpeg -f lavfi -i testsrc=duration=10:size=640x360:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=10 \
           -c:v libx264 -preset fast -crf 23 -pix_fmt yuv420p \
           -c:a aac -b:a 128k \
           "$OUTPUT_DIR/test_pattern.mp4" -y
    SOURCE="$OUTPUT_DIR/test_pattern.mp4"
    echo "✓ Test pattern generated"
fi

echo ""
echo "Generating test videos with different QP characteristics..."
echo ""

# Test 1: High Quality (CRF 18) - Low QP, mostly green
echo "1/5 Generating: high_quality_crf18.mp4 (Low QP, Green heatmap)"
ffmpeg -i "$SOURCE" \
       -c:v libx264 \
       -preset slow \
       -crf 18 \
       -tune film \
       -x264-params "aq-mode=2:aq-strength=1.0" \
       -c:a copy \
       "$OUTPUT_DIR/high_quality_crf18.mp4" -y -v quiet -stats

echo "   ✓ Expected QP: 15-22 (Green colors)"
echo ""

# Test 2: Medium Quality (CRF 23) - Medium QP, yellow-green
echo "2/5 Generating: medium_quality_crf23.mp4 (Medium QP, Yellow-Green)"
ffmpeg -i "$SOURCE" \
       -c:v libx264 \
       -preset medium \
       -crf 23 \
       -tune film \
       -x264-params "aq-mode=2:aq-strength=1.0" \
       -c:a copy \
       "$OUTPUT_DIR/medium_quality_crf23.mp4" -y -v quiet -stats

echo "   ✓ Expected QP: 20-28 (Yellow-Green colors)"
echo ""

# Test 3: Low Quality (CRF 35) - High QP, orange-red
echo "3/5 Generating: low_quality_crf35.mp4 (High QP, Orange-Red)"
ffmpeg -i "$SOURCE" \
       -c:v libx264 \
       -preset fast \
       -crf 35 \
       -tune film \
       -x264-params "aq-mode=2:aq-strength=1.0" \
       -c:a copy \
       "$OUTPUT_DIR/low_quality_crf35.mp4" -y -v quiet -stats

echo "   ✓ Expected QP: 32-40 (Orange-Red colors)"
echo ""

# Test 4: Variable QP (CBR mode) - Wide QP range
echo "4/5 Generating: variable_qp_cbr.mp4 (CBR, Wide QP range)"
ffmpeg -i "$SOURCE" \
       -c:v libx264 \
       -preset medium \
       -b:v 500k \
       -minrate 500k \
       -maxrate 500k \
       -bufsize 1000k \
       -x264-params "aq-mode=2:aq-strength=1.5" \
       -c:a copy \
       "$OUTPUT_DIR/variable_qp_cbr.mp4" -y -v quiet -stats

echo "   ✓ Expected QP: 15-40 (Wide color range - Green to Red)"
echo ""

# Test 5: HEVC/H.265 version
echo "5/5 Generating: hevc_quality_test.mp4 (H.265, Medium QP)"
ffmpeg -i "$SOURCE" \
       -c:v libx265 \
       -preset medium \
       -crf 23 \
       -x265-params "aq-mode=2:aq-strength=1.0" \
       -tag:v hvc1 \
       -c:a copy \
       "$OUTPUT_DIR/hevc_quality_test.mp4" -y -v quiet -stats

echo "   ✓ Expected QP: 20-28 (Yellow-Green colors)"
echo ""

echo "============================================"
echo "✓ All test videos generated successfully!"
echo "============================================"
echo ""
echo "Test files created in: $OUTPUT_DIR"
echo ""
echo "File List:"
ls -lh "$OUTPUT_DIR"/*.mp4 | awk '{print "  - " $9 " (" $5 ")"}'
echo ""
echo "How to test in VideoStudio:"
echo "1. Open VideoStudio"
echo "2. File → Open → Select any test video above"
echo "3. Enable: View → Overlay → QP Heatmap (ALT+5)"
echo "4. Or check 'QP Heatmap (ALT+5)' in Overlays panel"
echo "5. Navigate through frames to see QP variations"
echo ""
echo "Expected Results:"
echo "  - high_quality_crf18.mp4:  Green heatmap (low QP)"
echo "  - medium_quality_crf23.mp4: Yellow-Green (medium QP)"
echo "  - low_quality_crf35.mp4:    Orange-Red (high QP)"
echo "  - variable_qp_cbr.mp4:      Mixed colors (wide QP range)"
echo "  - hevc_quality_test.mp4:    Yellow-Green (H.265 test)"
echo ""
