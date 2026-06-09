# SPS/PPS 详细解析增强 (Enhanced SPS/PPS Parsing)

## 概述 (Overview)

本次更新为 VideoStudio 的 NAL Unit List 功能添加了详细的 SPS/PPS 参数解析和显示，使其能够显示更多的编码参数，满足专业视频分析需求。

## 新增 SPS 参数 (New SPS Parameters)

### 基础信息 (Basic Information)
- **Profile IDC**: 编码档次 (Baseline, Main, High, High 10, High 4:2:2, High 4:4:4)
- **Level IDC**: 编码级别 (例如 5.1)
- **Constraint Set Flags**: 约束集标志 (Set0, Set1, Set2, Set3)
- **Resolution**: 分辨率 (宽×高)
- **Chroma Format**: 色度格式 (4:0:0, 4:2:0, 4:2:2, 4:4:4)
- **Bit Depth**: 位深度 (Luma 和 Chroma)

### 帧结构信息 (Frame Structure Information)
- **Frame Mode**: 帧模式 (Progressive 渐进式 / Interlaced 隔行扫描)
- **Max Reference Frames**: 最大参考帧数量
- **POC Type**: 图像顺序计数类型 (0, 1, 2)
- **Max POC LSB**: 最大 POC LSB 值 (当 POC Type = 0)
- **Max Frame Num**: 最大帧编号
- **Gaps in Frame Num**: 是否允许帧编号间隙

### VUI 参数 (VUI Parameters)
- **Aspect Ratio**: 宽高比信息
  - 标准比例: 1:1, 4:3, 16:9, 等
  - Extended SAR: 自定义宽高比
- **Timing Info**: 时序信息
  - Num Units in Tick: 时钟单位数
  - Time Scale: 时间刻度
  - Fixed Frame Rate: 固定帧率标志
  - **Calculated Frame Rate**: 计算得出的帧率 (fps)

## 新增 PPS 参数 (New PPS Parameters)

### 基础编码参数 (Basic Encoding Parameters)
- **Entropy Coding Mode**: 熵编码模式 (CAVLC / CABAC)
- **Slice Groups**: 片组数量
- **Initial QP**: 初始量化参数 (默认 26)
- **Chroma QP Offset**: 色度 QP 偏移

### 编码工具 (Encoding Tools)
- **Deblocking Filter Control**: 去块滤波控制
- **Constrained Intra Pred**: 约束帧内预测
- **Redundant Pic Count**: 冗余图像计数
- **8×8 Transform**: 8×8 变换模式 (High profiles)

### 加权预测 (Weighted Prediction)
- **P-slices**: P 片加权预测 (Enabled/Disabled)
- **B-slices**: B 片加权预测 (Disabled/Explicit/Implicit)

## 实现细节 (Implementation Details)

### 修改的文件 (Modified Files)

1. **src/core/naldata.h**
   - 添加了 20+ 个新的 SPS/PPS 字段
   - 包括约束标志、VUI 参数、时序信息等

2. **src/core/nalunitparser.cpp**
   - `parseH264SPS()`: 增强的 SPS 解析
     - 解析约束标志 (constraint_set_flags)
     - 解析 VUI 参数 (aspect ratio, timing info)
     - 计算帧率 (从 time_scale 和 num_units_in_tick)
   - `parseH264PPS()`: 增强的 PPS 解析
     - 解析 QP 参数
     - 解析编码工具标志
     - 支持 High profile 扩展参数

3. **src/panels/propertypanel.cpp**
   - `displayNALUnit()`: 显著增强的显示逻辑
     - 分层显示 SPS/PPS 参数
     - VUI 参数折叠显示
     - 颜色编码关键信息
     - 计算并显示实际帧率

## 使用方法 (Usage)

1. 打开 MP4/MKV 视频文件
2. 切换到 "NAL Unit List" 标签页
3. 点击 SPS 或 PPS NAL 单元
4. 在右侧 Property Panel 查看详细参数
5. VUI Parameters 默认折叠，可点击展开查看

## 显示特性 (Display Features)

### 颜色编码 (Color Coding)
- **Orange (橙色)**: SPS/PPS 标题
- **Blue (蓝色)**: VUI Parameters 标题
- **Green (绿色)**: 重要参数值 (如 CABAC, 帧率)
- **Yellow (黄色)**: 备选参数值 (如 CAVLC)

### 智能显示 (Smart Display)
- 仅显示非默认值的参数
- 自动计算衍生值 (如帧率)
- 分层组织相关参数
- 提供详细的参数解释

## 技术参考 (Technical Reference)

### H.264 标准参考
- ITU-T H.264 (2019) - Annex A: Profiles
- ITU-T H.264 (2019) - Annex E: VUI parameters

### 帧率计算公式 (Frame Rate Calculation)
```
frame_rate = time_scale / (2 * num_units_in_tick)
```

### 宽高比 IDC 值 (Aspect Ratio IDC Values)
- 1 = 1:1 (Square)
- 2 = 12:11
- 3 = 10:11
- 4 = 16:11
- 13 = 160:99
- 14 = 4:3
- 15 = 3:2
- 16 = 2:1
- 255 = Extended_SAR (自定义)

## 已知限制 (Known Limitations)

1. **Scaling Matrix**: 缩放矩阵仅粗略跳过，未详细解析
2. **FMO Parameters**: 片组映射参数未完全支持
3. **H.265 VUI**: HEVC VUI 参数暂未扩展
4. **Bitstream Restrictions**: 未解析 VUI 中的码流限制标志

## 未来改进 (Future Enhancements)

1. 添加 HEVC (H.265) SPS/PPS 详细解析
2. 解析 SEI 消息内容
3. 显示更多 VUI 参数 (颜色空间、视频格式等)
4. 添加参数验证和合规性检查
5. 导出 SPS/PPS 参数到 CSV

## 测试建议 (Testing Recommendations)

### 测试文件类型
1. **Baseline Profile**: 简单的移动视频
2. **Main Profile**: 标准广播视频
3. **High Profile**: 高质量视频
4. **High 10 Profile**: 10-bit 视频
5. **不同帧率**: 24fps, 25fps, 30fps, 60fps
6. **不同分辨率**: 720p, 1080p, 4K

### 验证要点
- [ ] Profile 和 Level 显示正确
- [ ] 分辨率匹配实际值
- [ ] 帧率计算准确
- [ ] VUI 参数正确解析
- [ ] 宽高比显示正确
- [ ] 编码工具标志准确

## 版本信息 (Version Information)

- **Feature Added**: 2026-06-08
- **VideoStudio Version**: 1.0.0+
- **Supported Codecs**: H.264/AVC, H.265/HEVC (partial)
- **Platform**: macOS, Windows (planned)
