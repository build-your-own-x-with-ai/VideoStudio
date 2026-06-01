#ifndef YUVREADER_H
#define YUVREADER_H

#include <QObject>
#include <QFile>
#include <QString>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

namespace VideoStudio {

enum class YUVPixelFormat {
    I420,      // YUV 4:2:0 planar (most common)
    I422,      // YUV 4:2:2 planar
    I444,      // YUV 4:4:4 planar
    NV12,      // YUV 4:2:0 semi-planar (Y + interleaved UV)
    NV21,      // YUV 4:2:0 semi-planar (Y + interleaved VU)
    YV12,      // YUV 4:2:0 planar (Y, V, U order)
    UYVY,      // YUV 4:2:2 packed
    YUY2,      // YUV 4:2:2 packed (YUYV)
    RGB24,     // RGB 24-bit
    RGB32,     // RGBA 32-bit
    GRAY       // Grayscale (Y only)
};

struct YUVFileInfo {
    int width;
    int height;
    YUVPixelFormat format;
    int frameCount;
    int64_t fileSize;
    int bytesPerFrame;
};

class YUVReader : public QObject {
    Q_OBJECT

public:
    explicit YUVReader(QObject* parent = nullptr);
    ~YUVReader();

    // Open raw YUV file with specified parameters
    bool openFile(const QString& filePath, int width, int height, YUVPixelFormat format);
    void close();
    bool isOpen() const { return m_file.isOpen(); }

    // Frame access
    AVFrame* readFrame(int frameNumber);
    bool seekToFrame(int frameNumber);

    // Getters
    int getWidth() const { return m_info.width; }
    int getHeight() const { return m_info.height; }
    int getFrameCount() const { return m_info.frameCount; }
    YUVPixelFormat getFormat() const { return m_info.format; }
    QString getFileName() const { return m_filePath; }
    const YUVFileInfo& getFileInfo() const { return m_info; }

    // Format utilities
    static int calculateBytesPerFrame(int width, int height, YUVPixelFormat format);
    static AVPixelFormat toAVPixelFormat(YUVPixelFormat format);
    static QString formatToString(YUVPixelFormat format);

signals:
    void error(const QString& message);

private:
    bool validateFileSize();
    int64_t calculateFrameOffset(int frameNumber);
    bool readFrameData(int frameNumber, AVFrame* frame);

    QFile m_file;
    QString m_filePath;
    YUVFileInfo m_info;
    AVFrame* m_frame;
    int m_currentFrameNumber;
};

} // namespace VideoStudio

#endif // YUVREADER_H
