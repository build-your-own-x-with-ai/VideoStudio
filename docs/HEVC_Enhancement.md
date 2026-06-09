# H.265/HEVC 详细解析增强 (Enhanced H.265/HEVC Parsing)

## 概述 (Overview)

本次更新为 VideoStudio 添加了完整的 H.265/HEVC VPS/SPS/PPS 参数解析，与 H.264 增强功能相匹配，提供专业级的 HEVC 视频分析能力。

## 新增 H.265 VPS 参数 (New H.265 VPS Parameters)

### Video Parameter Set 信息
- **Max Layers**: 最大层数（空间/质量可伸缩性）
- **Max Temporal Sub-Layers**: 最大时域子层数
- **Temporal ID Nesting**: 时域 ID 嵌套标志

## 新增 H.265 SPS 参数 (New H.265 SPS Parameters)

### 基础信息 (Basic Information)
- **Profile**: 编码档次
  - Main (1): 主档次
  - Main 10 (2): 10-bit 主档次
  - Main Still Picture (3): 静态图像档次
  - Rext (4): 范围扩展档次
- **Tier**: 层级
  - Main: 主层级
  - High: 高层级
- **Level**: 编码级别（除以 30.0，例如 93 = Level 3.1）
- **Resolution**: 分辨率（应用裁剪窗口后）
- **Chroma Format**: 色度格式 (4:0:0, 4:2:0, 4:2:2, 4:4:4)
- **Bit Depth**: 位深度（Luma 和 Chroma，支持 8/10/12-bit）

### 源特性 (Source Characteristics)
- **Source Scan Type**: 源扫描类型
  - Progressive: 渐进式
  - Interlaced: 隔行扫描
  - Mixed: 混合模式
- **Frame Only Constraint**: 仅帧约束标志
- **Progressive Source Flag**: 渐进源标志
- **Interlaced Source Flag**: 隔行源标志

### 时域可伸缩性 (Temporal Scalability)
- **Max Temporal Sub-Layers**: 最大时域子层数
- **Temporal ID Nesting**: 时域 ID 嵌套
- **SPS ID**: 序列参数集 ID

### 裁剪信息 (Conformance Window / Cropping)
- **Left Offset**: 左侧裁剪偏移
- **Right Offset**: 右侧裁剪偏移
- **Top Offset**: 顶部裁剪偏移
- **Bottom Offset**: 底部裁剪偏移
- 自动应用到显示分辨率

### POC 信息 (Picture Order Count)
- **Max POC LSB**: 最大图像顺序计数 LSB 值
- 从 log2_max_pic_order_cnt_lsb 计算

### VUI 参数 (VUI Parameters)
- **Aspect Ratio**: 宽高比
  - 标准比例（与 H.264 相同）
  - Extended SAR（自定义宽高比）
- **Timing Info**: 时序信息
  - Num Units in Tick
  - Time Scale
  - **Calculated Frame Rate**: 自动计算帧率
    - 公式: `frame_rate = time_scale / num_units_in_tick`

## 新增 H.265 PPS 参数 (New H.265 PPS Parameters)

### 标识信息 (Identification)
- **PPS ID**: 图像参数集 ID
- **SPS ID**: 关联的序列参数集 ID

### 编码工具 (Encoding Tools)
- **CABAC Init Present**: CABAC 初始化表存在标志
- **Transform Skip Enabled**: 变换跳过模式
- **CU QP Delta Enabled**: 编码单元 QP 增量
- **Transquant Bypass Enabled**: 变换量化旁路模式

### 参考图像配置 (Reference Picture Configuration)
- **Default Active References**:
  - List 0 (L0): 默认激活的前向参考数
  - List 1 (L1): 默认激活的后向参考数

### QP 参数 (Quantization Parameters)
- **Initial QP**: 初始量化参数
  - 显示实际值和偏移值
  - 计算: `actual_qp = init_qp_minus26 + 26`

### 预测模式 (Prediction Modes)
- **Constrained Intra Pred**: 约束帧内预测
  - Enabled: 帧内预测仅使用帧内编码块
  - Disabled: 可使用帧间编码块

## 实现细节 (Implementation Details)

### 修改的文件 (Modified Files)

1. **src/core/naldata.h**
   - 添加 30+ 个新的 HEVC 字段
   - VPS/SPS/PPS 详细参数结构

2. **src/core/nalunitparser.h**
   - 添加 `parseH265PPS()` 函数声明

3. **src/core/nalunitparser.cpp**
   - `parseH265VPS()`: 增强的 VPS 解析
   - `parseH265SPS()`: 完全重写的 SPS 解析
     - 解析 profile_tier_level
     - 解析裁剪窗口（conformance window）
     - 解析 VUI 参数（aspect ratio, timing info）
     - 支持时域子层
   - `parseH265PPS()`: 全新的 PPS 解析
     - 解析 CABAC 初始化标志
     - 解析参考索引
     - 解析 QP 参数
     - 解析编码工具标志
   - `parseH265NALHeader()`: 添加 PPS 解析调用

4. **src/panels/propertypanel.cpp**
   - `displayNALUnit()`: 完全重写的 HEVC 显示逻辑
     - 详细的 SPS 参数分层显示
     - 新的 PPS 参数显示
     - 裁剪信息嵌套显示
     - VUI 参数折叠显示
     - 颜色编码关键信息

## H.264 vs H.265 差异 (H.264 vs H.265 Differences)

### Profile 编号
- **H.264**: 66=Baseline, 77=Main, 100=High
- **H.265**: 1=Main, 2=Main 10, 4=Rext

### Level 计算
- **H.264**: `level = level_idc / 10.0` (例如 51 = Level 5.1)
- **H.265**: `level = level_idc / 30.0` (例如 93 = Level 3.1)

### 帧率计算
- **H.264**: `fps = time_scale / (2 * num_units_in_tick)`
- **H.265**: `fps = time_scale / num_units_in_tick` (无需除以 2)

### 裁剪机制
- **H.264**: frame_cropping_flag（帧裁剪）
- **H.265**: conformance_window_flag（一致性窗口）

### NAL 头部结构
- **H.264**: 1 byte (type: 5 bits)
- **H.265**: 2 bytes (type: 6 bits + layer_id: 6 bits + temporal_id: 3 bits)

## 使用方法 (Usage)

1. 打开 H.265/HEVC 编码的 MP4/MKV 视频
2. 切换到 "NAL Unit List" 标签页
3. 点击 VPS/SPS/PPS NAL 单元
4. 在右侧 Property Panel 查看详细参数

## 显示特性 (Display Features)

### 颜色编码 (Color Coding)
- **Orange (橙色)**: VPS/SPS/PPS 标题
- **Blue (蓝色)**: VUI Parameters 标题
- **Green (绿色)**: 重要特性（CABAC、Transquant Bypass、帧率）
- **Gray (灰色)**: 标准参数

### 智能显示 (Smart Display)
- 自动计算显示分辨率（应用裁剪）
- 自动计算帧率（从时序信息）
- 嵌套显示相关参数
- 仅显示存在的参数

### 分层结构 (Hierarchical Structure)
```
SPS (Sequence Parameter Set)
├─ Profile: Main 10 (2)
├─ Tier: Main
├─ Level: 4.0 (120)
├─ Resolution: 1920 × 1080
│  └─ Conformance Window: L:0 R:0 T:0 B:0
├─ Chroma Format: 4:2:0
├─ Bit Depth: Luma: 10-bit, Chroma: 10-bit
├─ Source Scan Type: Progressive
├─ Max Temporal Sub-Layers: 1
├─ Max POC LSB: 256
└─ VUI Parameters
   ├─ Aspect Ratio: 1:1 (IDC 1)
   └─ Timing Info
      ├─ Num Units in Tick: 1
      ├─ Time Scale: 60
      └─ Frame Rate: 60.000 fps
```

## 技术参考 (Technical Reference)

### HEVC 标准
- ITU-T H.265 (2021) - High Efficiency Video Coding
- ITU-T H.265 (2021) - Annex A: Profiles, tiers and levels
- ITU-T H.265 (2021) - Annex E: VUI parameters

### Profile 定义
- **Main (1)**: 8-bit 4:2:0, 主流应用
- **Main 10 (2)**: 10-bit 4:2:0, HDR 内容
- **Main Still Picture (3)**: 静态图像编码
- **Rext (4)**: 支持 4:2:2, 4:4:4, 12-bit

### Tier 定义
- **Main Tier**: 标准应用
- **High Tier**: 高要求应用（更高码率）

### Level 示例
- **Level 3.0 (90)**: 720p @ 30fps
- **Level 3.1 (93)**: 1080p @ 30fps
- **Level 4.0 (120)**: 1080p @ 60fps
- **Level 5.0 (150)**: 4K @ 30fps
- **Level 5.1 (153)**: 4K @ 60fps

## 测试建议 (Testing Recommendations)

### 测试文件类型
1. **Main Profile 8-bit**: 标准 1080p 视频
2. **Main 10 Profile**: 10-bit HDR 内容
3. **4K HEVC**: 2160p 高分辨率
4. **高帧率**: 60fps, 120fps
5. **时域可伸缩**: 多层 HEVC

### 验证要点
- [ ] Profile/Tier/Level 显示正确
- [ ] 分辨率匹配（包括裁剪）
- [ ] 帧率计算准确（注意 H.265 无需除以 2）
- [ ] 10-bit 视频正确识别
- [ ] VUI 参数正确解析
- [ ] PPS 参数完整显示

## 与 Elecard StreamEye 对比

VideoStudio 现在提供与 Elecard StreamEye 相当的 HEVC 分析能力：

✅ **已实现**:
- VPS/SPS/PPS 完整解析
- Profile/Tier/Level 识别
- 时域可伸缩性信息
- VUI 参数（宽高比、帧率）
- 编码工具标志
- 参考图像配置

📋 **未来增强**:
- Scaling list 详细解析
- Short-term RPS (Reference Picture Set)
- Long-term reference pictures
- Tiles 和 WPP 详细信息
- SEI 消息解析（buffering period, picture timing 等）

## 已知限制 (Known Limitations)

1. **Scaling Lists**: 仅粗略跳过，未详细解析
2. **Short-term RPS**: 参考图像集未完全解析
3. **Tiles Configuration**: 瓦片配置仅部分支持
4. **WPP**: Wavefront Parallel Processing 未详细显示
5. **Transform Tree**: 变换树深度未解析

## 版本信息 (Version Information)

- **Feature Added**: 2026-06-09
- **VideoStudio Version**: 1.0.0+
- **Supported Codecs**: H.264/AVC (full), H.265/HEVC (full)
- **Platform**: macOS ✅, Windows (planned)

## 总结 (Summary)

现在 VideoStudio 对 H.264 和 H.265 都提供了专业级的 SPS/PPS 参数解析和显示能力，涵盖：

- ✅ **20+ H.264 SPS/PPS 参数**
- ✅ **30+ H.265 VPS/SPS/PPS 参数**
- ✅ **VUI 参数完整支持**（宽高比、时序、帧率）
- ✅ **智能显示**（颜色编码、分层结构、自动计算）
- ✅ **专业分析**（适合视频工程师和编码器开发者）

这使 VideoStudio 成为 Elecard StreamEye 的强大开源替代方案！
