#!/usr/bin/env python3
# VideoStudio Translation Dictionary - English to Simplified Chinese

translations = {
    # Technical terms - Video coding
    "DTS": "DTS",  # Decoding Time Stamp - keep as-is
    "PTS": "PTS",  # Presentation Time Stamp - keep as-is
    "SPS": "SPS",  # Sequence Parameter Set - keep as-is
    "PPS": "PPS",  # Picture Parameter Set - keep as-is
    "VPS": "VPS",  # Video Parameter Set - keep as-is
    "Slice": "切片",
    "NAL": "NAL",  # Network Abstraction Layer - keep as-is
    "Bitstream": "比特流",
    "GOP": "GOP",  # Group of Pictures - keep as-is

    # File operations
    "Load Comments": "加载注释",
    "Save Comments": "保存注释",
    "XML Files (*.xml);;All Files (*)": "XML 文件 (*.xml);;所有文件 (*)",
    "JSON Files (*.json);;Text Files (*.txt);;All Files (*)": "JSON 文件 (*.json);;文本文件 (*.txt);;所有文件 (*)",
    "CSV Files (*.csv);;All Files (*)": "CSV 文件 (*.csv);;所有文件 (*)",
    "Binary Files (*.bin);;All Files (*)": "二进制文件 (*.bin);;所有文件 (*)",
    "Export Report": "导出报告",
    "Export Error": "导出错误",
    "Export Failed": "导出失败",
    "Failed to open file for writing": "无法打开文件进行写入",
    "Failed to open file for writing:\n%1": "无法打开文件进行写入：\n%1",
    "Export Complete": "导出完成",
    "Save Atom Data": "保存 Atom 数据",
    "Save Element Data": "保存元素数据",
    "Save Chunk Data": "保存块数据",
    "Save Tag Data": "保存标签数据",
    "Save Elementary Stream": "保存基本流",
    "Failed to open source file": "无法打开源文件",
    "Failed to open destination file": "无法打开目标文件",

    # Compliance Validation
    "H.264/H.265 Compliance Validation": "H.264/H.265 合规性验证",
    "Video File": "视频文件",
    "Ready to validate": "准备验证",
    "Validation Issues": "验证问题",
    "Severity": "严重程度",
    "Category": "类别",
    "Description": "描述",
    "Frame": "帧",
    "Issue Details": "问题详情",
    "Start Validation": "开始验证",
    "Close": "关闭",
    "Validating...": "正在验证...",
    "Validation Error": "验证错误",
    "Failed to validate file. See issues for details.": "验证文件失败。详见问题列表。",
    "Validation complete": "验证完成",
    "CRITICAL": "严重",
    "ERROR": "错误",
    "WARNING": "警告",
    "INFO": "信息",
    "Export Compliance Report": "导出合规性报告",
    "Compliance report exported successfully": "合规性报告导出成功",

    # Duplicate Frame Detection
    "Duplicate Frame Detection": "重复帧检测",
    "Analysis": "分析",
    "Analysis cancelled": "分析已取消",
    "Starting analysis...": "开始分析...",
    "Similarity Threshold:": "相似度阈值：",
    "0.000 = Exact match only (fastest)\n0.001-0.005 = Nearly identical (1-5 gray levels avg diff)\n0.01-0.02 = Very similar frames\n0.05+ = Allow noticeable differences": "0.000 = 仅完全匹配（最快）\n0.001-0.005 = 几乎相同（1-5 灰度级平均差）\n0.01-0.02 = 非常相似的帧\n0.05+ = 允许明显差异",
    "Unique Frames: -": "唯一帧：-",
    "Unique Frames: %1": "唯一帧：%1",
    "Duplicate Frames: -": "重复帧：-",
    "Duplicate Groups: -": "重复组：-",
    "Longest Freeze: -": "最长冻结：-",
    "Longest Freeze: %1": "最长冻结：%1",
    "Consecutive": "连续",
    "Consecutive Duplicate Frames: %1 (%2%)": "连续重复帧：%1 (%2%)",
    "Freeze Frame Groups: %1": "冻结帧组：%1",
    "Duplicate Groups": "重复组",
    "Group": "组",
    "Occurrences": "出现次数",
    "Actions": "操作",
    "Go to Frame": "跳转到帧",
    "No duplicate frames to export.": "没有重复帧可导出。",
    "Export Duplicate Frame Report": "导出重复帧报告",

    # Audio
    "No audio streams found": "未找到音频流",
    "Failed to open audio file: %1": "无法打开音频文件：%1",
    "No buffer data available": "无缓冲区数据",
    "Buffer Occupancy (Mbits)": "缓冲区占用 (Mbits)",
    "Frame Number": "帧号",

    # UI Controls
    "Zoom In": "放大",
    "Zoom Out": "缩小",
    "Fit All": "适应全部",
    "Tip: Ctrl+Wheel to zoom": "提示：Ctrl+滚轮缩放",
    "OK": "确定",
    "Cancel": "取消",
    "Browse...": "浏览...",
    "Select": "选择",
    "File": "文件",
    "Help": "帮助",
    "About": "关于",
    "README": "自述文件",
    "Remove": "移除",
    "Remove Parameter": "移除参数",
    "No parameters to remove.": "没有可移除的参数。",
    "Select parameter to remove:": "选择要移除的参数：",
    "No parameters to export.": "没有可导出的参数。",
    "Export Graphics Data": "导出图形数据",

    # Messages
    "Loading image...": "正在加载图片...",
    "Image load failed": "图片加载失败",
    "Failed to load image": "图片加载失败",
    "No Data": "无数据",
    "Error": "错误",
    "Atom not found": "未找到 Atom",
    "Element not found": "未找到元素",
    "Chunk not found": "未找到块",
    "Tag not found": "未找到标签",
    "Open Video File": "打开视频文件",
    "Loading cancelled": "加载已取消",
    "Opened: %1": "已打开：%1",
    "No video file loaded.": "未加载视频文件。",
    "Paused": "已暂停",
    "No GOP structure available": "无 GOP 结构可用",
    "Dumped %1 packets (%2 bytes) to:\n%3": "已转储 %1 个数据包（%2 字节）到：\n%3",
    "Dumped %1 bytes to:\n%2": "已转储 %1 字节到：\n%2",

    # Shortcuts
    "Ctrl+D": "Ctrl+D",
    "Ctrl+Shift+C": "Ctrl+Shift+C",
    "Ctrl+Shift+P": "Ctrl+Shift+P",
    "Ctrl+,": "Ctrl+,",

    # Toolbar
    "Main Toolbar": "主工具栏",
    "Play video (Space)": "播放视频（空格）",
    "Pause playback": "暂停播放",

    # Quality Metrics
    "Quality Metrics": "质量指标",
    "PSNR": "PSNR",
    "SSIM": "SSIM",
    "VMAF": "VMAF",

    # Export
    "CSV Export": "CSV 导出",
    "Export to CSV": "导出为 CSV",
    "Export data": "导出数据",
    "Frame list": "帧列表",
    "Statistics": "统计信息",
    "GOP structure": "GOP 结构",
    "Bitrate data": "比特率数据",

    # Common UI
    "Open": "打开",
    "Save": "保存",
    "Exit": "退出",
    "Settings": "设置",
    "Language": "语言",
    "Theme": "主题",
    "View": "视图",
    "Tools": "工具",
    "Window": "窗口",
}
