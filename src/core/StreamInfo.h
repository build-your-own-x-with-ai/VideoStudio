#ifndef STREAMINFO_H
#define STREAMINFO_H

#include <cstdint>
#include <QString>

struct StreamInfo {
    QString codecName;
    QString codecLongName;
    QString pixelFormat;
    int width;
    int height;
    double frameRate;
    int64_t bitrate;
    int64_t duration;
    int64_t numFrames;
    QString containerFormat;

    StreamInfo()
        : width(0), height(0), frameRate(0.0),
          bitrate(0), duration(0), numFrames(0) {}
};

#endif // STREAMINFO_H
