# MIDI Testing with libmtcmaster

## Overview

The MIDI implementation can be tested using `libmtcmaster` for MTC (MIDI Time Code) generation and reception. This allows testing the `ALSASeqMIDIDriver` implementation without requiring external MIDI hardware or software.

## Prerequisites

1. **Install libmtcmaster**:
   ```bash
   # Debian/Ubuntu
   sudo apt-get install libmtcmaster-dev
   
   # Or build from source if not available in package manager
   ```

2. **Build with MIDI testing enabled**:
   ```bash
   cd build
   cmake .. -DENABLE_MIDI=ON -DENABLE_MIDI_TEST=ON -DENABLE_ALSAMIDI=ON
   make
   ```

## Running Tests

### Unit Test: MTCDecoder

Test the MTC decoder directly without hardware:
```bash
./xjadeo_midi_test --skip-mtc
```

This tests the `MTCDecoder` class with known MTC quarter-frame messages.

### Integration Test: MTC Reception

Test full MTC reception using libmtcmaster:
```bash
./xjadeo_midi_test
```

This test:
1. Starts libmtcmaster to generate MTC
2. Opens `ALSASeqMIDIDriver` to receive MTC
3. Compares generated vs received frames
4. Reports any discrepancies

**Note**: You may need to connect ALSA sequencer ports manually:
```bash
# List available ports
aconnect -l

# Connect libmtcmaster output to xjadeo input
aconnect <mtcmaster_port> <xjadeo_port>
```

## Test Configuration

The test can be configured by editing `TestMIDI.cpp`:
- `TEST_FRAMERATE`: Framerate for MTC generation (default: 25.0 fps)
- `TEST_DURATION_SEC`: Test duration in seconds (default: 10)
- `TEST_START_FRAME`: Starting frame number (default: 0)

## Expected Output

Successful test output:
```
=== Testing MTC Reception ===
Opening ALSA MIDI driver...
Starting MTC generation on port: 128:0
MTC generation started on port: 128:0
Framerate: 25 fps, Start frame: 0
Testing MTC reception for 10 seconds...
      Time       Generated       Received            Diff
         0s              0              0              0
         1s            25             25              0
         2s            50             50              0
...

=== Test Results ===
Frames received: 250
Errors: 0
Test PASSED
```

## Troubleshooting

### libmtcmaster not found
- Install libmtcmaster development package
- Verify with: `pkg-config --exists libmtcmaster`

### No frames received
- Check ALSA sequencer connection: `aconnect -l`
- Connect ports manually if needed
- Verify MIDI driver is connected: `driver->isConnected()`

### Frame errors
- Small differences (< 2 frames) are normal due to timing
- Large differences indicate synchronization issues
- Check framerate settings match between generator and receiver

## Manual Testing

If libmtcmaster is not available, you can test manually:

1. **Use external MTC source**:
   - Connect hardware MTC generator
   - Or use another software MTC generator
   - Connect to ALSA sequencer port

2. **Run xjadeo with MIDI**:
   ```bash
   ./xjadeo --midi <port> <video_file>
   ```

3. **Monitor frame reception**:
   - Check OSD for current frame
   - Verify frame updates match MTC source
   - Use verbose mode: `--verbose` or `--midi-clkadj`

## API Notes

**libmtcmaster** appears to be a C++ library with Python bindings. The library is installed at `/usr/local/lib/libmtcmaster.so.0`.

### Using Python Interface (Recommended)

Since libmtcmaster has a Python interface, you can use `TestMIDI_Python.cpp` which uses Python C API to call the Python bindings. This is likely more reliable than trying to use a C++ library from C.

### Using C/C++ Directly

If libmtcmaster provides a C API or C++ headers, you can use `TestMIDI.cpp`. However, you may need to:
- Find the header file location
- Adjust function names and signatures based on actual API
- Link against the library correctly

### Finding the API

To find the actual API:
```bash
# Check for header files
find /usr/local/include /usr/include -name "*mtc*"

# Check exported symbols
nm -D /usr/local/lib/libmtcmaster.so.0 | grep mtc

# Check Python interface
python3 -c "import mtcmaster; help(mtcmaster.MtcMaster)"
```

### Recommended Approach

1. **First try**: Use Python interface (`TestMIDI_Python.cpp`) - most reliable
2. **Alternative**: If C/C++ API is documented, update `TestMIDI.cpp` with correct API

