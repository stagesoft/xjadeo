# C++ Migration Status

This document tracks the migration of xjadeo from C to C++.

## Migration Strategy

The migration follows a gradual approach:
- New C++ code lives in `src/xjadeo/cpp/`
- C and C++ code coexist during migration
- C++ code uses `CLegacyBridge.h` to access C globals/functions (being phased out)
- Final goal: Full C++ implementation with C code removed

## Migration Progress Summary

### Files Removed from Build (✅ Completed)
- ✅ `main.c` - Replaced by `cpp/main.cpp`
- ✅ `configfile.c` - Replaced by `ConfigurationManager`
- ✅ `remote.c` - Replaced by `RemoteControl`/`OSCRemoteControl`
- ✅ `xjosc.c` - Replaced by `OSCRemoteControl`
- ✅ `gtime.c` - Replaced by `cpp/utils/TimeUtils.cpp`
- ✅ `smpte.c` - Replaced by `cpp/utils/SMPTEUtils.cpp` + `SMPTEWrapper.cpp`

### Major Components Migrated (✅ Completed)
- ✅ **Application Core**: `XjadeoApplication`, `ConfigurationManager`, `Logger`
- ✅ **Input/Output**: `VideoFileInput`, `FrameBuffer`, `FrameFormat`
- ✅ **Layer System**: `VideoLayer`, `LayerManager`, `LayerProperties`
- ✅ **Display/Rendering**: `OpenGLDisplay`, `OpenGLRenderer`, `DisplayManager`, `XineramaHelper`
- ✅ **Remote Control**: `OSCRemoteControl`, `RemoteCommandRouter`
- ✅ **OSD**: `OSDManager`, `OSDRenderer`
- ✅ **Sync Sources**: `MIDISyncSource`, `MTCDecoder`, `MIDIDriver` (ALSA Sequencer only)
- ✅ **Utilities**: `SMPTEUtils` (SMPTE timecode), `SMPTEWrapper` (C-compatible wrapper), `TimeUtils` (monotonic time)
- ✅ **Testing**: Unit and integration test framework

### Remaining C Code (⏳ In Progress)
- ⏳ `xjadeo.c` - Legacy video playback logic (mostly replaced by C++ classes, kept for compatibility)
  - **Status**: Core functionality migrated to `VideoFileInput`, `XjadeoApplication`, `VideoLayer`
  - **Remaining**: Some utility functions and legacy event loop (not used by C++ codebase)
  - **Note**: This file is still in build but not actively used by `cpp/main.cpp`
- ⏳ `midi.c` - MIDI implementation (C++ version complete, C version still in build for testing)
- ✅ `smpte.c` - **MIGRATED** to `cpp/utils/SMPTEUtils.cpp` + `SMPTEWrapper.cpp` (removed from build, C code uses C++ via wrapper)
- ✅ `gtime.c` - **MIGRATED** to `cpp/utils/TimeUtils.cpp` (using std::chrono, removed from build)
- ⏳ Display backends - Platform-specific X11/GLX code (Linux only) - **KEEP IN C** per hybrid migration strategy
  - **Rationale**: Platform APIs (X11, GLX) are C-only, performance-critical rendering path
  - **Implementation**: C++ `OpenGLDisplay` class wraps C display code for management, rendering stays in C

## C++ Components (Completed)

### Core Application
- ✅ `XjadeoApplication` - Main application orchestrator
- ✅ `ConfigurationManager` - Configuration handling
- ✅ `Logger` - Logging system

### Input & Sync
- ✅ `InputSource` - Abstract input interface
- ✅ `VideoFileInput` - FFmpeg-based video file input
- ✅ `SyncSource` - Abstract sync interface
- ✅ `MIDISyncSource` - MIDI MTC synchronization
- ❌ `JACKSyncSource` - **REMOVED** (JACK support removed)

### Video Processing
- ✅ `FrameBuffer` - Video frame buffer management
- ✅ `FrameFormat` - Frame format definitions

### Layer Management
- ✅ `VideoLayer` - Individual video layer
- ✅ `LayerManager` - Multi-layer management
- ✅ `LayerProperties` - Layer display properties

### Display & Rendering
- ✅ `DisplayBackend` - Abstract display interface
- ✅ `OpenGLDisplay` - OpenGL display backend (Linux GLX only)
- ✅ `OpenGLRenderer` - OpenGL rendering operations
- ✅ `DisplayManager` - Multi-display management
- ✅ `XineramaHelper` - Xinerama multi-display helper
- ❌ Windows (WGL) - Will not be implemented
- ❌ macOS (CGL) - Will not be implemented

### Remote Control
- ✅ `RemoteControl` - Abstract remote control interface
- ✅ `OSCRemoteControl` - OSC remote control implementation
- ✅ `RemoteCommandRouter` - Command routing system

### OSD (On-Screen Display)
- ✅ `OSDManager` - OSD state management
- ✅ `OSDRenderer` - Freetype text rendering

### Testing
- ✅ `TestFramework` - Unit test framework
- ✅ Unit tests for core components
- ✅ Integration tests

## C Code Still in Use

### Core Functionality
- `main.c` - ✅ **REMOVED** from build (replaced by `cpp/main.cpp`)
- `xjadeo.c` - Core video playback logic (partially replaced, **not used by C++ codebase**)
- `common.c` - Common utilities (legacy UI functions, **not used by C++ codebase**)
- `configfile.c` - ✅ **REMOVED** from build (replaced by `ConfigurationManager`)

### Display Backends
- `display.c` - Display abstraction (replaced by `DisplayBackend`)
- `display_x11.c` - X11 display (Linux only)
- `display_glx.c` - GLX OpenGL context (Linux only, **KEEP IN C** per hybrid strategy)
- `display_x_dnd.c` - X11 drag-and-drop (Linux only)
- `display_x_dialog.c` - X11 dialogs (Linux only)
- `display_mac.c` - macOS display (not being migrated)
- `display_sdl.c` - SDL display (not being migrated)

### Remote Control
- `remote.c` - ✅ **REMOVED** from build (replaced by `RemoteControl`/`OSCRemoteControl`)
- `xjosc.c` - ✅ **REMOVED** from build (replaced by `OSCRemoteControl`)
- `mqueue.c` - Message queue remote control (not yet migrated)

### Sync Sources
- `midi.c` - ✅ **C++ version complete** (C code still in build but unused by C++ codebase)
- `ltc-jack.c` - LTC sync (JACK removed, **not used by C++ codebase**, likely safe to remove)

### Utilities
- `freetype.c` - Freetype utilities (C++ `OSDRenderer` uses Freetype C API directly - **KEEP IN C**)
- `smpte.c` - ✅ **MIGRATED** to `SMPTEUtils.cpp` + `SMPTEWrapper.cpp` (removed from build)
- `gtime.c` - ✅ **MIGRATED** to `TimeUtils.cpp` (removed from build)
- `libsofd.c` - OSD font data (embedded font resources)

## C Functions Accessed via CLegacyBridge

The following C functions/globals are still accessed by C++ code:

### Video Information (from `xjadeo.c`)
- `movie_width`, `movie_height` - Video dimensions
- `movie_aspect` - Aspect ratio
- `framerate` - Video framerate
- `frames` - Total frame count
- `have_dropframes` - Drop frame flag

### Configuration Flags
- `want_quiet` - Quiet mode flag
- `want_verbose` - Verbose mode flag
- `want_debug` - Debug mode flag

### MIDI Functions (from `midi.c`) - ✅ **MIGRATED TO C++**
- `midi_connected()` - Check MIDI connection (use `MIDISyncSource` instead)
- `midi_poll_frame()` - Poll MIDI for frame (use `MIDISyncSource` instead)
- `midi_open()` - Open MIDI connection (use `MIDISyncSource` instead)
- `midi_close()` - Close MIDI connection (use `MIDISyncSource` instead)
- `midi_driver_name()` - Get MIDI driver name (use `MIDISyncSource` instead)

## Migration Priorities

### High Priority (Core Functionality)
1. **MIDI Sync** - ✅ **COMPLETE** - Migrated `midi.c` to pure C++ `MIDISyncSource` (ALSA Sequencer only)
   - ✅ Core architecture complete (MTCDecoder, MIDIDriver, MIDISyncSource)
   - ✅ ALSA Sequencer driver implementation complete
   - ✅ Removed C global dependencies (midi_clkadj, delay, waare 
2. **Main Entry Point** - ✅ **COMPLETE** - Migration from `main.c` to `cpp/main.cpp`
   - ✅ `main.c` removed from build
   - ✅ Using `cpp/main.cpp` as entry point
3. **Configuration** - ✅ **COMPLETE** - Migration from `configfile.c` to `ConfigurationManager`
   - ✅ `configfile.c` removed from build
   - ✅ All configuration handled through C++ `ConfigurationManager`
4. **Remote Control** - ✅ **COMPLETE** - Migration from `remote.c`/`xjosc.c` to `OSCRemoteControl`
   - ✅ `remote.c` and `xjosc.c` removed from build
   - ✅ All remote control handled through C++ `OSCRemoteControl` and `RemoteCommandRouter`
5. **Video Playback Core** - ✅ **SUBSTANTIALLY COMPLETE**
   - ✅ Core video file handling: `VideoFileInput` class
   - ✅ Frame decoding and seeking: Implemented in `VideoFileInput`
   - ✅ Event loop: `XjadeoApplication::run()`
   - ✅ Layer management: `LayerManager` and `VideoLayer`
   - ⏳ Legacy `xjadeo.c` still in build but not used by C++ codebase
   - **Note**: Remaining `xjadeo.c` code is legacy and can be removed once fully tested

### Medium Priority (Utilities)
6. **SMPTE Utilities** - ✅ **MIGRATED** to `cpp/utils/SMPTEUtils.cpp` + `SMPTEWrapper.cpp` (C code uses C++ via wrapper, `smpte.c` removed from build)
7. **Time Utilities** - ✅ **MIGRATED** to `cpp/utils/TimeUtils.cpp` (using std::chrono, C code removed from build)
8. **Freetype Utilities** - Complete migration to `OSDRenderer`

### Low Priority (Platform-Specific)
9. **Display Backends** - Complete Linux GLX migration (Windows/macOS not planned)
10. **Legacy Remote Control** - ✅ Already removed (`remote.c` and `xjosc.c` removed from build)

## Deprecated Code Paths

### Removed
- ❌ JACK support - Completely removed
  - `JACKSyncSource` deleted
  - `jack.c`, `common_jack.c`, `weak_libjack.c` removed from build
  - JACK dependencies removed from CMakeLists.txt

### To Be Removed
- `main.c` - ✅ **REMOVED** from build (using `cpp/main.cpp` instead)
- `configfile.c` - ✅ **REMOVED** from build (replaced by `ConfigurationManager`)
- `remote.c` - ✅ **REMOVED** from build (replaced by `RemoteControl`/`OSCRemoteControl`)
- `xjosc.c` - ✅ **REMOVED** from build (replaced by `OSCRemoteControl`)

## Build System

- ✅ CMake build system configured
- ✅ Both C and C++ sources compiled
- ✅ Test suite integrated
- ✅ JACK dependencies removed

## Notes

- The C++ codebase is fully functional and can operate independently
- **Migration Strategy**: Following **Hybrid Approach with Strategic Migration** (see `MIGRATION_STRATEGY_ANALYSIS.md`)
- C code retention strategy:
  - **Keep C for**: Platform APIs (X11/GLX, ALSA), FFmpeg integration, performance-critical paths
  - **Migrate to C++**: Business logic, state management, utilities
- **Implementation**: C++ classes wrap C APIs (FFmpeg, ALSA, X11/GLX) while keeping the underlying C code for performance and compatibility (SMPTE, time)
- C code is currently used for:
  - Platform-specific display backends (Linux X11/GLX only - Windows/macOS not supported) - **KEEP IN C**
  - MIDI implementation (C++ version complete, C code still in build but unused)
  - Freetype font loading (C++ `OSDRenderer` uses Freetype C API directly) - **KEEP IN C**
- The architecture is designed to allow gradual migration without breaking functionality
- **Platform Support**: Only Linux is supported. Windows and macOS implementations are not planned.

## Library Strategy

### C Libraries (Keep - No Alternatives)
- **FFmpeg**: Industry standard, no C++ alternative → Keep C interface
- **ALSA**: Linux standard, no alternative → Keep C interface, wrap in C++
- **X11/GLX**: Platform standard, required → Keep C interface, wrap in C++
- **OpenGL**: Industry standard graphics API → Keep C API, use C++ for management
- **Freetype**: Industry standard font rendering → Keep C interface, wrap in C++
- **liblo**: OSC library → Keep (mature, lightweight, works well with C++)

### C++ Standard Library Usage
- ✅ **std::chrono**: Replace `gtime.c` time functions
- ✅ **std::thread**: Already using for threading
- ✅ **std::mutex**: Already using for synchronization
- ✅ **std::string/vector/map**: Already using throughout
- ✅ **Smart pointers**: Already using for memory management

## MIDI Migration Status

### Completed
- ✅ **MTCDecoder** - Pure C++ MTC (MIDI Time Code) decoder
- ✅ **MIDIDriver** - Abstract base class for MIDI driver implementations
- ✅ **MIDIDriverFactory** - Factory for creating MIDI drivers
- ✅ **NullMIDIDriver** - Null/dummy driver implementation
- ✅ **MIDISyncSource** - Updated to use pure C++ implementation (no longer depends on C `midi.c`)
- ✅ **ALSASeqMIDIDriver** - ALSA Sequencer driver implementation (fully ported from C)

### Not Planned
- ❌ PortMidi driver - Will not be implemented
- ❌ ALSA Raw MIDI driver - Will not be implemented
- ❌ JACK-MIDI driver - JACK support removed

### Notes
- ✅ ALSA Sequencer MIDI driver ported from C to C++ - **COMPLETE**
- ⏳ `midi.c` still in build but not used by C++ codebase (can be removed after testing)
- ⚠️ MIDI functions in `CLegacyBridge.h` are marked as deprecated (for C code compatibility only)
- ✅ C++ code uses `MIDISyncSource` which is pure C++ implementation

