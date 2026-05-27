#include "QualityAnalyzer.h"
#include <cmath>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

QualityAnalyzer::QualityAnalyzer() {
    clear();
}

QualityAnalyzer::~QualityAnalyzer() {
}

void QualityAnalyzer::clear() {
    metrics.clear();
    stats = QualityStats();
    errorMessage.clear();
    referenceVideoPath.clear();
    testVideoPath.clear();
}

bool QualityAnalyzer::setReferenceVideo(const QString& filePath) {
    referenceVideoPath = filePath;
    return true;
}

bool QualityAnalyzer::setTestVideo(const QString& filePath) {
    testVideoPath = filePath;
    return true;
}

bool QualityAnalyzer::analyze() {
    if (referenceVideoPath.isEmpty() || testVideoPath.isEmpty()) {
        errorMessage = "请先选择参考视频和测试视频";
        return false;
    }

    metrics.clear();

    // Open reference video
    AVFormatContext* refFormatCtx = nullptr;
    if (avformat_open_input(&refFormatCtx, referenceVideoPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        errorMessage = "无法打开参考视频";
        return false;
    }

    if (avformat_find_stream_info(refFormatCtx, nullptr) < 0) {
        errorMessage = "无法获取参考视频流信息";
        avformat_close_input(&refFormatCtx);
        return false;
    }

    int refVideoStreamIndex = -1;
    for (unsigned int i = 0; i < refFormatCtx->nb_streams; i++) {
        if (refFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            refVideoStreamIndex = i;
            break;
        }
    }

    if (refVideoStreamIndex == -1) {
        errorMessage = "参考视频中未找到视频流";
        avformat_close_input(&refFormatCtx);
        return false;
    }

    // Open reference codec
    AVCodecParameters* refCodecParams = refFormatCtx->streams[refVideoStreamIndex]->codecpar;
    const AVCodec* refCodec = avcodec_find_decoder(refCodecParams->codec_id);
    AVCodecContext* refCodecCtx = avcodec_alloc_context3(refCodec);
    avcodec_parameters_to_context(refCodecCtx, refCodecParams);
    avcodec_open2(refCodecCtx, refCodec, nullptr);

    // Open test video
    AVFormatContext* testFormatCtx = nullptr;
    if (avformat_open_input(&testFormatCtx, testVideoPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        errorMessage = "无法打开测试视频";
        avcodec_free_context(&refCodecCtx);
        avformat_close_input(&refFormatCtx);
        return false;
    }

    avformat_find_stream_info(testFormatCtx, nullptr);

    int testVideoStreamIndex = -1;
    for (unsigned int i = 0; i < testFormatCtx->nb_streams; i++) {
        if (testFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            testVideoStreamIndex = i;
            break;
        }
    }

    // Open test codec
    AVCodecParameters* testCodecParams = testFormatCtx->streams[testVideoStreamIndex]->codecpar;
    const AVCodec* testCodec = avcodec_find_decoder(testCodecParams->codec_id);
    AVCodecContext* testCodecCtx = avcodec_alloc_context3(testCodec);
    avcodec_parameters_to_context(testCodecCtx, testCodecParams);
    avcodec_open2(testCodecCtx, testCodec, nullptr);

    // Check dimensions match
    if (refCodecCtx->width != testCodecCtx->width || refCodecCtx->height != testCodecCtx->height) {
        errorMessage = "参考视频和测试视频的分辨率不匹配";
        avcodec_free_context(&refCodecCtx);
        avcodec_free_context(&testCodecCtx);
        avformat_close_input(&refFormatCtx);
        avformat_close_input(&testFormatCtx);
        return false;
    }

    AVFrame* refFrame = av_frame_alloc();
    AVFrame* testFrame = av_frame_alloc();
    AVPacket* refPacket = av_packet_alloc();
    AVPacket* testPacket = av_packet_alloc();

    int frameNumber = 0;
    double sumPSNR = 0.0;
    double sumSSIM = 0.0;
    double minPSNR = 100.0;
    double maxPSNR = 0.0;
    double minSSIM = 1.0;
    double maxSSIM = 0.0;

    // Read frames from both videos
    bool refHasFrame = true;
    bool testHasFrame = true;

    while (refHasFrame && testHasFrame) {
        // Read reference frame
        refHasFrame = false;
        while (av_read_frame(refFormatCtx, refPacket) >= 0) {
            if (refPacket->stream_index == refVideoStreamIndex) {
                avcodec_send_packet(refCodecCtx, refPacket);
                if (avcodec_receive_frame(refCodecCtx, refFrame) == 0) {
                    refHasFrame = true;
                    av_packet_unref(refPacket);
                    break;
                }
            }
            av_packet_unref(refPacket);
        }

        // Read test frame
        testHasFrame = false;
        while (av_read_frame(testFormatCtx, testPacket) >= 0) {
            if (testPacket->stream_index == testVideoStreamIndex) {
                avcodec_send_packet(testCodecCtx, testPacket);
                if (avcodec_receive_frame(testCodecCtx, testFrame) == 0) {
                    testHasFrame = true;
                    av_packet_unref(testPacket);
                    break;
                }
            }
            av_packet_unref(testPacket);
        }

        if (!refHasFrame || !testHasFrame) {
            break;
        }

        // Calculate quality metrics
        QualityMetrics qm;
        qm.frameNumber = frameNumber;
        qm.timestamp = frameNumber / (refFormatCtx->streams[refVideoStreamIndex]->avg_frame_rate.num /
                                      static_cast<double>(refFormatCtx->streams[refVideoStreamIndex]->avg_frame_rate.den));

        qm.psnrY = calculatePSNR(refFrame, testFrame, 0);
        qm.psnrU = calculatePSNR(refFrame, testFrame, 1);
        qm.psnrV = calculatePSNR(refFrame, testFrame, 2);
        qm.psnr = (6.0 * qm.psnrY + qm.psnrU + qm.psnrV) / 8.0;  // Weighted average

        qm.ssim = calculateSSIM(refFrame, testFrame);

        metrics.append(qm);

        sumPSNR += qm.psnr;
        sumSSIM += qm.ssim;
        minPSNR = std::min(minPSNR, qm.psnr);
        maxPSNR = std::max(maxPSNR, qm.psnr);
        minSSIM = std::min(minSSIM, qm.ssim);
        maxSSIM = std::max(maxSSIM, qm.ssim);

        frameNumber++;
    }

    // Calculate statistics
    stats.totalFrames = frameNumber;
    stats.avgPSNR = frameNumber > 0 ? sumPSNR / frameNumber : 0.0;
    stats.avgSSIM = frameNumber > 0 ? sumSSIM / frameNumber : 0.0;
    stats.minPSNR = minPSNR;
    stats.maxPSNR = maxPSNR;
    stats.minSSIM = minSSIM;
    stats.maxSSIM = maxSSIM;

    // Cleanup
    av_frame_free(&refFrame);
    av_frame_free(&testFrame);
    av_packet_free(&refPacket);
    av_packet_free(&testPacket);
    avcodec_free_context(&refCodecCtx);
    avcodec_free_context(&testCodecCtx);
    avformat_close_input(&refFormatCtx);
    avformat_close_input(&testFormatCtx);

    return true;
}

double QualityAnalyzer::calculatePSNR(AVFrame* ref, AVFrame* test, int component) {
    if (!ref || !test) return 0.0;

    int width = ref->width;
    int height = ref->height;

    if (component == 1 || component == 2) {
        // U or V component (chroma)
        width /= 2;
        height /= 2;
    }

    const uint8_t* refData = ref->data[component];
    const uint8_t* testData = test->data[component];
    int refStride = ref->linesize[component];
    int testStride = test->linesize[component];

    double mse = 0.0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int diff = refData[y * refStride + x] - testData[y * testStride + x];
            mse += diff * diff;
        }
    }

    mse /= (width * height);

    if (mse == 0.0) {
        return 100.0;  // Perfect match
    }

    return 10.0 * std::log10(255.0 * 255.0 / mse);
}

double QualityAnalyzer::calculateSSIM(AVFrame* ref, AVFrame* test) {
    if (!ref || !test) return 0.0;

    // Calculate SSIM for Y plane only (luminance)
    return calculateSSIMPlane(ref->data[0], test->data[0],
                              ref->width, ref->height, ref->linesize[0]);
}

double QualityAnalyzer::calculateSSIMPlane(const uint8_t* ref, const uint8_t* test,
                                           int width, int height, int stride) {
    const double C1 = 6.5025;   // (0.01 * 255)^2
    const double C2 = 58.5225;  // (0.03 * 255)^2

    double ssimSum = 0.0;
    int count = 0;

    // Use 8x8 blocks for SSIM calculation
    for (int y = 0; y < height - 7; y += 4) {
        for (int x = 0; x < width - 7; x += 4) {
            double meanRef = 0.0;
            double meanTest = 0.0;
            double varRef = 0.0;
            double varTest = 0.0;
            double covar = 0.0;

            // Calculate means
            for (int by = 0; by < 8; by++) {
                for (int bx = 0; bx < 8; bx++) {
                    int refVal = ref[(y + by) * stride + (x + bx)];
                    int testVal = test[(y + by) * stride + (x + bx)];
                    meanRef += refVal;
                    meanTest += testVal;
                }
            }
            meanRef /= 64.0;
            meanTest /= 64.0;

            // Calculate variances and covariance
            for (int by = 0; by < 8; by++) {
                for (int bx = 0; bx < 8; bx++) {
                    double refVal = ref[(y + by) * stride + (x + bx)] - meanRef;
                    double testVal = test[(y + by) * stride + (x + bx)] - meanTest;
                    varRef += refVal * refVal;
                    varTest += testVal * testVal;
                    covar += refVal * testVal;
                }
            }
            varRef /= 63.0;
            varTest /= 63.0;
            covar /= 63.0;

            // Calculate SSIM for this block
            double numerator = (2.0 * meanRef * meanTest + C1) * (2.0 * covar + C2);
            double denominator = (meanRef * meanRef + meanTest * meanTest + C1) *
                                 (varRef + varTest + C2);

            ssimSum += numerator / denominator;
            count++;
        }
    }

    return count > 0 ? ssimSum / count : 0.0;
}
