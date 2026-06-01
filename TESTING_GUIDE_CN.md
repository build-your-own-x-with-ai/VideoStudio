# VideoStudio - 测试文件使用指南

## 快速开始

### 1. 生成测试文件

```bash
cd /Users/i/Code/VideoStudio
./generate_test_files.sh
```

这将在 `test_files/` 目录中创建各种格式的测试视频文件。

### 2. 测试 AVI 支持

```bash
# 构建项目
cd build
cmake --build .

# 测试 AVI 文件
./VideoStudio.app/Contents/MacOS/VideoStudio ../test_files/test_h264.avi
```

### 3. 验证功能

打开 AVI 文件后，应该看到：

✅ **Explorer Panel (左侧)**
- 显示 "AVI stream" 根节点
- 显示 RIFF/LIST 层次结构
- 显示 avih, strh, strf 等 chunk
- 每个 chunk 显示 FourCC, 偏移量, 百分比

✅ **Property Panel (右侧)**
- 点击 Explorer 中的 chunk
- 显示详细属性：
  - FourCC, Offset, Size, Total Size
  - Percentage, Level
  - avih: 分辨率, 帧率, 总帧数, 流数量
  - strh: 流类型, 编解码器, 速率

✅ **Hex Viewer Panel (底部)**
- 与 Explorer 选择同步
- 显示 chunk 的十六进制数据
- 支持搜索功能
- 可切换 Hex/Binary 模式

✅ **Video Output (中央)**
- 正常播放视频
- 帧导航功能正常
- Bar Chart 显示帧大小

## 测试其他格式

### MP4 文件
```bash
./VideoStudio.app/Contents/MacOS/VideoStudio ../test_files/test_h264.mp4
```
- 应显示 MP4 atom 层次结构 (ftyp, moov, mdat, etc.)

### MKV 文件
```bash
./VideoStudio.app/Contents/MacOS/VideoStudio ../test_files/test_h264.mkv
```
- 应显示 EBML 元素结构 (Segment, Tracks, Cluster, etc.)

### Transport Stream 文件
```bash
./VideoStudio.app/Contents/MacOS/VideoStudio ../test_files/test_h264.ts
```
- 应显示 TS 包列表
- 显示 PSI/SI 表
- TR 101-290 合规性检查

## 测试场景

### 场景 1: 容器解析
1. 打开不同格式的文件 (AVI, MP4, MKV, TS)
2. 验证 Explorer Panel 显示正确的层次结构
3. 检查百分比计算是否正确

### 场景 2: 属性显示
1. 在 Explorer 中点击不同的元素
2. 验证 Property Panel 显示正确的字段
3. 测试 Sync/Compare/Dump 模式

### 场景 3: 十六进制查看
1. 选择 Explorer 中的元素
2. 验证 Hex Viewer 显示正确的数据
3. 测试搜索功能（例如搜索 "47" 查找 TS 同步字节）
4. 测试 Bytes/Row 设置（8, 16, 24, 32）

### 场景 4: 视频播放
1. 播放/暂停控制
2. 逐帧前进/后退
3. 通过 Bar Chart 或 Thumbnails 跳转
4. 验证帧信息显示正确

### 场景 5: 多格式切换
1. 打开 AVI 文件
2. 关闭并打开 MP4 文件
3. 验证 UI 正确切换（Explorer 标题变化）
4. 验证没有内存泄漏或崩溃

## 已知问题

- Packet View 限制为 1000 个包（性能优化）
- 非常大的文件可能需要较长加载时间
- 某些编解码器可能需要额外的 FFmpeg 支持

## 性能测试

### 小文件 (< 10MB)
- 应立即打开
- Explorer 应立即显示结构

### 中等文件 (10-100MB)
- 应在 1-2 秒内打开
- 解析应在后台进行

### 大文件 (> 100MB)
- 可能需要几秒钟
- 应显示进度指示

## 调试技巧

### 查看解析日志
```bash
# 运行时会在控制台输出调试信息
./VideoStudio.app/Contents/MacOS/VideoStudio ../test_files/test_h264.avi 2>&1 | grep -i "avi\|chunk\|parse"
```

### 验证文件结构
```bash
# 使用 ffprobe 验证文件信息
ffprobe -v quiet -print_format json -show_format -show_streams ../test_files/test_h264.avi

# 使用 mediainfo 查看详细信息
mediainfo ../test_files/test_h264.avi
```

### 十六进制查看
```bash
# 查看文件头部
xxd ../test_files/test_h264.avi | head -20

# AVI 文件应以 "RIFF" 开头
# 偏移 8 应该是 "AVI " 或 "AVIX"
```

## 报告问题

如果发现问题，请提供：
1. 测试文件名称
2. 操作步骤
3. 预期行为 vs 实际行为
4. 控制台输出（如果有错误）
5. 截图（如果 UI 显示异常）

## 下一步

完成 AVI 测试后，可以继续实现：
- [ ] FLV 格式支持
- [ ] WebM 格式支持（可复用 MKV 解析器）
- [ ] 更多 TR 101-290 检查
- [ ] EPG 面板实现
- [ ] Comments 面板实现
- [ ] Graphics 面板实现
