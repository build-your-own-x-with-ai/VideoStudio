#include "VideoAnalyzerThread.h"

VideoAnalyzerThread::VideoAnalyzerThread(const QString& filePath, QObject* parent)
    : QThread(parent), filePath(filePath), shouldStop(false) {
}

VideoAnalyzerThread::~VideoAnalyzerThread() {
    stop();
    wait();
}

void VideoAnalyzerThread::stop() {
    shouldStop = true;
}

void VideoAnalyzerThread::run() {
    VideoDecoder decoder;

    if (!decoder.open(filePath)) {
        emit analysisFailed("无法打开视频文件");
        return;
    }

    StreamInfo info = decoder.getStreamInfo();
    int estimatedFrames = info.numFrames > 0 ? info.numFrames : 1000;

    frames.clear();
    frames.reserve(estimatedFrames);

    FrameInfo frameInfo;
    int frameCount = 0;
    bool firstFrame = true;

    while (!shouldStop && decoder.readNextFrame(frameInfo)) {
        frames.append(frameInfo);
        frameCount++;

        if (firstFrame) {
            firstFrameImage = decoder.getCurrentFrameImage();
            firstFrame = false;
        }

        if (frameCount % 100 == 0) {
            emit progressUpdated(frameCount, estimatedFrames);
        }
    }

    decoder.close();

    if (!shouldStop) {
        emit progressUpdated(frameCount, frameCount);
        emit analysisComplete();
    }
}
