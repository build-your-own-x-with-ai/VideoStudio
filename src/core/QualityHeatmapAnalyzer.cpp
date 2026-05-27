#include "QualityHeatmapAnalyzer.h"
#include <cmath>
#include <algorithm>

QualityHeatmapAnalyzer::QualityHeatmapAnalyzer()
    : blockRows(0), blockCols(0) {
}

QVector<double> QualityHeatmapAnalyzer::calculatePSNRPerMacroblock(AVFrame* ref, AVFrame* test, int blockSize) {
    QVector<double> psnrValues;

    if (!ref || !test || ref->width != test->width || ref->height != test->height) {
        return psnrValues;
    }

    int width = ref->width;
    int height = ref->height;

    blockCols = (width + blockSize - 1) / blockSize;
    blockRows = (height + blockSize - 1) / blockSize;

    // 计算每个宏块的 PSNR（仅 Y 平面）
    const uint8_t* refData = ref->data[0];
    const uint8_t* testData = test->data[0];
    int refStride = ref->linesize[0];
    int testStride = test->linesize[0];

    for (int by = 0; by < blockRows; by++) {
        for (int bx = 0; bx < blockCols; bx++) {
            int x = bx * blockSize;
            int y = by * blockSize;
            int bw = std::min(blockSize, width - x);
            int bh = std::min(blockSize, height - y);

            const uint8_t* refBlock = refData + y * refStride + x;
            const uint8_t* testBlock = testData + y * testStride + x;

            double psnr = calculateBlockPSNR(refBlock, testBlock, bw, bh, refStride, testStride);
            psnrValues.append(psnr);
        }
    }

    return psnrValues;
}

QVector<double> QualityHeatmapAnalyzer::calculateSSIMPerMacroblock(AVFrame* ref, AVFrame* test, int blockSize) {
    QVector<double> ssimValues;

    if (!ref || !test || ref->width != test->width || ref->height != test->height) {
        return ssimValues;
    }

    int width = ref->width;
    int height = ref->height;

    blockCols = (width + blockSize - 1) / blockSize;
    blockRows = (height + blockSize - 1) / blockSize;

    // 计算每个宏块的 SSIM（仅 Y 平面）
    const uint8_t* refData = ref->data[0];
    const uint8_t* testData = test->data[0];
    int refStride = ref->linesize[0];
    int testStride = test->linesize[0];

    for (int by = 0; by < blockRows; by++) {
        for (int bx = 0; bx < blockCols; bx++) {
            int x = bx * blockSize;
            int y = by * blockSize;
            int bw = std::min(blockSize, width - x);
            int bh = std::min(blockSize, height - y);

            const uint8_t* refBlock = refData + y * refStride + x;
            const uint8_t* testBlock = testData + y * testStride + x;

            double ssim = calculateBlockSSIM(refBlock, testBlock, bw, bh, refStride, testStride);
            ssimValues.append(ssim);
        }
    }

    return ssimValues;
}

QImage QualityHeatmapAnalyzer::generateTemperatureMap(AVFrame* ref, AVFrame* test) {
    if (!ref || !test || ref->width != test->width || ref->height != test->height) {
        return QImage();
    }

    int width = ref->width;
    int height = ref->height;

    QImage heatmap(width, height, QImage::Format_ARGB32);

    const uint8_t* refData = ref->data[0];
    const uint8_t* testData = test->data[0];
    int refStride = ref->linesize[0];
    int testStride = test->linesize[0];

    // 计算每个像素的绝对差值并映射到颜色
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int refPixel = refData[y * refStride + x];
            int testPixel = testData[y * testStride + x];
            int diff = std::abs(refPixel - testPixel);

            // 映射到颜色：蓝色(0) -> 绿色 -> 黄色 -> 红色(255)
            int r, g, b;
            if (diff < 64) {
                // 蓝色到绿色
                r = 0;
                g = diff * 4;
                b = 255 - diff * 4;
            } else if (diff < 128) {
                // 绿色到黄色
                r = (diff - 64) * 4;
                g = 255;
                b = 0;
            } else {
                // 黄色到红色
                r = 255;
                g = 255 - (diff - 128) * 2;
                b = 0;
            }

            heatmap.setPixel(x, y, qRgba(r, g, b, 180));
        }
    }

    return heatmap;
}

QImage QualityHeatmapAnalyzer::generateSubtractionMap(AVFrame* ref, AVFrame* test) {
    if (!ref || !test || ref->width != test->width || ref->height != test->height) {
        return QImage();
    }

    int width = ref->width;
    int height = ref->height;

    QImage heatmap(width, height, QImage::Format_ARGB32);

    const uint8_t* refData = ref->data[0];
    const uint8_t* testData = test->data[0];
    int refStride = ref->linesize[0];
    int testStride = test->linesize[0];

    // 计算每个像素的有符号差值并映射到颜色
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int refPixel = refData[y * refStride + x];
            int testPixel = testData[y * testStride + x];
            int diff = testPixel - refPixel;  // 有符号差值

            // 映射到颜色：蓝色(-128) -> 灰色(0) -> 红色(+128)
            int r, g, b;
            if (diff < 0) {
                // 负差值：蓝色
                int intensity = std::min(255, -diff * 2);
                r = 0;
                g = 0;
                b = intensity;
            } else if (diff > 0) {
                // 正差值：红色
                int intensity = std::min(255, diff * 2);
                r = intensity;
                g = 0;
                b = 0;
            } else {
                // 零差值：灰色
                r = g = b = 128;
            }

            heatmap.setPixel(x, y, qRgba(r, g, b, 180));
        }
    }

    return heatmap;
}

double QualityHeatmapAnalyzer::calculateBlockPSNR(const uint8_t* refData, const uint8_t* testData,
                                                   int width, int height, int refStride, int testStride) {
    double mse = 0.0;
    int count = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int ref = refData[y * refStride + x];
            int test = testData[y * testStride + x];
            int diff = ref - test;
            mse += diff * diff;
            count++;
        }
    }

    if (count == 0) return 0.0;

    mse /= count;

    if (mse == 0.0) {
        return 100.0;  // 完美匹配
    }

    return 10.0 * log10(255.0 * 255.0 / mse);
}

double QualityHeatmapAnalyzer::calculateBlockSSIM(const uint8_t* refData, const uint8_t* testData,
                                                   int width, int height, int refStride, int testStride) {
    // 简化的 SSIM 计算
    double meanRef = 0.0, meanTest = 0.0;
    int count = 0;

    // 计算均值
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            meanRef += refData[y * refStride + x];
            meanTest += testData[y * testStride + x];
            count++;
        }
    }

    if (count == 0) return 0.0;

    meanRef /= count;
    meanTest /= count;

    // 计算方差和协方差
    double varRef = 0.0, varTest = 0.0, covar = 0.0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            double diffRef = refData[y * refStride + x] - meanRef;
            double diffTest = testData[y * testStride + x] - meanTest;
            varRef += diffRef * diffRef;
            varTest += diffTest * diffTest;
            covar += diffRef * diffTest;
        }
    }

    varRef /= count;
    varTest /= count;
    covar /= count;

    // SSIM 公式
    const double C1 = 6.5025;   // (0.01 * 255)^2
    const double C2 = 58.5225;  // (0.03 * 255)^2

    double numerator = (2.0 * meanRef * meanTest + C1) * (2.0 * covar + C2);
    double denominator = (meanRef * meanRef + meanTest * meanTest + C1) * (varRef + varTest + C2);

    if (denominator == 0.0) return 0.0;

    return numerator / denominator;
}
