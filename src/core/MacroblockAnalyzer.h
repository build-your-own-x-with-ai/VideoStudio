#ifndef MACROBLOCKANALYZER_H
#define MACROBLOCKANALYZER_H

#include <QVector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/motion_vector.h>
}

struct MacroblockInfo {
    int x, y;                     // 位置（像素）
    int width, height;            // 大小（像素）
    char type;                    // 'I', 'P', 'S' (skip)
    int qp;                       // 量化参数
    int mvX, mvY;                 // 运动矢量
    int source;                   // 参考帧索引

    // 扩展参数
    bool hasExtendedParams;       // 是否有扩展参数
    bool saoEnabled;              // SAO 启用标志（HEVC）
    int saoType;                  // SAO 类型
    bool merged;                  // Merged 标志（HEVC）

    MacroblockInfo() : x(0), y(0), width(0), height(0), type('?'), qp(-1),
                       mvX(0), mvY(0), source(-1), hasExtendedParams(false),
                       saoEnabled(false), saoType(0), merged(false) {}
};

class MacroblockAnalyzer {
public:
    MacroblockAnalyzer();

    QVector<MacroblockInfo> extractMacroblocks(AVFrame* frame);
    void clear();

private:
    QVector<MacroblockInfo> macroblocks;
};

#endif // MACROBLOCKANALYZER_H
