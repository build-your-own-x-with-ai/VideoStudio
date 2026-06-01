#!/bin/bash

# VideoStudio Test File Generator
# Generates various video format test files for testing the application

set -e

OUTPUT_DIR="test_files"
mkdir -p "$OUTPUT_DIR"

echo "=== VideoStudio Test File Generator ==="
echo "Output directory: $OUTPUT_DIR"
echo ""

# Check if ffmpeg is available
if ! command -v ffmpeg &> /dev/null; then
    echo "Error: ffmpeg is not installed"
    echo "Install with: brew install ffmpeg"
    exit 1
fi

# Generate a simple test pattern video (10 seconds, 30fps, 640x480)
generate_base_video() {
    echo "Generating base test pattern video..."
    ffmpeg -f lavfi -i testsrc=duration=10:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=10 \
           -pix_fmt yuv420p \
           -y "$OUTPUT_DIR/base_test.yuv" 2>&1 | grep -v "frame="
    echo "✓ Base test pattern created"
}

# 1. AVI Format Tests
generate_avi_files() {
    echo ""
    echo "=== Generating AVI Files ==="

    # AVI with MJPEG codec
    echo "1. AVI + MJPEG..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v mjpeg -q:v 5 -c:a pcm_s16le \
           -y "$OUTPUT_DIR/test_mjpeg.avi" 2>&1 | grep -v "frame="
    echo "✓ test_mjpeg.avi"

    # AVI with H.264 codec
    echo "2. AVI + H.264..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -y "$OUTPUT_DIR/test_h264.avi" 2>&1 | grep -v "frame="
    echo "✓ test_h264.avi"

    # AVI with MPEG-4 codec
    echo "3. AVI + MPEG-4..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v mpeg4 -q:v 5 -c:a mp3 \
           -y "$OUTPUT_DIR/test_mpeg4.avi" 2>&1 | grep -v "frame="
    echo "✓ test_mpeg4.avi"

    # AVI with uncompressed video
    echo "4. AVI + Uncompressed..."
    ffmpeg -f lavfi -i testsrc=duration=2:size=320x240:rate=15 \
           -c:v rawvideo -pix_fmt bgr24 \
           -y "$OUTPUT_DIR/test_uncompressed.avi" 2>&1 | grep -v "frame="
    echo "✓ test_uncompressed.avi"
}

# 2. H.264 Raw Stream Tests
generate_h264_files() {
    echo ""
    echo "=== Generating H.264 Raw Streams ==="

    # H.264 Baseline Profile
    echo "1. H.264 Baseline Profile..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -c:v libx264 -profile:v baseline -level 3.0 -preset fast \
           -y "$OUTPUT_DIR/test_h264_baseline.h264" 2>&1 | grep -v "frame="
    echo "✓ test_h264_baseline.h264"

    # H.264 Main Profile
    echo "2. H.264 Main Profile..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -c:v libx264 -profile:v main -level 3.1 -preset fast \
           -y "$OUTPUT_DIR/test_h264_main.h264" 2>&1 | grep -v "frame="
    echo "✓ test_h264_main.h264"

    # H.264 High Profile
    echo "3. H.264 High Profile..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -c:v libx264 -profile:v high -level 4.0 -preset fast \
           -y "$OUTPUT_DIR/test_h264_high.h264" 2>&1 | grep -v "frame="
    echo "✓ test_h264_high.h264"

    # H.264 with B-frames
    echo "4. H.264 with B-frames..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -c:v libx264 -bf 2 -g 30 -preset fast \
           -y "$OUTPUT_DIR/test_h264_bframes.h264" 2>&1 | grep -v "frame="
    echo "✓ test_h264_bframes.h264"
}

# 3. H.265/HEVC Tests
generate_h265_files() {
    echo ""
    echo "=== Generating H.265/HEVC Streams ==="

    # H.265 Main Profile
    echo "1. H.265 Main Profile..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -c:v libx265 -preset fast -crf 28 \
           -y "$OUTPUT_DIR/test_h265_main.h265" 2>&1 | grep -v "frame="
    echo "✓ test_h265_main.h265"

    # H.265 Main10 Profile (10-bit)
    echo "2. H.265 Main10 Profile..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -c:v libx265 -preset fast -crf 28 -pix_fmt yuv420p10le \
           -y "$OUTPUT_DIR/test_h265_main10.h265" 2>&1 | grep -v "frame="
    echo "✓ test_h265_main10.h265"

    # H.265 4K
    echo "3. H.265 4K..."
    ffmpeg -f lavfi -i testsrc=duration=3:size=3840x2160:rate=30 \
           -c:v libx265 -preset ultrafast -crf 28 \
           -y "$OUTPUT_DIR/test_h265_4k.h265" 2>&1 | grep -v "frame="
    echo "✓ test_h265_4k.h265"
}

# 4. AV1 Tests
generate_av1_files() {
    echo ""
    echo "=== Generating AV1 Streams ==="

    # Check if libaom-av1 is available
    if ! ffmpeg -codecs 2>&1 | grep -q "libaom-av1"; then
        echo "⚠ AV1 encoder not available, skipping AV1 tests"
        return
    fi

    # AV1 in MP4
    echo "1. AV1 in MP4..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -c:v libaom-av1 -cpu-used 8 -crf 35 \
           -y "$OUTPUT_DIR/test_av1.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_av1.mp4"

    # AV1 in WebM
    echo "2. AV1 in WebM..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -c:v libaom-av1 -cpu-used 8 -crf 35 \
           -y "$OUTPUT_DIR/test_av1.webm" 2>&1 | grep -v "frame="
    echo "✓ test_av1.webm"
}

# 5. VP9 Tests
generate_vp9_files() {
    echo ""
    echo "=== Generating VP9 Streams ==="

    # VP9 in WebM
    echo "1. VP9 in WebM..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libvpx-vp9 -crf 30 -b:v 0 -c:a libopus \
           -y "$OUTPUT_DIR/test_vp9.webm" 2>&1 | grep -v "frame="
    echo "✓ test_vp9.webm"

    # VP9 in MKV
    echo "2. VP9 in MKV..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libvpx-vp9 -crf 30 -b:v 0 -c:a libopus \
           -y "$OUTPUT_DIR/test_vp9.mkv" 2>&1 | grep -v "frame="
    echo "✓ test_vp9.mkv"
}

# 6. MP4 Container Tests
generate_mp4_files() {
    echo ""
    echo "=== Generating MP4 Files ==="

    # MP4 with H.264
    echo "1. MP4 + H.264..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -y "$OUTPUT_DIR/test_h264.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_h264.mp4"

    # MP4 with H.265
    echo "2. MP4 + H.265..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx265 -preset fast -crf 28 -c:a aac \
           -y "$OUTPUT_DIR/test_h265.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_h265.mp4"

    # MP4 with multiple audio tracks
    echo "3. MP4 + Multiple Audio Tracks..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -f lavfi -i sine=frequency=500:duration=5 \
           -map 0:v -map 1:a -map 2:a \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -metadata:s:a:0 language=eng -metadata:s:a:0 title="English" \
           -metadata:s:a:1 language=spa -metadata:s:a:1 title="Spanish" \
           -y "$OUTPUT_DIR/test_multi_audio.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_multi_audio.mp4"
}

# 7. MKV Container Tests
generate_mkv_files() {
    echo ""
    echo "=== Generating MKV Files ==="

    # MKV with H.264
    echo "1. MKV + H.264..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -y "$OUTPUT_DIR/test_h264.mkv" 2>&1 | grep -v "frame="
    echo "✓ test_h264.mkv"

    # MKV with H.265
    echo "2. MKV + H.265..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx265 -preset fast -crf 28 -c:a aac \
           -y "$OUTPUT_DIR/test_h265.mkv" 2>&1 | grep -v "frame="
    echo "✓ test_h265.mkv"

    # MKV with chapters
    echo "3. MKV + Chapters..."
    ffmpeg -f lavfi -i testsrc=duration=10:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=10 \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -y "$OUTPUT_DIR/test_chapters.mkv" 2>&1 | grep -v "frame="
    # Add chapters
    cat > "$OUTPUT_DIR/chapters.txt" << EOF
[CHAPTER]
TIMEBASE=1/1000
START=0
END=3000
title=Chapter 1

[CHAPTER]
TIMEBASE=1/1000
START=3000
END=7000
title=Chapter 2

[CHAPTER]
TIMEBASE=1/1000
START=7000
END=10000
title=Chapter 3
EOF
    ffmpeg -i "$OUTPUT_DIR/test_chapters.mkv" -i "$OUTPUT_DIR/chapters.txt" \
           -map_metadata 1 -codec copy \
           -y "$OUTPUT_DIR/test_chapters_final.mkv" 2>&1 | grep -v "frame="
    mv "$OUTPUT_DIR/test_chapters_final.mkv" "$OUTPUT_DIR/test_chapters.mkv"
    rm "$OUTPUT_DIR/chapters.txt"
    echo "✓ test_chapters.mkv"
}

# 8. Transport Stream Tests
generate_ts_files() {
    echo ""
    echo "=== Generating Transport Stream Files ==="

    # MPEG-TS with H.264
    echo "1. MPEG-TS + H.264..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -f mpegts -y "$OUTPUT_DIR/test_h264.ts" 2>&1 | grep -v "frame="
    echo "✓ test_h264.ts"

    # MPEG-TS with H.265
    echo "2. MPEG-TS + H.265..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx265 -preset fast -crf 28 -c:a aac \
           -f mpegts -y "$OUTPUT_DIR/test_h265.ts" 2>&1 | grep -v "frame="
    echo "✓ test_h265.ts"

    # MPEG-TS with multiple programs
    echo "3. MPEG-TS + Multiple Programs..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -f lavfi -i testsrc=duration=5:size=640x480:rate=30:decimals=1 \
           -f lavfi -i sine=frequency=500:duration=5 \
           -map 0:v -map 1:a -map 2:v -map 3:a \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -program title="Program 1":st=0:st=1 \
           -program title="Program 2":st=2:st=3 \
           -f mpegts -y "$OUTPUT_DIR/test_multi_program.ts" 2>&1 | grep -v "frame="
    echo "✓ test_multi_program.ts"
}

# 9. MOV Container Tests
generate_mov_files() {
    echo ""
    echo "=== Generating MOV Files ==="

    # MOV with H.264
    echo "1. MOV + H.264..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx264 -preset fast -crf 23 -c:a aac \
           -y "$OUTPUT_DIR/test_h264.mov" 2>&1 | grep -v "frame="
    echo "✓ test_h264.mov"

    # MOV with ProRes (if available)
    if ffmpeg -codecs 2>&1 | grep -q "prores"; then
        echo "2. MOV + ProRes..."
        ffmpeg -f lavfi -i testsrc=duration=3:size=1280x720:rate=30 \
               -c:v prores_ks -profile:v 3 -pix_fmt yuv422p10le \
               -y "$OUTPUT_DIR/test_prores.mov" 2>&1 | grep -v "frame="
        echo "✓ test_prores.mov"
    else
        echo "⚠ ProRes encoder not available, skipping"
    fi
}

# 10. Special Test Cases
generate_special_files() {
    echo ""
    echo "=== Generating Special Test Cases ==="

    # Very short video (1 frame)
    echo "1. Single Frame Video..."
    ffmpeg -f lavfi -i testsrc=duration=0.033:size=640x480:rate=30 \
           -c:v libx264 -preset fast \
           -y "$OUTPUT_DIR/test_single_frame.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_single_frame.mp4"

    # High frame rate video
    echo "2. High Frame Rate (60fps)..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=1280x720:rate=60 \
           -c:v libx264 -preset fast -crf 23 \
           -y "$OUTPUT_DIR/test_60fps.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_60fps.mp4"

    # Variable frame rate
    echo "3. Variable Frame Rate..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -c:v libx264 -preset fast -crf 23 -vsync vfr \
           -y "$OUTPUT_DIR/test_vfr.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_vfr.mp4"

    # Different aspect ratios
    echo "4. Different Aspect Ratios..."
    ffmpeg -f lavfi -i testsrc=duration=3:size=1920x1080:rate=30 \
           -c:v libx264 -preset fast -crf 23 \
           -y "$OUTPUT_DIR/test_16x9.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_16x9.mp4 (16:9)"

    ffmpeg -f lavfi -i testsrc=duration=3:size=1280x1024:rate=30 \
           -c:v libx264 -preset fast -crf 23 \
           -y "$OUTPUT_DIR/test_5x4.mp4" 2>&1 | grep -v "frame="
    echo "✓ test_5x4.mp4 (5:4)"
}

# Generate FLV files
generate_flv_files() {
    echo ""
    echo "=== Generating FLV Files ==="

    # Basic FLV with H.264 video and AAC audio
    echo "1. Basic FLV (H.264 + AAC)..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=30 \
           -f lavfi -i sine=frequency=440:duration=5 \
           -c:v libx264 -preset fast -crf 23 \
           -c:a aac -b:a 128k \
           -f flv -y "$OUTPUT_DIR/test_h264_aac.flv" 2>&1 | grep -v "frame="
    echo "✓ test_h264_aac.flv"

    # FLV with VP6 video (legacy Flash codec)
    echo "2. FLV with VP6..."
    ffmpeg -f lavfi -i testsrc=duration=3:size=320x240:rate=15 \
           -c:v flv -b:v 500k \
           -f flv -y "$OUTPUT_DIR/test_vp6.flv" 2>&1 | grep -v "frame="
    echo "✓ test_vp6.flv"

    # FLV with MP3 audio
    echo "3. FLV with MP3 audio..."
    ffmpeg -f lavfi -i testsrc=duration=4:size=640x480:rate=25 \
           -f lavfi -i sine=frequency=880:duration=4 \
           -c:v libx264 -preset fast -crf 23 \
           -c:a libmp3lame -b:a 128k \
           -f flv -y "$OUTPUT_DIR/test_h264_mp3.flv" 2>&1 | grep -v "frame="
    echo "✓ test_h264_mp3.flv"

    # FLV video only (no audio)
    echo "4. FLV video only..."
    ffmpeg -f lavfi -i testsrc=duration=3:size=640x480:rate=30 \
           -c:v libx264 -preset fast -crf 23 \
           -an -f flv -y "$OUTPUT_DIR/test_video_only.flv" 2>&1 | grep -v "frame="
    echo "✓ test_video_only.flv"

    # FLV with metadata (onMetaData script tag)
    echo "5. FLV with metadata..."
    ffmpeg -f lavfi -i testsrc=duration=5:size=854x480:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -c:v libx264 -preset fast -crf 23 \
           -c:a aac -b:a 128k \
           -metadata title="Test FLV Video" \
           -metadata author="VideoStudio" \
           -f flv -y "$OUTPUT_DIR/test_metadata.flv" 2>&1 | grep -v "frame="
    echo "✓ test_metadata.flv"
}

# Main execution
main() {
    echo "Starting test file generation..."
    echo "This may take several minutes..."
    echo ""

    generate_avi_files
    generate_h264_files
    generate_h265_files
    generate_av1_files
    generate_vp9_files
    generate_mp4_files
    generate_mkv_files
    generate_ts_files
    generate_mov_files
    generate_flv_files
    generate_special_files

    echo ""
    echo "=== Generation Complete ==="
    echo "Test files created in: $OUTPUT_DIR"
    echo ""
    echo "File count:"
    ls -1 "$OUTPUT_DIR" | wc -l | xargs echo "  Total files:"
    echo ""
    echo "Total size:"
    du -sh "$OUTPUT_DIR" | awk '{print "  " $1}'
    echo ""
    echo "You can now test VideoStudio with these files:"
    echo "  ./VideoStudio.app/Contents/MacOS/VideoStudio"
}

# Run main function
main
