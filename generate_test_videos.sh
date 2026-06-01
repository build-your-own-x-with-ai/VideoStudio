#!/bin/bash

# Generate test videos with different block sizes for VideoStudio testing
# This script creates H.264 videos with specific partition sizes

OUTPUT_DIR="test_videos"
mkdir -p "$OUTPUT_DIR"

# Generate a test pattern video (color bars with moving text)
generate_source() {
    ffmpeg -f lavfi -i testsrc=duration=5:size=640x360:rate=30 \
           -f lavfi -i sine=frequency=1000:duration=5 \
           -vf "drawtext=text='Block Size Test':fontsize=48:fontcolor=white:x=(w-text_w)/2:y=(h-text_h)/2:box=1:boxcolor=black@0.5:boxborderw=5" \
           -c:v libx264 -preset ultrafast -crf 23 -c:a aac \
           -t 5 "$OUTPUT_DIR/source.mp4" -y
}

echo "Generating source video..."
generate_source

echo ""
echo "Generating test videos with different block sizes..."
echo ""

# 1. 32x32 blocks (using partitions=p8x8,b8x8,i8x8,i4x4 and forcing larger blocks)
echo "1. Generating 32x32 block video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx264 \
       -preset ultrafast \
       -tune zerolatency \
       -x264-params "partitions=none:me=dia:subme=1:ref=1" \
       -crf 28 \
       -g 30 \
       -bf 0 \
       "$OUTPUT_DIR/test_32x32_blocks.mp4" -y

# 2. 16x16 blocks (standard macroblock, disable sub-partitions)
echo "2. Generating 16x16 block video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx264 \
       -preset ultrafast \
       -x264-params "partitions=p8x8:me=dia:subme=2:ref=1" \
       -crf 23 \
       -g 30 \
       -bf 2 \
       "$OUTPUT_DIR/test_16x16_blocks.mp4" -y

# 3. 8x8 blocks (enable 8x8 partitions)
echo "3. Generating 8x8 block video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx264 \
       -preset medium \
       -x264-params "partitions=all:8x8dct:me=umh:subme=7:ref=3" \
       -crf 20 \
       -g 30 \
       -bf 3 \
       "$OUTPUT_DIR/test_8x8_blocks.mp4" -y

# 4. 4x4 blocks (enable smallest partitions)
echo "4. Generating 4x4 block video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx264 \
       -preset veryslow \
       -x264-params "partitions=all:8x8dct:me=tesa:subme=10:ref=5:no-fast-pskip" \
       -crf 18 \
       -g 30 \
       -bf 3 \
       "$OUTPUT_DIR/test_4x4_blocks.mp4" -y

# 5. Mixed blocks (let encoder choose optimal)
echo "5. Generating mixed block video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx264 \
       -preset slow \
       -crf 22 \
       -g 30 \
       -bf 3 \
       "$OUTPUT_DIR/test_mixed_blocks.mp4" -y

# Generate HEVC videos with different CTU sizes
echo ""
echo "Generating HEVC test videos with different CTU sizes..."
echo ""

# 6. HEVC 64x64 CTU
echo "6. Generating HEVC 64x64 CTU video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx265 \
       -preset medium \
       -x265-params "ctu=64:min-cu-size=8" \
       -crf 23 \
       -g 30 \
       "$OUTPUT_DIR/test_hevc_64x64_ctu.mp4" -y

# 7. HEVC 32x32 CTU
echo "7. Generating HEVC 32x32 CTU video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx265 \
       -preset medium \
       -x265-params "ctu=32:min-cu-size=8" \
       -crf 23 \
       -g 30 \
       "$OUTPUT_DIR/test_hevc_32x32_ctu.mp4" -y

# 8. HEVC 16x16 CTU
echo "8. Generating HEVC 16x16 CTU video..."
ffmpeg -i "$OUTPUT_DIR/source.mp4" \
       -c:v libx265 \
       -preset medium \
       -x265-params "ctu=16:min-cu-size=8" \
       -crf 23 \
       -g 30 \
       "$OUTPUT_DIR/test_hevc_16x16_ctu.mp4" -y

echo ""
echo "Done! Test videos generated in $OUTPUT_DIR/"
echo ""
echo "Files created:"
ls -lh "$OUTPUT_DIR/"/*.mp4
echo ""
echo "You can now open these videos in VideoStudio to test block/partition visualization."
