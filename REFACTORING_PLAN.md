# Xjadeo C++ Refactoring Plan

## Overview
This document tracks the progress of refactoring xjadeo from C to C++, making it object-oriented and modular. The goal is to separate each component into its own class to allow for easy replacement of parts like file reading, decoding, and display.

## Migration Strategy
- **Gradual migration**: Keep C code working while building C++ alongside
- **Build system**: CMake (replacing autotools)
- **Code organization**: New C++ code in `src/xjadeo/cpp/` subfolder
- **API compatibility**: Maintain compatibility in OSC remote commands and command-line options only
- **Hybrid approach**: Keep C code for performance-critical paths and platform APIs, migrate business logic to C++
- **Library strategy**: Keep C libraries (FFmpeg, ALSA, X11, OpenGL, Freetype) - wrap in C++ classes
- **C++ standard library**: Use std::chrono, std::thread, std::mutex, smart pointers instead of C utilities where appropriate

## Architecture Approach
- **Mixed approach**: Abstract interfaces for swappable components (InputSource, SyncSource, RemoteControl, DisplayBackend)
- **Concrete classes**: For fixed components (OSDRenderer, FrameBuffer, ConfigurationManager, etc.)

## Completed Phases

### Phase 1-5: Foundation (Completed)
- CMake build system setup
- Basic class structure and interfaces
- Core component classes

### Phase 6: Multi-Layer Foundation (Completed)
- Enhanced LayerManager with z-order management
- Layer duplication and reordering
- Z-order sorted rendering
- Multi-layer OSC commands

### Phase 7: Event Loop & Core Functionality (Completed)
- Improved event loop timing with frame rate control
- Completed remote command TODOs (FPS, offset handlers)
- Enhanced frame synchronization logic in VideoLayer
- Time offset support per layer

### Phase 8: OSD Implementation (Completed)
- OSDManager class for OSD state management
- OSDRenderer class structure
- OSD integration into application
- OSC commands for OSD control (frame, smpte, text, box, font, pos)

### Phase 9: Additional Features & Integration (Completed)
- C/C++ integration bridge (CLegacyBridge.h)
- Logging system (Logger.h)
- Error handling improvements
- Configuration integration

### Phase 10: OSD Rendering Implementation (Completed)
- Full Freetype text rendering to OpenGL textures
- OSD render item system
- Integration with OpenGLRenderer
- Font management with fallbacks
- Complete OSD rendering pipeline

## Remaining Phases

### Phase 11: Time-Scaling Features (Completed)
- Per-layer time-scaling (multiply, offset, wraparound/loop)
- OSC commands for time-scaling
- Integration with sync sources
- Reverse playback support

### Phase 12: Frame Cropping / Panorama Mode (Completed)
- Per-layer frame cropping (panorama mode)
- X-offset control for panning
- OSC commands for crop/pan
- Texture coordinate-based cropping in OpenGLRenderer

### Phase 13: Additional Input Sources
- Live video input (camera capture)
- Streaming client input (network streams)
- Status: Only VideoFileInput implemented, architecture ready

### Phase 14: Platform-Specific Display Backends (Completed - Linux Only)
- ✅ Linux (GLX) OpenGL implementation - Fully implemented
- ❌ Windows (WGL) - Will not be implemented
- ❌ OSX (CGL) - Will not be implemented
- Status: Only Linux GLX OpenGL is implemented and supported

### Phase 15: Multi-Display Support
- Full Xinerama multi-display rendering
- Wayland multi-display support
- Multiple window mode (one window per display)
- Status: XineramaHelper exists but full rendering not complete

### Phase 16: Video Processing Improvements
- Complete YUV to RGB conversion (currently placeholder)
- Better frame format handling
- Color space conversion
- Status: Basic conversion exists but needs improvement

### Phase 17: Testing & Validation (Completed)
- Unit tests for core components (LayerManager, VideoLayer, ConfigurationManager)
- Integration tests
- Test framework with CMake integration
- CTest integration

### Phase 18: Additional Remote Control Protocols
- MessageQueue (MQ) remote control
- IPC remote control
- Status: Only OSC implemented, architecture ready

### Phase 19: Advanced Features
- Color correction/EQ (brightness, contrast, gamma, saturation, hue)
- Action override system
- Status: Not implemented in C++ code

### Phase 20: Code Migration & Cleanup (In Progress)
- ✅ JACK support completely removed
- ✅ CMakeLists.txt cleaned up
- ✅ Migration status document created
- ✅ Migration strategy analysis completed (see MIGRATION_STRATEGY_ANALYSIS.md)
- ⏳ Gradually replace remaining C code with C++ (strategic migration)
- ⏳ Remove deprecated code paths
- ⏳ Final integration and polish
- Status: C and C++ code coexist, JACK removed

**Migration Strategy**: Hybrid approach with strategic migration
- **Keep C for**: Platform APIs (X11, GLX, ALSA), FFmpeg integration, proven performance-critical paths
- **Migrate to C++**: Business logic, state management, utilities (SMPTE, time)
- **Rationale**: Pragmatic balance of performance, maintainability, and migration speed

## Library & Code Evaluation Strategy

### Strategy Analysis
See `MIGRATION_STRATEGY_ANALYSIS.md` for detailed pros/cons analysis of hybrid vs full C++ migration.

### C Code Retention Criteria
C code should be **kept** if:
1. **Performance Critical**: Low-level operations that benefit from C's direct memory access
2. **Library Compatibility**: Required by C-only libraries (FFmpeg, ALSA, X11, OpenGL)
3. **Platform APIs**: Direct system calls or platform-specific APIs (X11, GLX, ALSA)
4. **Mature & Stable**: Well-tested code that works reliably
5. **Minimal Dependencies**: Simple utilities with no C++ benefits

C code should be **migrated** if:
1. **Business Logic**: Application-level logic that benefits from OOP
2. **Complex State Management**: Code that would benefit from classes/RAII
3. **Error Handling**: Code that would benefit from exceptions/RAII
4. **Testability**: Code that would be easier to test with C++ features
5. **Maintainability**: Code that would be clearer with modern C++ idioms

### Recommended Approach: Hybrid with Strategic Migration

**Strategy**: Keep C code for performance/compatibility, migrate business logic to C++

**Keep C for:**
1. ✅ **Platform APIs** (X11, GLX, ALSA) - No alternative, minimal benefit from migration
2. ✅ **FFmpeg integration** - C library, direct calls are most efficient
3. ✅ **Performance-critical hot paths** - If proven faster in C

**Migrate to C++:**
1. ✅ **Business logic** (`xjadeo.c`) - Complex state, benefits from OOP
2. ✅ **Utilities** (`smpte.c`, `gtime.c`) - Better integration, not performance critical
3. ✅ **State management** - All application state in C++

**Rationale:**
- **Pragmatic**: Focus migration effort on code that benefits most
- **Performance**: Keep C where it matters, use C++ where it helps
- **Maintainability**: Migrate complex logic to C++, keep simple platform code in C
- **Risk**: Lower risk than full migration, faster to complete

**See `MIGRATION_STRATEGY_ANALYSIS.md` for detailed pros/cons analysis.**

### Library Evaluation

#### Keep C Libraries (No C++ Alternatives)
- ✅ **FFmpeg** (libavformat, libavcodec, libavutil, libswscale)
  - Industry standard for video/audio processing
  - No viable C++ alternative
  - Keep C wrapper interface, use from C++
  
- ✅ **ALSA** (asoundlib)
  - Linux standard audio/MIDI API
  - No C++ alternative
  - Keep C interface, wrap in C++ classes
  
- ✅ **X11/GLX**
  - Platform standard for Linux graphics
  - No C++ alternative (XCB exists but GLX still needed)
  - Keep C interface, wrap in C++ classes
  
- ✅ **OpenGL**
  - Industry standard graphics API
  - C API, but C++ wrappers available (not replacing)
  - Keep direct OpenGL calls, use C++ for management
  
- ✅ **Freetype**
  - Industry standard font rendering
  - No viable C++ alternative
  - Keep C interface, wrap in C++ classes

#### Consider C++ Alternatives
- ⚠️ **liblo** (OSC) - Currently using C library
  - **Keep**: Lightweight, well-established, works well with C++
  - **Alternative**: Could use C++ OSC libraries (e.g., `oscpack`, `liboscpp`) but liblo is mature
  - **Decision**: Keep liblo for now, consider C++ alternative if needed for better integration
  
- ⚠️ **SDL** - Currently using C library
  - **Keep**: Well-established, cross-platform
  - **Alternative**: SDL2 has C++ bindings, but we're only using Linux
  - **Decision**: Keep C interface, not critical (optional dependency)

#### C Code to Keep (Performance/Compatibility)
- ✅ **Platform-specific display code** (`display_glx.c`, `display_x11.c`, `display_x_dnd.c`, `display_x_dialog.c`)
  - Direct X11/GLX API calls
  - Performance critical (rendering path)
  - Platform-specific, complex integration
  - **Decision**: Keep C code, wrap in C++ classes for management
  
- ✅ **FFmpeg compatibility layer** (`ffcompat.h`)
  - Handles FFmpeg API version differences
  - Required for FFmpeg integration
  - **Decision**: Keep as C/C++ bridge
  
- ⚠️ **Low-level utilities** (`gtime.c`, `smpte.c`)
  - Simple, fast operations
  - **Evaluation**: 
    - `gtime.c`: Migrate to `std::chrono` (C++ standard, better integration)
    - `smpte.c`: Migrate to C++ utility class (business logic, not performance critical)
  - **Decision**: Migrate to C++ for better maintainability and integration

#### C Code to Migrate (Business Logic)
- ⏳ **Video playback core** (`xjadeo.c`)
  - Complex state management
  - Would benefit from OOP
  - **Priority**: High - Core business logic
  - **Migration**: Migrate to C++ classes
  
- ⏳ **SMPTE utilities** (`smpte.c`)
  - Business logic, not performance critical
  - **Priority**: Medium - Utility, better integration
  - **Migration**: Migrate to C++ utility class
  
- ⏳ **Time utilities** (`gtime.c`)
  - Simple utilities, better as C++ classes
  - **Priority**: Medium - Replace with std::chrono
  - **Migration**: Migrate to C++ with std::chrono integration

### Migration Priority (from MIGRATION_STRATEGY_ANALYSIS.md)

**High Priority (Migrate):**
- `xjadeo.c` - Core business logic, complex state
- `smpte.c` - Utility, better as C++ class
- `gtime.c` - Replace with `std::chrono`

**Low Priority (Keep):**
- Platform display code (`display_glx.c`, `display_x11.c`) - Performance critical, platform APIs
- FFmpeg compatibility (`ffcompat.h`) - Required bridge

**Evaluate Case-by-Case:**
- Simple utilities - Migrate if maintainability > performance
- Platform code - Keep if performance critical or complex integration

## Current Status

**Completed**: Phases 1-10, 11, 12, 17, 20 (substantially complete)
**In Progress**: Phase 20 (Code Migration & Cleanup) - Final cleanup and testing
**Next**: 
- ⏳ Complete ALSA Sequencer MIDI driver testing
- ⏳ Remove legacy C code from build (`midi.c`, `xjadeo.c`, `ltc-jack.c` once fully tested)
- ⏳ Final integration testing
- ✅ Utilities migration complete (`smpte.c`, `gtime.c` removed)
- ✅ Legacy C code status documented (see `LEGACY_C_CODE_STATUS.md`)

**Migration Strategy**: Hybrid approach - Keep C for platform APIs/performance, migrate business logic to C++

## Key Components Implemented

### Core Classes
- `XjadeoApplication` - Main application orchestrator
- `ConfigurationManager` - Configuration handling
- `LayerManager` - Multi-layer management
- `VideoLayer` - Individual video layer with input/sync
- `OSDManager` - On-screen display state
- `OSDRenderer` - Freetype text rendering

### Interfaces
- `InputSource` - Abstract input (VideoFileInput implemented)
- `SyncSource` - Abstract sync (MIDI implemented, JACK removed)
- `RemoteControl` - Abstract remote (OSC implemented)
- `DisplayBackend` - Abstract display (OpenGLDisplay implemented - Linux GLX only)

### Utilities
- `FrameBuffer` - Video frame buffer management
- `FrameFormat` - Frame format definitions
- `LayerProperties` - Layer display properties
- `Logger` - Logging system
- `CLegacyBridge` - C/C++ integration bridge

## Notes
- All code compiles without errors
- Architecture is ready for future extensions
- OSD rendering is fully functional
- Multi-layer system is operational
- Remote control (OSC) is complete

## Library Migration Strategy

### Current C Libraries (Keep)
- **FFmpeg**: Industry standard, no alternative → Keep C interface
- **ALSA**: Linux standard, no alternative → Keep C interface, wrap in C++
- **X11/GLX**: Platform standard, no alternative → Keep C interface, wrap in C++
- **OpenGL**: Industry standard, C API → Keep direct calls, use C++ for management
- **Freetype**: Industry standard, no alternative → Keep C interface, wrap in C++
- **liblo**: OSC library → Keep for now (mature, lightweight)

### C++ Standard Library Usage
- ✅ **std::chrono**: Replace `gtime.c` with std::chrono for time operations
- ✅ **std::thread**: Already using for ALSA MIDI driver
- ✅ **std::mutex**: Already using for thread safety
- ✅ **std::string**: Already using throughout C++ code
- ✅ **std::vector**, **std::map**: Already using for collections
- ✅ **std::unique_ptr**, **std::shared_ptr**: Already using for memory management

### Performance Considerations
- **Keep C for**: 
  - Low-level platform APIs (X11, GLX, ALSA)
  - FFmpeg integration (C library, direct calls)
  - Rendering hot paths (if proven faster)
  - Simple, performance-critical utilities (if C++ overhead matters)
  
- **Use C++ for**: 
  - Business logic and state management
  - Error handling (exceptions, RAII)
  - Testing (easier to mock/test)
  - Memory management (smart pointers)
  - Threading (std::thread, std::mutex)
  - Time operations (std::chrono)
  
- **Hybrid approach**: 
  - C++ classes wrapping C libraries (current approach)
  - C++ for application logic, C for platform/library integration
  - Evaluate performance on case-by-case basis

### Library Replacement Evaluation

#### No Replacement Needed (Keep C Libraries)
- **FFmpeg**: Industry standard, mature, no C++ alternative
- **ALSA**: Linux standard, no alternative
- **X11/GLX**: Platform standard, required for Linux
- **OpenGL**: Industry standard graphics API
- **Freetype**: Industry standard font rendering

#### Potential Replacements (Evaluate)
- **liblo (OSC)**: 
  - Current: C library, lightweight, mature
  - Alternatives: `oscpack` (C++), `liboscpp` (C++)
  - **Decision**: Keep liblo unless C++ alternative provides significant benefits
  - **Rationale**: liblo is well-tested, lightweight, works fine with C++

#### C++ Standard Library Replacements
- ✅ **std::chrono** → Replace `gtime.c` time functions
- ✅ **std::thread** → Already using for threading
- ✅ **std::mutex** → Already using for synchronization
- ✅ **std::string** → Already using throughout
- ✅ **std::vector/map** → Already using for collections
- ✅ **Smart pointers** → Already using for memory management

