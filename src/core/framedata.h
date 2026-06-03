#ifndef FRAMEDATA_H
#define FRAMEDATA_H

#include <QString>
#include <QVector>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace VideoStudio {

// Block/Macroblock information for visualization
struct BlockInfo {
    int x;                   // Block position X
    int y;                   // Block position Y
    int width;               // Block width
    int height;              // Block height
    int qp;                  // Quantization parameter
    bool isIntra;            // Intra/Inter prediction
    int predMode;            // Prediction mode
    int motionX;             // Motion vector X (quarter-pixel)
    int motionY;             // Motion vector Y (quarter-pixel)

    BlockInfo()
        : x(0), y(0), width(0), height(0), qp(0),
          isIntra(false), predMode(0), motionX(0), motionY(0) {}
};

struct FrameInfo {
    int frameNumber;
    int64_t pts;
    int64_t dts;
    int64_t offset;          // Byte offset in file
    AVPictureType frameType; // I, P, B, etc.
    int size;                // Frame size in bytes
    int qp;                  // Average QP value
    bool isKeyFrame;
    double bitrate;          // Instantaneous bitrate (bps)
    double timestamp;        // Timestamp in seconds

    // Block-level information (optional, for detailed analysis)
    QVector<BlockInfo> blocks;

    FrameInfo()
        : frameNumber(0), pts(0), dts(0), offset(0),
          frameType(AV_PICTURE_TYPE_NONE), size(0), qp(0),
          isKeyFrame(false), bitrate(0.0), timestamp(0.0) {}
};

class FrameIndex {
public:
    FrameIndex();
    ~FrameIndex();

    void addFrame(const FrameInfo& frame);
    void clear();

    int frameCount() const { return m_frames.size(); }
    const FrameInfo* getFrame(int index) const;

    // Statistics
    int getIFrameCount() const;
    int getPFrameCount() const;
    int getBFrameCount() const;
    double getAverageBitrate() const;
    int getMaxFrameSize() const;
    int getMinFrameSize() const;

private:
    QVector<FrameInfo> m_frames;
};

} // namespace VideoStudio

#endif // FRAMEDATA_H
