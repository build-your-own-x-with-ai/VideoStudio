#!/bin/bash

# Quick test video generator for block size visualization

OUTPUT_DIR="test_videos"
mkdir -p "$OUTPUT_DIR"

echo "Generating simple test videos..."

# 1. H.264 with 16x16 blocks (no sub-partitions)
echo "1. H.264 16x16 blocks..."
ffmpeg -f lavfi -i "color=c=blue:s=640x360:d=3,format=yuv420p" \
       -f lavfi -i "color=c=red:s=640x360:d=3,format=yuv420p" \
       -filter_complex "[0:v][1:v]concat=n=2:v=1[v]" \
       -map "[v]" \
       -c:v libx264 -preset ultrafast \
       -x264-params "partitions=none:me=dia:subme=0" \
       -g 10 -bf 0 -crf 28 \
       "$OUTPUT_DIR/h264_16x16.mp4" -y

# 2. H.264 with 8x8 blocks
echo "2. H.264 8x8 blocks..."
ffmpeg -f lavfi -i "testsrc=duration=3:size=640x360:rate=30" \
       -c:v libx264 -preset medium \
       -x264-params "partitions=all:8x8dct:me=umh:subme=7" \
       -g 30 -bf 2 -crf 22 \
       "$OUTPUT_DIR/h264_8x8.mp4" -y

# 3. HEVC with 32x32 CTU
echo "3. HEVC 32x32 CTU..."
ffmpeg -f lavfi -i "testsrc=duration=3:size=640x360:rate=30" \
       -c:v libx265 -preset fast \
       -x265-params "ctu=32:min-cu-size=16" \
       -g 30 -crf 25 \
       "$OUTPUT_DIR/hevc_32x32.mp4" -y

echo ""
echo "Done! Test videos in $OUTPUT_DIR/"
ls -lh "$OUTPUT_DIR/"
