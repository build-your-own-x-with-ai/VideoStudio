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

    // Slice header fields (parsed from slice NAL units)
    int firstMbInSlice;         // First macroblock address in slice
    int sliceTypeValue;         // Raw slice_type value (0-9)
    int picOrderCntLsb;         // Picture order count LSB
    int frameNum;               // Frame number from slice header
    bool fieldPicFlag;          // Field/frame flag
    bool bottomFieldFlag;       // Bottom field flag (if field_pic_flag)
    int idrPicId;               // IDR picture ID (for IDR slices)
    int ppsId;                  // PPS ID referenced by this slice
    int numRefIdxL0ActiveMinus1; // Number of reference pictures in list 0
    int numRefIdxL1ActiveMinus1; // Number of reference pictures in list 1
    bool sliceQpDeltaValid;     // Whether sliceQP was successfully parsed

    // Reference picture list modification
    bool refPicListModificationFlagL0;  // Reference list 0 modification present
    bool refPicListModificationFlagL1;  // Reference list 1 modification present
    QVector<int> refPicListL0;          // Reference frame indices for L0
    QVector<int> refPicListL1;          // Reference frame indices for L1

    // Flags
    bool isIDR;
    bool isKeyFrame;

    // H.264 SPS (Sequence Parameter Set) fields
    int spsProfileIdc;          // Profile (66=Baseline, 77=Main, 100=High)
    int spsLevelIdc;            // Level (e.g., 51 = Level 5.1)
    int spsWidth;               // Coded width
    int spsHeight;              // Coded height
    int spsChromaFormat;        // 0=Mono, 1=4:2:0, 2=4:2:2, 3=4:4:4
    int spsBitDepthLuma;        // Luma bit depth (8, 10, 12)
    int spsBitDepthChroma;      // Chroma bit depth

    // SPS additional fields
    bool spsConstraintSet0Flag; // Constraint set 0 flag
    bool spsConstraintSet1Flag; // Constraint set 1 flag
    bool spsConstraintSet2Flag; // Constraint set 2 flag
    bool spsConstraintSet3Flag; // Constraint set 3 flag
    int spsMaxNumRefFrames;     // Maximum number of reference frames
    bool spsFrameMbsOnlyFlag;   // true=progressive, false=interlaced possible
    int spsPicOrderCntType;     // POC type (0, 1, or 2)
    int spsLog2MaxFrameNum;     // log2_max_frame_num_minus4 + 4
    int spsLog2MaxPicOrderCntLsb; // log2_max_pic_order_cnt_lsb_minus4 + 4
    bool spsGapsInFrameNumAllowed; // Gaps in frame_num allowed

    // SPS VUI (Video Usability Information) parameters
    bool spsVuiPresent;         // VUI parameters present
    int spsAspectRatioIdc;      // Aspect ratio IDC (0-255)
    int spsSarWidth;            // SAR width (if aspect_ratio_idc == 255)
    int spsSarHeight;           // SAR height (if aspect_ratio_idc == 255)
    bool spsTimingInfoPresent;  // Timing info present in VUI
    int spsNumUnitsInTick;      // Frame rate numerator
    int spsTimeScale;           // Frame rate denominator
    bool spsFixedFrameRate;     // Fixed frame rate flag

    // H.264 PPS (Picture Parameter Set) fields
    bool ppsEntropyCodingMode;  // false=CAVLC, true=CABAC
    int ppsNumSliceGroups;      // Number of slice groups
    bool ppsDeblockingFilter;   // Deblocking filter present
    bool ppsWeightedPred;       // Weighted prediction for P slices
    int ppsWeightedBipred;      // Weighted prediction for B slices (0=none, 1=explicit, 2=implicit)

    // PPS additional fields
    int ppsPicInitQp;           // Initial QP value (pic_init_qp_minus26 + 26)
    int ppsChromaQpIndexOffset; // Chroma QP offset
    bool ppsConstainedIntraPred; // Constrained intra prediction
    bool ppsRedundantPicCnt;    // Redundant picture count present
    bool ppsTransform8x8Mode;   // 8x8 transform mode (High profiles)

    // H.265 VPS (Video Parameter Set) fields
    int vpsMaxLayers;           // Maximum number of layers
    int vpsMaxSubLayers;        // Maximum temporal sub-layers
    int vpsTemporalIdNesting;   // Temporal ID nesting flag

    // H.265 SPS additional fields
    bool hevcSpsPresent;        // HEVC SPS parsed flag
    int hevcSpsMaxSubLayersMinus1; // Max temporal sub-layers - 1
    bool hevcSpsTemporalIdNesting; // Temporal ID nesting
    int hevcSpsSeqParameterSetId;  // SPS ID
    bool hevcConformanceWindowFlag; // Cropping present
    int hevcConfWinLeftOffset;   // Conformance window left
    int hevcConfWinRightOffset;  // Conformance window right
    int hevcConfWinTopOffset;    // Conformance window top
    int hevcConfWinBottomOffset; // Conformance window bottom

    // HEVC profile/tier/level
    bool hevcGeneralTierFlag;    // General tier (false=Main, true=High)
    bool hevcGeneralProgressiveSourceFlag;
    bool hevcGeneralInterlacedSourceFlag;
    bool hevcGeneralFrameOnlyConstraintFlag;

    // HEVC VUI parameters
    bool hevcVuiPresent;
    int hevcVuiAspectRatioIdc;
    int hevcVuiSarWidth;
    int hevcVuiSarHeight;
    bool hevcVuiTimingInfoPresent;
    int hevcVuiNumUnitsInTick;
    int hevcVuiTimeScale;

    // H.265 PPS additional fields
    bool hevcPpsPresent;
    int hevcPpsPicParameterSetId;
    int hevcPpsSeqParameterSetId;
    bool hevcPpsCabacInitPresent;
    int hevcPpsNumRefIdxL0DefaultActive; // Num ref idx L0
    int hevcPpsNumRefIdxL1DefaultActive; // Num ref idx L1
    int hevcPpsInitQpMinus26;
    bool hevcPpsConstrainedIntraPred;
    bool hevcPpsTransformSkipEnabled;
    bool hevcPpsCuQpDeltaEnabled;
    bool hevcPpsTransquantBypassEnabled;

    NALUnitInfo()
        : index(0), fileOffset(0), size(0), frameNumber(0),
          nalUnitType(0), layerId(0), temporalId(-1),
          isSlice(false), sliceQP(-1),
          firstMbInSlice(0), sliceTypeValue(-1), picOrderCntLsb(-1),
          frameNum(-1), fieldPicFlag(false), bottomFieldFlag(false),
          idrPicId(-1), ppsId(-1),
          numRefIdxL0ActiveMinus1(-1), numRefIdxL1ActiveMinus1(-1),
          sliceQpDeltaValid(false),
          refPicListModificationFlagL0(false), refPicListModificationFlagL1(false),
          isIDR(false), isKeyFrame(false),
          spsProfileIdc(-1), spsLevelIdc(-1), spsWidth(0), spsHeight(0),
          spsChromaFormat(-1), spsBitDepthLuma(8), spsBitDepthChroma(8),
          spsConstraintSet0Flag(false), spsConstraintSet1Flag(false),
          spsConstraintSet2Flag(false), spsConstraintSet3Flag(false),
          spsMaxNumRefFrames(0), spsFrameMbsOnlyFlag(true),
          spsPicOrderCntType(-1), spsLog2MaxFrameNum(0), spsLog2MaxPicOrderCntLsb(0),
          spsGapsInFrameNumAllowed(false),
          spsVuiPresent(false), spsAspectRatioIdc(0), spsSarWidth(0), spsSarHeight(0),
          spsTimingInfoPresent(false), spsNumUnitsInTick(0), spsTimeScale(0),
          spsFixedFrameRate(false),
          ppsEntropyCodingMode(false), ppsNumSliceGroups(1), ppsDeblockingFilter(true),
          ppsWeightedPred(false), ppsWeightedBipred(0),
          ppsPicInitQp(26), ppsChromaQpIndexOffset(0),
          ppsConstainedIntraPred(false), ppsRedundantPicCnt(false),
          ppsTransform8x8Mode(false),
          vpsMaxLayers(1), vpsMaxSubLayers(1), vpsTemporalIdNesting(0),
          hevcSpsPresent(false), hevcSpsMaxSubLayersMinus1(0), hevcSpsTemporalIdNesting(false),
          hevcSpsSeqParameterSetId(0),
          hevcConformanceWindowFlag(false), hevcConfWinLeftOffset(0), hevcConfWinRightOffset(0),
          hevcConfWinTopOffset(0), hevcConfWinBottomOffset(0),
          hevcGeneralTierFlag(false), hevcGeneralProgressiveSourceFlag(false),
          hevcGeneralInterlacedSourceFlag(false), hevcGeneralFrameOnlyConstraintFlag(false),
          hevcVuiPresent(false), hevcVuiAspectRatioIdc(0), hevcVuiSarWidth(0), hevcVuiSarHeight(0),
          hevcVuiTimingInfoPresent(false), hevcVuiNumUnitsInTick(0), hevcVuiTimeScale(0),
          hevcPpsPresent(false), hevcPpsPicParameterSetId(0), hevcPpsSeqParameterSetId(0),
          hevcPpsCabacInitPresent(false), hevcPpsNumRefIdxL0DefaultActive(0),
          hevcPpsNumRefIdxL1DefaultActive(0), hevcPpsInitQpMinus26(0),
          hevcPpsConstrainedIntraPred(false), hevcPpsTransformSkipEnabled(false),
          hevcPpsCuQpDeltaEnabled(false), hevcPpsTransquantBypassEnabled(false)
    {}
};

// AAC Audio Object Types
enum AACAudioObjectType {
    AAC_MAIN = 1,
    AAC_LC = 2,              // Low Complexity (most common)
    AAC_SSR = 3,
    AAC_LTP = 4,
    AAC_SBR = 5,             // HE-AAC (High Efficiency)
    AAC_SCALABLE = 6,
    AAC_TWINVQ = 7,
    AAC_CELP = 8,
    AAC_HVXC = 9,
    AAC_ER_AAC_LC = 17,
    AAC_ER_AAC_LTP = 19,
    AAC_ER_AAC_SCALABLE = 20,
    AAC_ER_TWINVQ = 21,
    AAC_ER_BSAC = 22,
    AAC_ER_AAC_LD = 23,
    AAC_PS = 29              // HE-AACv2 (Parametric Stereo)
};

// AAC Channel Configurations
enum AACChannelConfig {
    AAC_CHANNEL_DEFINED_IN_AOT = 0,
    AAC_CHANNEL_MONO = 1,
    AAC_CHANNEL_STEREO = 2,
    AAC_CHANNEL_3_0 = 3,
    AAC_CHANNEL_4_0 = 4,
    AAC_CHANNEL_5_0 = 5,
    AAC_CHANNEL_5_1 = 6,
    AAC_CHANNEL_7_1 = 7
};

struct AudioFrameInfo {
    int index;
    int64_t fileOffset;
    int size;
    int frameNumber;
    QString codecType;          // "AAC", "AC-3", "MP3", etc.
    QString frameName;          // "AAC raw_data_block", "AC-3 frame", etc.

    // AAC-specific (ADTS header info)
    bool hasADTS;               // ADTS header present (vs raw AAC)
    int audioObjectType;        // AAC profile (1=Main, 2=LC, 5=SBR/HE-AAC, 29=PS/HE-AACv2)
    int samplingFrequency;      // Sample rate in Hz (44100, 48000, etc.)
    int channelConfig;          // 1=Mono, 2=Stereo, 6=5.1, etc.
    int frameLength;            // ADTS frame length including header
    bool protectionAbsent;      // CRC protection flag

    // MP3-specific
    int mpegVersion;            // MPEG-1, MPEG-2, MPEG-2.5
    int layer;                  // Layer I, II, III
    int bitrate;                // Bitrate in kbps

    AudioFrameInfo()
        : index(0), fileOffset(0), size(0), frameNumber(0),
          hasADTS(false), audioObjectType(0), samplingFrequency(0),
          channelConfig(0), frameLength(0), protectionAbsent(true),
          mpegVersion(0), layer(0), bitrate(0)
    {}
};

} // namespace VideoStudio

#endif // NALDATA_H
