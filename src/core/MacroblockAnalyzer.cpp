#include "MacroblockAnalyzer.h"

MacroblockAnalyzer::MacroblockAnalyzer() {
}

void MacroblockAnalyzer::clear() {
    macroblocks.clear();
}

QVector<MacroblockInfo> MacroblockAnalyzer::extractMacroblocks(AVFrame* frame) {
    QVector<MacroblockInfo> result;

    if (!frame) {
        return result;
    }

    // Extract motion vectors from side data
    AVFrameSideData* sd = av_frame_get_side_data(frame, AV_FRAME_DATA_MOTION_VECTORS);

    if (sd) {
        const AVMotionVector* mvs = (const AVMotionVector*)sd->data;
        int mv_count = sd->size / sizeof(AVMotionVector);

        for (int i = 0; i < mv_count; i++) {
            const AVMotionVector* mv = &mvs[i];

            MacroblockInfo mb;
            mb.x = mv->src_x;
            mb.y = mv->src_y;
            mb.width = mv->w;
            mb.height = mv->h;

            // Determine type based on motion vector
            if (mv->source < 0) {
                mb.type = 'I';  // Intra
            } else if (mv->motion_x == 0 && mv->motion_y == 0) {
                mb.type = 'S';  // Skip
            } else {
                mb.type = 'P';  // Inter
            }

            mb.mvX = mv->motion_x;
            mb.mvY = mv->motion_y;
            mb.source = mv->source;
            mb.qp = -1;  // QP not available from motion vector data

            result.append(mb);
        }
    } else {
        // No motion vector data available
        // Create a grid based on codec type
        // For HEVC, use 32x32 CTU; for H.264, use 16x16 macroblock
        int mbSize = 32;  // Default to HEVC CTU size
        int width = frame->width;
        int height = frame->height;

        for (int y = 0; y < height; y += mbSize) {
            for (int x = 0; x < width; x += mbSize) {
                MacroblockInfo mb;
                mb.x = x;
                mb.y = y;
                mb.width = mbSize;
                mb.height = mbSize;
                mb.type = '?';  // Unknown
                mb.mvX = 0;
                mb.mvY = 0;
                mb.source = -1;
                mb.qp = -1;

                result.append(mb);
            }
        }
    }

    macroblocks = result;
    return result;
}
