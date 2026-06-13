# VideoStudio CLI

Command-line interface for VideoStudio - Professional video stream analysis tool.

## Features

- **Video Information Extraction**: Get codec, resolution, frame rate, duration, and bitrate
- **Frame-Level Metrics**: Export CSV data with frame type, size, PTS, DTS, QP values
- **GOP Structure Analysis**: Analyze Group of Pictures structure with JSON output
- **H.264/H.265 Compliance Validation**: Validate bitstream compliance against ISO/IEC standards
- **Multiple Output Formats**: JSON, CSV, and text formats

## Installation

The CLI tool is built alongside the main VideoStudio application:

```bash
cd VideoStudio/build
cmake ..
cmake --build . --target videostudio-cli
```

The executable will be located at `build/videostudio-cli`.

## Usage

### Video Information

Extract video file information in various formats:

```bash
# Text format (human-readable)
videostudio-cli info video.mp4 --format text

# JSON format (machine-readable)
videostudio-cli info video.mp4 --format json

# CSV format
videostudio-cli info video.mp4 --format csv
```

**Example output (text):**
```
Video Information
=================
File:         video.mp4
Codec:        H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10
Resolution:   640x360
Frame Rate:   60.00 fps
Duration:     10.00 seconds
Total Frames: 598
Bitrate:      2114060 kb/s
Pixel Format: yuv420p
```

### Frame Metrics Export

Export frame-level metrics to CSV:

```bash
# Export all frames
videostudio-cli frames video.mp4 -o frames.csv

# Export specific frame range
videostudio-cli frames video.mp4 --start-frame 0 --end-frame 100 -o frames.csv

# Output to stdout
videostudio-cli frames video.mp4
```

**CSV columns:**
- `frame`: Frame number
- `type`: Frame type (I, P, B)
- `size`: Frame size in bytes
- `pts`: Presentation timestamp
- `dts`: Decoding timestamp
- `qp`: Quantization parameter
- `keyframe`: Is keyframe (1/0)

### GOP Structure Analysis

Analyze Group of Pictures (GOP) structure:

```bash
# JSON output to stdout
videostudio-cli gop video.mp4

# Save to file
videostudio-cli gop video.mp4 -o gop_analysis.json
```

**Example output:**
```json
{
    "file": "video.mp4",
    "total_frames": 598,
    "total_gops": 20,
    "gops": [
        {
            "gop_number": 0,
            "start_frame": 0,
            "end_frame": 27,
            "length": 28,
            "i_frames": 1,
            "p_frames": 6,
            "b_frames": 21
        },
        ...
    ]
}
```

### Compliance Validation

Validate H.264/H.265 bitstream compliance against ISO/IEC standards:

```bash
# Text report to stdout
videostudio-cli compliance video.mp4

# Save JSON report
videostudio-cli compliance video.mp4 -o report.json

# Save text report
videostudio-cli compliance video.mp4 -o report.txt
```

**What is validated:**

**H.264/AVC (ISO/IEC 14496-10):**
- Profile and Level compliance
- Resolution limits for the specified level
- Bitrate limits
- SPS/PPS parameter sets
- NAL unit structure
- Timestamp consistency (PTS/DTS)
- Reference frame configuration

**H.265/HEVC (ISO/IEC 23008-2):**
- Profile and Level compliance
- VPS/SPS/PPS parameter sets
- Basic bitstream structure

**Example output:**
```
===========================================
  H.264/H.265 Compliance Validation Report
===========================================

File: video.mp4
Total Issues: 9
  Errors:   0
  Warnings: 2
  Info:     7

-------------------------------------------
Issues:
-------------------------------------------

[INFO] Profile/Level: H.264 Profile: High, Level: 3.1

[WARNING] NAL Unit: No IDR frames found in first 100 frames
  Suggestion: Consider adding periodic IDR frames for seeking

[WARNING] Timing: Found 46 PTS discontinuities in first 100 frames
  Suggestion: Check timestamp generation
```

**Exit codes:**
- `0`: No errors found (warnings/info may be present)
- `1`: Errors found or validation failed

### HRD/VBV Buffer Analysis

Analyze buffer behavior and verify compliance with HRD (Hypothetical Reference Decoder) and VBV (Video Buffering Verifier) constraints:

```bash
# Text report to stdout
videostudio-cli buffer video.mp4

# Save JSON report
videostudio-cli buffer video.mp4 -o buffer_report.json

# Save text report
videostudio-cli buffer video.mp4 -o buffer_report.txt
```

**What is analyzed:**

- **CPB (Coded Picture Buffer) Simulation**: Simulates decoder buffer filling and draining
- **Buffer Occupancy**: Tracks buffer fullness over time
- **Overflow Detection**: Identifies when buffer exceeds capacity
- **Underflow Detection**: Identifies when buffer empties (stalls)
- **Bitrate Analysis**: Peak, average, and minimum instantaneous bitrates
- **Buffering Delays**: Maximum and average buffering latency

**Example output:**
```
===========================================
      HRD/VBV Buffer Analysis Report
===========================================

File: video.mp4

Buffer Configuration:
  Buffer Size: 3125000 bytes (25.00 Mbits)
  Target Bitrate: 2.11 Mbps

Buffer Occupancy Statistics:
  Maximum: 3125000 bytes (100.0%)
  Minimum: 0 bytes (0.2%)
  Average: 1982680 bytes (63.4%)

Bitrate Statistics:
  Peak: 40.13 Mbps
  Average: 1.44 Mbps
  Minimum: 0.02 Mbps

Buffer Events:
  Overflows: 131
  Underflows: 1
  Near Overflows (>90%): 79
  Near Underflows (<10%): 42

Buffering Delays:
  Maximum: 11.826 seconds
  Average: 7.503 seconds

-------------------------------------------
Compliance Assessment:
-------------------------------------------

[ERROR] Buffer overflow detected (131 times)
  This violates HRD/VBV constraints and may cause decoder failures.

[ERROR] Buffer underflow detected (1 times)
  This may cause decoder stalls or playback interruptions.
```

**Exit codes:**
- `0`: No buffer violations (PASS)
- `1`: Buffer violations detected (FAIL)

**Use cases:**
- Verify encoder buffer configuration
- Debug playback stalls or decoder failures
- Validate streaming delivery constraints
- Optimize buffer size for target bitrate

## Use Cases

### Batch Processing

Process multiple videos:

```bash
for video in *.mp4; do
    echo "Analyzing $video"
    videostudio-cli info "$video" --format json > "${video%.mp4}_info.json"
    videostudio-cli frames "$video" -o "${video%.mp4}_frames.csv"
    videostudio-cli gop "$video" -o "${video%.mp4}_gop.json"
done
```

### CI/CD Integration

Validate video encoding in continuous integration:

```bash
#!/bin/bash
# Check if encoded video meets requirements

INFO=$(videostudio-cli info encoded.mp4 --format json)

WIDTH=$(echo $INFO | jq -r '.width')
HEIGHT=$(echo $INFO | jq -r '.height')
CODEC=$(echo $INFO | jq -r '.codec')

if [ "$WIDTH" != "1920" ] || [ "$HEIGHT" != "1080" ]; then
    echo "Error: Invalid resolution"
    exit 1
fi

if [[ "$CODEC" != *"H.264"* ]]; then
    echo "Error: Wrong codec"
    exit 1
fi

# Validate compliance
videostudio-cli compliance encoded.mp4 -o compliance.json
if [ $? -ne 0 ]; then
    echo "Error: Compliance validation failed"
    cat compliance.json
    exit 1
fi

echo "Video validation passed"
```

### Compliance Validation in Production

Ensure encoded videos meet broadcast standards:

```bash
#!/bin/bash
# Validate video before delivery

VIDEO=$1

echo "Validating $VIDEO..."

# Run compliance check
videostudio-cli compliance "$VIDEO" -o "${VIDEO%.mp4}_compliance.json"
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    echo "❌ COMPLIANCE FAILED"
    echo "Critical issues found. Video does not meet delivery standards."
    exit 1
fi

# Check for warnings
WARNINGS=$(jq -r '.warnings' "${VIDEO%.mp4}_compliance.json")
if [ "$WARNINGS" -gt 0 ]; then
    echo "⚠️  WARNING: $WARNINGS issues found"
    echo "Review the compliance report before delivery."
fi

echo "✅ PASSED: Video meets compliance standards"
```

### Quality Analysis Pipeline

Combine with other tools:

```bash
# Export frames and calculate statistics
videostudio-cli frames video.mp4 -o frames.csv

# Calculate average bitrate by frame type
awk -F',' 'NR>1 && $2=="I" {sum+=$3; count++} END {print "Avg I-frame size:", sum/count}' frames.csv
awk -F',' 'NR>1 && $2=="P" {sum+=$3; count++} END {print "Avg P-frame size:", sum/count}' frames.csv
awk -F',' 'NR>1 && $2=="B" {sum+=$3; count++} END {print "Avg B-frame size:", sum/count}' frames.csv
```

### GOP Structure Validation

Check GOP structure compliance:

```bash
# Verify GOP length doesn't exceed maximum
videostudio-cli gop video.mp4 | jq '.gops[] | select(.length > 250) | {gop: .gop_number, length: .length}'

# Check keyframe interval
videostudio-cli gop video.mp4 | jq '.gops[] | .length' | awk '{sum+=$1; count++} END {print "Average GOP length:", sum/count}'
```

## Output Formats

### JSON
- Structured data for programmatic processing
- Compatible with `jq`, Python, Node.js, etc.
- Ideal for automation and integration

### CSV
- Tabular data for spreadsheet applications
- Easy to import into Excel, pandas, R
- Good for statistical analysis

### Text
- Human-readable format
- Quick information viewing
- Suitable for terminal output

## Performance

- Fast frame indexing (typically < 1 second for 10-second video)
- Low memory footprint
- No GUI overhead
- Suitable for server environments

## Requirements

- Qt 6.11.0 (Core module only)
- FFmpeg 7.1.1 or later
- macOS 10.15+ / Linux

## Limitations

- Quality metrics (PSNR/SSIM/VMAF) require the GUI application
- Transport stream validation not yet implemented
- No batch processing built-in (use shell scripts)

## Examples

See the `examples/` directory for sample scripts:
- `batch_analyze.sh`: Process multiple videos
- `validate_encoding.sh`: CI/CD validation script
- `gop_stats.py`: Python script for GOP analysis

## Contributing

Contributions are welcome! Areas for improvement:
- Batch processing mode
- Quality metrics calculation
- Transport stream validation
- Progress indicators
- More output formats (XML, YAML)

## License

MIT License - see LICENSE file for details.
