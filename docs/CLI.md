# VideoStudio CLI

Command-line interface for VideoStudio - Professional video stream analysis tool.

## Features

- **Video Information Extraction**: Get codec, resolution, frame rate, duration, and bitrate
- **Frame-Level Metrics**: Export CSV data with frame type, size, PTS, DTS, QP values
- **GOP Structure Analysis**: Analyze Group of Pictures structure with JSON output
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

echo "Video validation passed"
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
