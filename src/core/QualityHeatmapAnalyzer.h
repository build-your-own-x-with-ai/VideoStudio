#ifndef QUALITYHEATMAPANALYZER_H
#define QUALITYHEATMAPANALYZER_H

#include <QVector>
#include <QImage>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

class QualityHeatmapAnalyzer {
public:
    QualityHeatmapAnalyzer();

    // 按宏块计算 PSNR
    QVector<double> calculatePSNRPerMacroblock(AVFrame* ref, AVFrame* test, int blockSize = 16);

    // 按宏块计算 SSIM
    QVector<double> calculateSSIMPerMacroblock(AVFrame* ref, AVFrame* test, int blockSize = 16);

    // 生成 Temperature 热力图（像素差异）
    QImage generateTemperatureMap(AVFrame* ref, AVFrame* test);

    // 生成 Subtraction 热力图（有符号差值）
    QImage generateSubtractionMap(AVFrame* ref, AVFrame* test);

    // 获取宏块网格信息
    int getBlockRows() const { return blockRows; }
    int getBlockCols() const { return blockCols; }

private:
    double calculateBlockPSNR(const uint8_t* refData, const uint8_t* testData,
                              int width, int height, int refStride, int testStride);

    double calculateBlockSSIM(const uint8_t* refData, const uint8_t* testData,
                              int width, int height, int refStride, int testStride);

    int blockRows;
    int blockCols;
};

#endif // QUALITYHEATMAPANALYZER_H
