# Explorer Panel Context Menu Features

## Overview

The Explorer Panel now provides professional stream analysis features through a comprehensive context menu system. Right-click on any element in the Explorer tree to access these features.

## Features by Container Format

### Transport Stream (.ts, .m2ts, .mts)

Right-click on any PID in the Explorer Panel to access:

#### 1. Dump Elementary Stream
- **Purpose**: Extract raw elementary stream data for a specific PID
- **Output**: Binary file containing all payload data from packets with the selected PID
- **File formats**: 
  - `.es` - Generic elementary stream
  - `.h264` - H.264/AVC video stream
  - `.h265` - H.265/HEVC video stream
- **Use cases**:
  - Extract video/audio streams for external analysis
  - Verify codec compliance
  - Debug stream issues
  - Feed to external decoders

**Example workflow:**
1. Open a TS file (e.g., `test_h264.ts`)
2. In Explorer Panel, expand "Transport stream" → "Program #1"
3. Right-click on a video PID (e.g., "Video (H.264)")
4. Select "Dump Elementary Stream"
5. Choose output filename (e.g., `video_stream.h264`)
6. File is saved with packet count and byte size displayed

#### 2. Set Compare Mode
- **Purpose**: Compare current packet with previous packet of same PID
- **Effect**: Property Panel switches to Compare mode
- **Display**: Differences highlighted in blue
- **Use cases**:
  - Detect continuity counter errors
  - Compare packet headers
  - Analyze timing changes (PTS/DTS/PCR)

#### 3. Set Sync Mode
- **Purpose**: Auto-update Property Panel when clicking packets
- **Effect**: Property Panel switches to Sync mode (default)
- **Display**: Shows current packet details
- **Use cases**:
  - Normal packet inspection
  - Quick navigation through stream

#### 4. Expand All / Collapse All
- **Purpose**: Quickly expand or collapse entire tree structure
- **Available**: On any item in Explorer Panel
- **Use cases**:
  - Navigate large stream structures
  - Get overview of stream hierarchy

---

### MP4 Container (.mp4, .mov, .m4v)

Right-click on any atom/box in the Explorer Panel to access:

#### 1. Dump Data to File
- **Purpose**: Extract raw atom data including header and payload
- **Output**: Binary file containing complete atom structure
- **Filename format**: `{atom_type}_0x{offset}.bin`
- **Use cases**:
  - Extract codec configuration (avcC, hvcC)
  - Analyze atom structure
  - Extract metadata
  - Debug container issues

**Example workflow:**
1. Open an MP4 file (e.g., `test_h264.mp4`)
2. In Explorer Panel, expand atoms to find target (e.g., "moov" → "trak" → "mdia" → "minf" → "stbl" → "stsd" → "avcC")
3. Right-click on the atom
4. Select "Dump Data to File"
5. Choose output filename (e.g., `avcC_0x12345.bin`)
6. File is saved with byte size displayed

**Common atoms to dump:**
- `ftyp` - File type and compatibility
- `moov` - Movie metadata
- `mdat` - Media data
- `avcC` - H.264 configuration
- `hvcC` - H.265 configuration
- `esds` - Elementary stream descriptor

---

### MKV Container (.mkv, .webm)

Right-click on any EBML element in the Explorer Panel to access:

#### 1. Dump Data to File
- **Purpose**: Extract raw EBML element data including header
- **Output**: Binary file containing complete element structure
- **Filename format**: `{element_name}_0x{offset}.bin`
- **Use cases**:
  - Extract codec private data
  - Analyze EBML structure
  - Extract attachments
  - Debug container issues

**Example workflow:**
1. Open an MKV file (e.g., `test_h264.mkv`)
2. In Explorer Panel, expand elements (e.g., "Segment" → "Tracks" → "TrackEntry")
3. Right-click on the element
4. Select "Dump Data to File"
5. Choose output filename (e.g., `TrackEntry_0x1000.bin`)
6. File is saved with byte size displayed

**Common elements to dump:**
- `EBML` - EBML header
- `Segment` - Main container
- `Info` - Segment information
- `Tracks` - Track definitions
- `Cluster` - Media data cluster
- `CodecPrivate` - Codec configuration

---

### AVI Container (.avi)

Right-click on any RIFF chunk in the Explorer Panel to access:

#### 1. Dump Data to File
- **Purpose**: Extract raw chunk data including header
- **Output**: Binary file containing complete chunk structure
- **Filename format**: `{fourCC}_0x{offset}.bin`
- **Use cases**:
  - Extract stream headers
  - Analyze chunk structure
  - Extract index data
  - Debug container issues

**Example workflow:**
1. Open an AVI file (e.g., `test_h264.avi`)
2. In Explorer Panel, expand chunks (e.g., "RIFF (AVI)" → "LIST (hdrl)" → "avih")
3. Right-click on the chunk
4. Select "Dump Data to File"
5. Choose output filename (e.g., `avih_0x20.bin`)
6. File is saved with byte size displayed

**Common chunks to dump:**
- `RIFF` - Main container
- `avih` - AVI header
- `strh` - Stream header
- `strf` - Stream format
- `movi` - Movie data
- `idx1` - Index

---

### FLV Container (.flv)

Right-click on any FLV tag in the Explorer Panel to access:

#### 1. Dump Data to File
- **Purpose**: Extract raw tag data including header
- **Output**: Binary file containing complete tag structure
- **Filename format**: `flv_{tag_type}_0x{offset}.bin`
- **Use cases**:
  - Extract video/audio frames
  - Analyze tag structure
  - Extract metadata (onMetaData)
  - Debug container issues

**Example workflow:**
1. Open an FLV file (e.g., `test_h264_aac.flv`)
2. In Explorer Panel, expand tags (e.g., "Video Tags" → "Video Tag #0")
3. Right-click on the tag
4. Select "Dump Data to File"
5. Choose output filename (e.g., `flv_video_0x0D.bin`)
6. File is saved with byte size displayed

**Tag types:**
- `video` - Video frame data
- `audio` - Audio frame data
- `script` - Script data (metadata)

---

## Technical Details

### File Format Support

| Format | Extension | Dump Unit | Output Format |
|--------|-----------|-----------|---------------|
| Transport Stream | .ts, .m2ts, .mts | Elementary Stream (PID) | Raw payload data |
| MP4 | .mp4, .mov, .m4v | Atom/Box | Complete atom with header |
| MKV | .mkv, .webm | EBML Element | Complete element with header |
| AVI | .avi | RIFF Chunk | Complete chunk with header |
| FLV | .flv | FLV Tag | Complete tag with header |

### Implementation Details

**Transport Stream Dump:**
- Iterates through all packets
- Filters by selected PID
- Extracts payload data only (no TS headers)
- Concatenates payloads in stream order
- Reports packet count and total bytes

**Container Format Dump:**
- Seeks to element offset in source file
- Reads complete element (header + data)
- Writes to destination file
- Reports total bytes dumped

### Error Handling

All dump operations include:
- File open error checking
- Write permission verification
- Source file accessibility validation
- User-friendly error messages
- Completion confirmation with statistics

---

## Usage Tips

### For H.264/H.265 Analysis

1. **Extract video elementary stream from TS:**
   - Right-click video PID → "Dump Elementary Stream"
   - Save as `.h264` or `.h265`
   - Use with external tools: `ffmpeg`, `MediaInfo`, `x264/x265 encoders`

2. **Extract codec configuration from MP4:**
   - Navigate to `moov → trak → mdia → minf → stbl → stsd → avcC` (H.264)
   - Or `moov → trak → mdia → minf → stbl → stsd → hvcC` (H.265)
   - Right-click → "Dump Data to File"
   - Analyze SPS/PPS/VPS parameters

3. **Extract codec private data from MKV:**
   - Navigate to `Segment → Tracks → TrackEntry → CodecPrivate`
   - Right-click → "Dump Data to File"
   - Contains codec initialization data

### For Audio Analysis

1. **Extract audio elementary stream from TS:**
   - Right-click audio PID → "Dump Elementary Stream"
   - Save as `.aac`, `.mp3`, or `.ac3`
   - Use with audio analysis tools

2. **Extract audio configuration from MP4:**
   - Navigate to audio track's `stsd` atom
   - Look for `esds` (AAC) or codec-specific atoms
   - Dump for detailed audio configuration

### For Metadata Analysis

1. **Extract FLV metadata:**
   - Right-click on Script Data Tag (usually first tag)
   - Dump to analyze onMetaData structure
   - Contains duration, dimensions, bitrates, etc.

2. **Extract MP4 metadata:**
   - Dump `moov` atom for complete movie metadata
   - Dump `udta` (user data) for custom metadata

---

## Keyboard Shortcuts

While context menu is open:
- **Enter** - Execute selected action
- **Esc** - Close menu
- **Arrow keys** - Navigate menu items

---

## Future Enhancements

Planned features:
- [ ] Batch dump multiple PIDs/elements
- [ ] Dump with range selection (offset start/end)
- [ ] Automatic codec detection and file extension
- [ ] Preview before dump (first N bytes)
- [ ] Dump statistics (bitrate, frame count)
- [ ] Export to multiple formats simultaneously
- [ ] Integration with external tools (ffmpeg, MediaInfo)

---

## Troubleshooting

**Problem**: "Failed to open file for writing"
- **Solution**: Check write permissions in target directory
- **Solution**: Ensure filename is valid (no special characters)

**Problem**: "Element not found"
- **Solution**: Refresh Explorer Panel (reopen file)
- **Solution**: Ensure element is fully parsed

**Problem**: Dumped file is empty or corrupted
- **Solution**: Verify source file is not corrupted
- **Solution**: Check available disk space
- **Solution**: Try different output location

**Problem**: Context menu doesn't appear
- **Solution**: Ensure you're right-clicking on a valid element (not header)
- **Solution**: Check that file is fully loaded

---

## Related Features

- **Property Panel**: View detailed element properties before dumping
- **Hex Viewer Panel**: Inspect raw bytes before dumping
- **Messages Panel**: Check for stream errors before extraction
- **TR 101-290 Panel**: Verify TS compliance before dumping

---

## References

- MPEG-TS: ISO/IEC 13818-1
- MP4: ISO/IEC 14496-12, 14496-14, 14496-15
- MKV: Matroska specification
- AVI: Microsoft RIFF AVI specification
- FLV: Adobe Flash Video File Format Specification

---

*Last updated: 2026-05-31*
*VideoStudio version: 1.0*
