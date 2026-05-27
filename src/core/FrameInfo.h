#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include <cstdint>

struct FrameInfo {
    int64_t frameNumber;
    char frameType;
    int64_t pts;
    int64_t dts;
    int size;
    double timestamp;
    bool isKeyFrame;
    int qp;

    FrameInfo()
        : frameNumber(0), frameType('?'), pts(0), dts(0),
          size(0), timestamp(0.0), isKeyFrame(false), qp(-1) {}
};

#endif // FRAMEINFO_H
