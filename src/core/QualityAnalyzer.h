#ifndef QUALITYANALYZER_H
#define QUALITYANALYZER_H

#include <QVector>
#include <QString>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
}

struct QualityMetrics {
    int frameNumber;
    double timestamp;
    double psnr;          // Peak Signal-to-Noise Ratio (dB)
    double psnrY;         // PSNR for Y channel
    double psnrU;         // PSNR for U channel
    double psnrV;         // PSNR for V channel
    double ssim;          // Structural Similarity Index
};

struct QualityStats {
    double avgPSNR;
    double minPSNR;
    double maxPSNR;
    double avgSSIM;
    double minSSIM;
    double maxSSIM;
    int totalFrames;
};

class QualityAnalyzer {
public:
    QualityAnalyzer();
    ~QualityAnalyzer();

    bool setReferenceVideo(const QString& filePath);
    bool setTestVideo(const QString& filePath);
    bool analyze();
    void clear();

    QVector<QualityMetrics> getMetrics() const { return metrics; }
    QualityStats getStats() const { return stats; }
    QString getErrorMessage() const { return errorMessage; }

private:
    double calculatePSNR(AVFrame* ref, AVFrame* test, int component);
    double calculateSSIM(AVFrame* ref, AVFrame* test);
    double calculateSSIMPlane(const uint8_t* ref, const uint8_t* test,
                              int width, int height, int stride);

    QString referenceVideoPath;
    QString testVideoPath;
    QVector<QualityMetrics> metrics;
    QualityStats stats;
    QString errorMessage;
};

#endif // QUALITYANALYZER_H
