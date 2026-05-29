#include "core/framedata.h"
#include <algorithm>

namespace VideoStudio {

FrameIndex::FrameIndex() {
}

FrameIndex::~FrameIndex() {
}

void FrameIndex::addFrame(const FrameInfo& frame) {
    m_frames.append(frame);
}

void FrameIndex::clear() {
    m_frames.clear();
}

const FrameInfo* FrameIndex::getFrame(int index) const {
    if (index >= 0 && index < m_frames.size()) {
        return &m_frames[index];
    }
    return nullptr;
}

int FrameIndex::getIFrameCount() const {
    return std::count_if(m_frames.begin(), m_frames.end(),
        [](const FrameInfo& f) { return f.frameType == AV_PICTURE_TYPE_I; });
}

int FrameIndex::getPFrameCount() const {
    return std::count_if(m_frames.begin(), m_frames.end(),
        [](const FrameInfo& f) { return f.frameType == AV_PICTURE_TYPE_P; });
}

int FrameIndex::getBFrameCount() const {
    return std::count_if(m_frames.begin(), m_frames.end(),
        [](const FrameInfo& f) { return f.frameType == AV_PICTURE_TYPE_B; });
}

double FrameIndex::getAverageBitrate() const {
    if (m_frames.isEmpty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& frame : m_frames) {
        sum += frame.bitrate;
    }
    return sum / m_frames.size();
}

int FrameIndex::getMaxFrameSize() const {
    if (m_frames.isEmpty()) {
        return 0;
    }

    auto it = std::max_element(m_frames.begin(), m_frames.end(),
        [](const FrameInfo& a, const FrameInfo& b) { return a.size < b.size; });
    return it->size;
}

int FrameIndex::getMinFrameSize() const {
    if (m_frames.isEmpty()) {
        return 0;
    }

    auto it = std::min_element(m_frames.begin(), m_frames.end(),
        [](const FrameInfo& a, const FrameInfo& b) { return a.size < b.size; });
    return it->size;
}

} // namespace VideoStudio
