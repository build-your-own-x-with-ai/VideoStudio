#include "yuvreader.h"
#include <QDebug>

extern "C" {
#include <libavutil/imgutils.h>
}

namespace VideoStudio {

YUVReader::YUVReader(QObject* parent)
    : QObject(parent)
    , m_frame(nullptr)
    , m_currentFrameNumber(-1)
{
    m_info.width = 0;
    m_info.height = 0;
    m_info.format = YUVPixelFormat::I420;
    m_info.frameCount = 0;
    m_info.fileSize = 0;
    m_info.bytesPerFrame = 0;
}

YUVReader::~YUVReader() {
    close();
}

bool YUVReader::openFile(const QString& filePath, int width, int height, YUVPixelFormat format) {
    close();

    if (width <= 0 || height <= 0) {
        emit error(tr("Width and height must be positive values."));
        return false;
    }

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        emit error(tr("Cannot open file: %1").arg(filePath));
        return false;
    }

    m_filePath = filePath;
    m_info.width = width;
    m_info.height = height;
    m_info.format = format;
    m_info.fileSize = m_file.size();
    m_info.bytesPerFrame = calculateBytesPerFrame(width, height, format);

    if (!validateFileSize()) {
        close();
        return false;
    }

    m_info.frameCount = m_info.fileSize / m_info.bytesPerFrame;

    // Allocate frame buffer
    m_frame = av_frame_alloc();
    if (!m_frame) {
        emit error(tr("Failed to allocate frame buffer."));
        close();
        return false;
    }

    m_frame->format = toAVPixelFormat(format);
    m_frame->width = width;
    m_frame->height = height;

    if (av_frame_get_buffer(m_frame, 0) < 0) {
        emit error(tr("Failed to allocate frame buffer for %1x%2 resolution.").arg(width).arg(height));
        close();
        return false;
    }

    qDebug() << "YUVReader: Opened" << filePath;
    qDebug() << "  Resolution:" << width << "x" << height;
    qDebug() << "  Format:" << formatToString(format);
    qDebug() << "  File size:" << m_info.fileSize << "bytes";
    qDebug() << "  Bytes per frame:" << m_info.bytesPerFrame;
    qDebug() << "  Frame count:" << m_info.frameCount;

    return true;
}

void YUVReader::close() {
    if (m_file.isOpen()) {
        m_file.close();
    }

    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    m_currentFrameNumber = -1;
}

bool YUVReader::validateFileSize() {
    if (m_info.bytesPerFrame <= 0) {
        emit error(tr("Invalid bytes per frame calculation."));
        return false;
    }

    if (m_info.fileSize % m_info.bytesPerFrame != 0) {
        double expectedFrames = static_cast<double>(m_info.fileSize) / m_info.bytesPerFrame;
        emit error(tr("File size (%1 bytes) is not a multiple of frame size (%2 bytes).\n"
                      "Expected frame count: %3 frames\n"
                      "Please verify width, height, and format parameters.")
                   .arg(m_info.fileSize)
                   .arg(m_info.bytesPerFrame)
                   .arg(expectedFrames, 0, 'f', 3));
        return false;
    }

    return true;
}

int64_t YUVReader::calculateFrameOffset(int frameNumber) {
    return static_cast<int64_t>(frameNumber) * m_info.bytesPerFrame;
}

AVFrame* YUVReader::readFrame(int frameNumber) {
    if (!isOpen()) {
        return nullptr;
    }

    if (frameNumber < 0 || frameNumber >= m_info.frameCount) {
        qWarning() << "YUVReader: Frame number" << frameNumber << "out of range [0," << m_info.frameCount - 1 << "]";
        return nullptr;
    }

    if (!readFrameData(frameNumber, m_frame)) {
        return nullptr;
    }

    m_currentFrameNumber = frameNumber;
    return m_frame;
}

bool YUVReader::seekToFrame(int frameNumber) {
    if (!isOpen()) {
        return false;
    }

    if (frameNumber < 0 || frameNumber >= m_info.frameCount) {
        return false;
    }

    int64_t offset = calculateFrameOffset(frameNumber);
    return m_file.seek(offset);
}

bool YUVReader::readFrameData(int frameNumber, AVFrame* frame) {
    int64_t offset = calculateFrameOffset(frameNumber);
    if (!m_file.seek(offset)) {
        emit error(tr("Failed to seek to frame %1 (offset %2).").arg(frameNumber).arg(offset));
        return false;
    }

    // Read frame data based on format
    switch (m_info.format) {
        case YUVPixelFormat::I420:
        case YUVPixelFormat::YV12: {
            // Planar 4:2:0: Y plane + U plane + V plane
            int ySize = m_info.width * m_info.height;
            int uvSize = (m_info.width / 2) * (m_info.height / 2);

            // Read Y plane
            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), ySize) != ySize) {
                emit error(tr("Failed to read Y plane for frame %1.").arg(frameNumber));
                return false;
            }

            if (m_info.format == YUVPixelFormat::I420) {
                // I420: U then V
                if (m_file.read(reinterpret_cast<char*>(frame->data[1]), uvSize) != uvSize) {
                    emit error(tr("Failed to read U plane for frame %1.").arg(frameNumber));
                    return false;
                }
                if (m_file.read(reinterpret_cast<char*>(frame->data[2]), uvSize) != uvSize) {
                    emit error(tr("Failed to read V plane for frame %1.").arg(frameNumber));
                    return false;
                }
            } else {
                // YV12: V then U
                if (m_file.read(reinterpret_cast<char*>(frame->data[2]), uvSize) != uvSize) {
                    emit error(tr("Failed to read V plane for frame %1.").arg(frameNumber));
                    return false;
                }
                if (m_file.read(reinterpret_cast<char*>(frame->data[1]), uvSize) != uvSize) {
                    emit error(tr("Failed to read U plane for frame %1.").arg(frameNumber));
                    return false;
                }
            }
            break;
        }

        case YUVPixelFormat::I422: {
            // Planar 4:2:2: Y plane + U plane + V plane
            int ySize = m_info.width * m_info.height;
            int uvSize = (m_info.width / 2) * m_info.height;

            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), ySize) != ySize) {
                emit error(tr("Failed to read Y plane for frame %1.").arg(frameNumber));
                return false;
            }
            if (m_file.read(reinterpret_cast<char*>(frame->data[1]), uvSize) != uvSize) {
                emit error(tr("Failed to read U plane for frame %1.").arg(frameNumber));
                return false;
            }
            if (m_file.read(reinterpret_cast<char*>(frame->data[2]), uvSize) != uvSize) {
                emit error(tr("Failed to read V plane for frame %1.").arg(frameNumber));
                return false;
            }
            break;
        }

        case YUVPixelFormat::I444: {
            // Planar 4:4:4: Y plane + U plane + V plane
            int planeSize = m_info.width * m_info.height;

            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), planeSize) != planeSize) {
                emit error(tr("Failed to read Y plane for frame %1.").arg(frameNumber));
                return false;
            }
            if (m_file.read(reinterpret_cast<char*>(frame->data[1]), planeSize) != planeSize) {
                emit error(tr("Failed to read U plane for frame %1.").arg(frameNumber));
                return false;
            }
            if (m_file.read(reinterpret_cast<char*>(frame->data[2]), planeSize) != planeSize) {
                emit error(tr("Failed to read V plane for frame %1.").arg(frameNumber));
                return false;
            }
            break;
        }

        case YUVPixelFormat::NV12:
        case YUVPixelFormat::NV21: {
            // Semi-planar 4:2:0: Y plane + interleaved UV plane
            int ySize = m_info.width * m_info.height;
            int uvSize = m_info.width * (m_info.height / 2);

            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), ySize) != ySize) {
                emit error(tr("Failed to read Y plane for frame %1.").arg(frameNumber));
                return false;
            }
            if (m_file.read(reinterpret_cast<char*>(frame->data[1]), uvSize) != uvSize) {
                emit error(tr("Failed to read UV plane for frame %1.").arg(frameNumber));
                return false;
            }
            break;
        }

        case YUVPixelFormat::UYVY:
        case YUVPixelFormat::YUY2: {
            // Packed 4:2:2: 2 bytes per pixel
            int dataSize = m_info.width * m_info.height * 2;

            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), dataSize) != dataSize) {
                emit error(tr("Failed to read packed YUV data for frame %1.").arg(frameNumber));
                return false;
            }
            break;
        }

        case YUVPixelFormat::RGB24: {
            // RGB 24-bit: 3 bytes per pixel
            int dataSize = m_info.width * m_info.height * 3;

            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), dataSize) != dataSize) {
                emit error(tr("Failed to read RGB24 data for frame %1.").arg(frameNumber));
                return false;
            }
            break;
        }

        case YUVPixelFormat::RGB32: {
            // RGBA 32-bit: 4 bytes per pixel
            int dataSize = m_info.width * m_info.height * 4;

            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), dataSize) != dataSize) {
                emit error(tr("Failed to read RGB32 data for frame %1.").arg(frameNumber));
                return false;
            }
            break;
        }

        case YUVPixelFormat::GRAY: {
            // Grayscale: 1 byte per pixel
            int dataSize = m_info.width * m_info.height;

            if (m_file.read(reinterpret_cast<char*>(frame->data[0]), dataSize) != dataSize) {
                emit error(tr("Failed to read grayscale data for frame %1.").arg(frameNumber));
                return false;
            }
            break;
        }

        default:
            emit error(tr("Unsupported YUV format."));
            return false;
    }

    return true;
}

int YUVReader::calculateBytesPerFrame(int width, int height, YUVPixelFormat format) {
    switch (format) {
        case YUVPixelFormat::I420:
        case YUVPixelFormat::NV12:
        case YUVPixelFormat::NV21:
        case YUVPixelFormat::YV12:
            // 4:2:0 formats: 1.5 bytes per pixel
            return width * height * 3 / 2;

        case YUVPixelFormat::I422:
        case YUVPixelFormat::UYVY:
        case YUVPixelFormat::YUY2:
            // 4:2:2 formats: 2 bytes per pixel
            return width * height * 2;

        case YUVPixelFormat::I444:
        case YUVPixelFormat::RGB24:
            // 4:4:4 and RGB24: 3 bytes per pixel
            return width * height * 3;

        case YUVPixelFormat::RGB32:
            // RGBA: 4 bytes per pixel
            return width * height * 4;

        case YUVPixelFormat::GRAY:
            // Grayscale: 1 byte per pixel
            return width * height;

        default:
            return 0;
    }
}

AVPixelFormat YUVReader::toAVPixelFormat(YUVPixelFormat format) {
    switch (format) {
        case YUVPixelFormat::I420:
            return AV_PIX_FMT_YUV420P;
        case YUVPixelFormat::I422:
            return AV_PIX_FMT_YUV422P;
        case YUVPixelFormat::I444:
            return AV_PIX_FMT_YUV444P;
        case YUVPixelFormat::NV12:
            return AV_PIX_FMT_NV12;
        case YUVPixelFormat::NV21:
            return AV_PIX_FMT_NV21;
        case YUVPixelFormat::YV12:
            return AV_PIX_FMT_YUV420P;  // YV12 is similar to I420
        case YUVPixelFormat::UYVY:
            return AV_PIX_FMT_UYVY422;
        case YUVPixelFormat::YUY2:
            return AV_PIX_FMT_YUYV422;
        case YUVPixelFormat::RGB24:
            return AV_PIX_FMT_RGB24;
        case YUVPixelFormat::RGB32:
            return AV_PIX_FMT_RGBA;
        case YUVPixelFormat::GRAY:
            return AV_PIX_FMT_GRAY8;
        default:
            return AV_PIX_FMT_NONE;
    }
}

QString YUVReader::formatToString(YUVPixelFormat format) {
    switch (format) {
        case YUVPixelFormat::I420:
            return "I420 (4:2:0 Planar)";
        case YUVPixelFormat::I422:
            return "I422 (4:2:2 Planar)";
        case YUVPixelFormat::I444:
            return "I444 (4:4:4 Planar)";
        case YUVPixelFormat::NV12:
            return "NV12 (4:2:0 Semi-Planar)";
        case YUVPixelFormat::NV21:
            return "NV21 (4:2:0 Semi-Planar)";
        case YUVPixelFormat::YV12:
            return "YV12 (4:2:0 Planar)";
        case YUVPixelFormat::UYVY:
            return "UYVY (4:2:2 Packed)";
        case YUVPixelFormat::YUY2:
            return "YUY2 (4:2:2 Packed)";
        case YUVPixelFormat::RGB24:
            return "RGB24";
        case YUVPixelFormat::RGB32:
            return "RGB32 (RGBA)";
        case YUVPixelFormat::GRAY:
            return "Grayscale";
        default:
            return "Unknown";
    }
}

} // namespace VideoStudio
