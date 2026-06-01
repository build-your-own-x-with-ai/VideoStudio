# Context Menu Dump Functionality Test Guide

## Test Files Available

Based on the test files in `test_files/`, here are the recommended files for testing:

### ✅ Working Test Files

1. **Transport Stream (TS)**
   - `test_h264.ts` (165K) - H.264 video + audio
   - `test_h265.ts` (174K) - H.265 video + audio
   - `test_multi_program.ts` (230K) - Multiple programs

2. **MP4 Container**
   - `test_h264.mp4` - H.264 video
   - `test_h265.mp4` - H.265 video
   - `test_multi_audio.mp4` - Multiple audio tracks

3. **MKV Container**
   - `test_h264.mkv` - H.264 video
   - `test_h265.mkv` - H.265 video
   - `test_chapters.mkv` - With chapters

4. **AVI Container**
   - `test_h264.avi` - H.264 video
   - `test_mjpeg.avi` - MJPEG video
   - `test_mpeg4.avi` - MPEG-4 video

5. **FLV Container**
   - `test_h264_aac.flv` - H.264 + AAC
   - `test_h264_mp3.flv` - H.264 + MP3
   - `test_metadata.flv` - With metadata

6. **Raw H.264 Stream**
   - `test_h264_bframes.h264` (58K) - Raw H.264 with B-frames

### ❌ Empty Test Files (Skip These)
- `test_h264_baseline.h264` (0 bytes)
- `test_h264_main.h264` (0 bytes)
- `test_h264_high.h264` (0 bytes)

---

## Test Procedure

### Test 1: Dump Elementary Stream from TS File

**File**: `test_h264.ts`

**Steps**:
1. Launch VideoStudio
2. Open `test_files/test_h264.ts`
3. Wait for parsing to complete
4. In Explorer Panel, expand "Transport stream" → "Program #1"
5. Find the Video PID (should show "Video (H.264)" or similar)
6. **Right-click** on the Video PID
7. Select **"Dump Elementary Stream"**
8. In the save dialog, choose location and filename (e.g., `video_stream.h264`)
9. Click Save

**Expected Result**:
- File is saved successfully
- Message box shows: "Dumped X packets (Y bytes) to: [filename]"
- File size should be > 0 bytes
- File can be played with: `ffplay video_stream.h264`

**Verification**:
```bash
# Check file was created
ls -lh video_stream.h264

# Verify it's valid H.264
ffprobe video_stream.h264

# Play it
ffplay video_stream.h264
```

---

### Test 2: Dump MP4 Atom (H.264 Configuration)

**File**: `test_h264.mp4`

**Steps**:
1. Open `test_files/test_h264.mp4`
2. In Explorer Panel, expand the atom tree:
   - `moov` → `trak` → `mdia` → `minf` → `stbl` → `stsd`
3. Look for `avc1` or `avcC` atom (H.264 configuration)
4. **Right-click** on `avcC` atom
5. Select **"Dump Data to File"**
6. Save as `avcC_config.bin`

**Expected Result**:
- File is saved successfully
- Message box shows: "Dumped X bytes to: avcC_config.bin"
- File contains H.264 SPS/PPS configuration

**Verification**:
```bash
# Check file was created
ls -lh avcC_config.bin

# View hex dump (should start with 01 for avcC version)
hexdump -C avcC_config.bin | head -10
```

---

### Test 3: Dump MKV Element (Codec Private Data)

**File**: `test_h264.mkv`

**Steps**:
1. Open `test_files/test_h264.mkv`
2. In Explorer Panel, expand:
   - `Segment` → `Tracks` → `TrackEntry #1`
3. Look for `CodecPrivate` element
4. **Right-click** on `CodecPrivate`
5. Select **"Dump Data to File"**
6. Save as `codec_private.bin`

**Expected Result**:
- File is saved successfully
- Contains H.264 codec initialization data

---

### Test 4: Dump AVI Chunk (Stream Header)

**File**: `test_h264.avi`

**Steps**:
1. Open `test_files/test_h264.avi`
2. In Explorer Panel, expand:
   - `RIFF (AVI)` → `LIST (hdrl)` → `avih`
3. **Right-click** on `avih` chunk
4. Select **"Dump Data to File"**
5. Save as `avih_header.bin`

**Expected Result**:
- File is saved successfully
- Contains AVI main header structure

---

### Test 5: Dump FLV Tag (Video Frame)

**File**: `test_h264_aac.flv`

**Steps**:
1. Open `test_files/test_h264_aac.flv`
2. In Explorer Panel, expand:
   - `Video Tags` → `Video Tag #0` (or any video tag)
3. **Right-click** on the video tag
4. Select **"Dump Data to File"**
5. Save as `flv_video_frame.bin`

**Expected Result**:
- File is saved successfully
- Contains FLV video tag data

---

### Test 6: Compare Mode (TS Only)

**File**: `test_h264.ts`

**Steps**:
1. Open `test_files/test_h264.ts`
2. In Explorer Panel, **right-click** on any Video PID
3. Select **"Set Compare Mode"**
4. In Packet View (Main Window), click on a packet
5. Property Panel should show "Compare with #X"
6. Differences should be highlighted in blue

**Expected Result**:
- Property Panel switches to Compare mode
- Toolbar shows "Compare" button as active
- Packet differences are highlighted

---

### Test 7: Sync Mode (TS Only)

**File**: `test_h264.ts`

**Steps**:
1. Open `test_files/test_h264.ts`
2. In Explorer Panel, **right-click** on any Video PID
3. Select **"Set Sync Mode"**
4. In Packet View, click on different packets
5. Property Panel should auto-update with each click

**Expected Result**:
- Property Panel switches to Sync mode
- Toolbar shows "Sync" button as active
- Property Panel updates automatically

---

### Test 8: Expand/Collapse All

**File**: Any file

**Steps**:
1. Open any file with hierarchical structure
2. **Right-click** anywhere in Explorer Panel
3. Select **"Expand All"**
4. Verify entire tree expands
5. **Right-click** again
6. Select **"Collapse All"**
7. Verify entire tree collapses

**Expected Result**:
- Tree expands/collapses completely
- Works on all container formats

---

## Advanced Tests

### Test 9: Extract H.265 Stream from TS

**File**: `test_h265.ts`

**Steps**:
1. Open `test_files/test_h265.ts`
2. Find H.265 Video PID in Explorer
3. Right-click → "Dump Elementary Stream"
4. Save as `video_stream.h265`
5. Verify with: `ffprobe video_stream.h265`

---

### Test 10: Extract Multiple PIDs

**File**: `test_multi_program.ts`

**Steps**:
1. Open `test_files/test_multi_program.ts`
2. Dump video PID from Program #1
3. Dump video PID from Program #2
4. Compare the two extracted streams

---

## Troubleshooting

### Issue: Context menu doesn't appear
- **Solution**: Make sure you're right-clicking on an element with data (not a header)
- **Solution**: Ensure file is fully loaded (check status bar)

### Issue: Dump fails with "Failed to open file"
- **Solution**: Check write permissions in target directory
- **Solution**: Try saving to a different location (e.g., Desktop)

### Issue: Dumped file is empty
- **Solution**: Verify source file is not corrupted
- **Solution**: Check that element actually contains data (check size in Property Panel)

### Issue: Raw H.264 file shows "0 frames"
- **Solution**: This is expected for raw streams without proper headers
- **Solution**: Use container formats (TS, MP4, MKV) for better results
- **Solution**: Try `test_h264_bframes.h264` which has valid data

---

## Success Criteria

✅ All dump operations complete without errors
✅ Dumped files have non-zero size
✅ Dumped files can be analyzed with external tools (ffprobe, hexdump)
✅ Compare/Sync modes work correctly for TS files
✅ Expand/Collapse All works for all formats
✅ Error messages are clear and helpful

---

## Performance Notes

- **TS files**: Dump speed depends on number of packets (typically fast)
- **Large containers**: Dumping large atoms/elements may take a few seconds
- **Multiple dumps**: Can dump multiple elements without reloading file

---

## Next Steps After Testing

If all tests pass:
1. ✅ Context menu functionality is working correctly
2. ✅ Dump feature supports all container formats
3. ✅ H.264/H.265 streams can be extracted
4. ✅ Ready for production use

If tests fail:
1. Check error messages in console output
2. Verify file permissions
3. Check available disk space
4. Report issues with specific file and error message

---

*Test guide created: 2026-05-31*
*VideoStudio version: 1.0*
