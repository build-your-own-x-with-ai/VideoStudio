#ifndef NALDATA_H
#define NALDATA_H

#include <QString>
#include <QByteArray>
#include <cstdint>

namespace VideoStudio {

// H.264 NAL Unit Types
enum H264NALUnitType {
    H264_NAL_UNSPECIFIED = 0,
    H264_NAL_SLICE = 1,
    H264_NAL_DPA = 2,
    H264_NAL_DPB = 3,
    H264_NAL_DPC = 4,
    H264_NAL_IDR_SLICE = 5,
    H264_NAL_SEI = 6,
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8,
    H264_NAL_AUD = 9,
    H264_NAL_END_SEQUENCE = 10,
    H264_NAL_END_STREAM = 11,
    H264_NAL_FILLER_DATA = 12,
    H264_NAL_SPS_EXT = 13,
    H264_NAL_PREFIX = 14,
    H264_NAL_SUB_SPS = 15,
    H264_NAL_AUXILIARY_SLICE = 19,
    H264_NAL_SLICE_EXT = 20
};

// H.265 NAL Unit Types
enum H265NALUnitType {
    HEVC_NAL_TRAIL_N = 0,
    HEVC_NAL_TRAIL_R = 1,
    HEVC_NAL_TSA_N = 2,
    HEVC_NAL_TSA_R = 3,
    HEVC_NAL_STSA_N = 4,
    HEVC_NAL_STSA_R = 5,
    HEVC_NAL_RADL_N = 6,
    HEVC_NAL_RADL_R = 7,
    HEVC_NAL_RASL_N = 8,
    HEVC_NAL_RASL_R = 9,
    HEVC_NAL_BLA_W_LP = 16,
    HEVC_NAL_BLA_W_RADL = 17,
    HEVC_NAL_BLA_N_LP = 18,
    HEVC_NAL_IDR_W_RADL = 19,
    HEVC_NAL_IDR_N_LP = 20,
    HEVC_NAL_CRA_NUT = 21,
    HEVC_NAL_VPS = 32,
    HEVC_NAL_SPS = 33,
    HEVC_NAL_PPS = 34,
    HEVC_NAL_AUD = 35,
    HEVC_NAL_EOS_NUT = 36,
    HEVC_NAL_EOB_NUT = 37,
    HEVC_NAL_FD_NUT = 38,
    HEVC_NAL_SEI_PREFIX = 39,
    HEVC_NAL_SEI_SUFFIX = 40
};

struct NALUnitInfo {
    int index;                  // Sequential index across all NAL units
    int64_t fileOffset;         // Byte offset in file
    int size;                   // NAL unit size in bytes
    int frameNumber;            // Parent frame number

    // H.264/H.265 specific
    int nalUnitType;            // NAL type value
    QString typeName;           // Human-readable type name
    int layerId;                // HEVC: spatial/quality layer (0 for H.264)
    int temporalId;             // HEVC: temporal layer (-1 for H.264)

    // Slice-specific (for slice NAL units)
    bool isSlice;
    QString sliceType;          // "I", "P", "B", or empty
    int sliceQP;                // Quantization parameter (-1 if unknown)

    // Flags
    bool isIDR;
    bool isKeyFrame;

    NALUnitInfo()
        : index(0), fileOffset(0), size(0), frameNumber(0),
          nalUnitType(0), layerId(0), temporalId(-1),
          isSlice(false), sliceQP(-1), isIDR(false), isKeyFrame(false)
    {}
};

struct AudioFrameInfo {
    int index;
    int64_t fileOffset;
    int size;
    int frameNumber;
    QString codecType;          // "AAC", "AC-3", etc.
    QString frameName;          // "AAC raw_data_block"

    AudioFrameInfo()
        : index(0), fileOffset(0), size(0), frameNumber(0)
    {}
};

} // namespace VideoStudio

#endif // NALDATA_H
