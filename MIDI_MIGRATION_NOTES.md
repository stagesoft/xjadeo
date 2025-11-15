# MIDI Migration Notes

## Status

The MIDI sync code has been migrated from C to C++ with a new architecture:

### Completed
- ✅ **MTCDecoder** - Pure C++ MTC (MIDI Time Code) decoder
- ✅ **MIDIDriver** - Abstract base class for MIDI driver implementations
- ✅ **MIDIDriverFactory** - Factory for creating MIDI drivers
- ✅ **NullMIDIDriver** - Null/dummy driver implementation
- ✅ **MIDISyncSource** - Updated to use pure C++ implementation (no longer depends on C `midi.c`)

### Architecture

The new C++ implementation uses:
- **MTCDecoder**: Parses MTC quarter-frame messages and converts to frame numbers
- **MIDIDriver**: Abstract interface for different MIDI backends
- **MIDIDriverFactory**: Creates appropriate driver instances

### Remaining Work

**ALSA Sequencer Driver** (ONLY driver being ported)
- Port `as_midi_*` functions from `midi.c`:
  - `as_midi_open()` - Open ALSA sequencer connection
  - `as_midi_close()` - Close ALSA sequencer connection
  - `as_midi_connected()` - Check connection status
  - `as_midi_poll_frame()` - Poll for MIDI messages and return frame
  - `as_midi_detectdevices()` - Detect available MIDI devices
- Requires ALSA sequencer library (`libasound2-dev` on Debian/Ubuntu)
- Create `ALSASeqMIDIDriver` class implementing `MIDIDriver` interface

### Drivers NOT Being Implemented

- ❌ **PortMidi Driver** - Will not be ported
- ❌ **ALSA Raw MIDI Driver** - Will not be ported
- ❌ **JACK-MIDI Driver** - JACK support removed

### Current State

- The C++ code compiles and works with the null driver
- `MIDISyncSource` no longer calls C functions directly
- The C `midi.c` file is still compiled but not used by C++ code
- Once ALSA Sequencer driver is complete, `midi.c` can be removed

### Next Steps

1. ✅ Implement `ALSASeqMIDIDriver` class - **COMPLETED**
2. ✅ Update `MIDIDriverFactory` to create ALSA Sequencer driver - **COMPLETED**
3. ⏳ Test ALSA Sequencer driver with real MIDI hardware
4. ⏳ Remove `midi.c` from build once verified working
5. ⏳ Remove MIDI functions from `CLegacyBridge.h`

### Driver Implementation Pattern

Each driver should:
1. Inherit from `MIDIDriver`
2. Implement `open()`, `close()`, `isConnected()`, `pollFrame()`, `getName()`, `isSupported()`
3. In `pollFrame()`, read MIDI messages and feed bytes to `MTCDecoder`
4. Use `MTCDecoder::processByte()` to parse MTC messages
5. Use `MTCDecoder::timecodeToFrame()` to convert to frame numbers

### MTC Message Format

MTC (MIDI Time Code) uses quarter-frame messages:
- Status byte: `0xF1` (MTC Quarter Frame)
- Data byte: Upper nibble (0-7) = quarter-frame number, Lower nibble = data
- 8 quarter-frames make one complete timecode
- Each quarter-frame contains part of: frame LSN, frame MSN, sec LSN, sec MSN, min LSN, min MSN, hour LSN, hour MSN+type

