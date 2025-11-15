# Legacy C Code Status

This document tracks the status of remaining C code files and their relationship to the C++ codebase.

## Files Not Used by C++ Codebase

These C files are still in the build but are **not used by the C++ codebase**. They exist for legacy compatibility or are being phased out.

### `xjadeo.c`
- **Status**: Legacy video playback logic
- **C++ Replacement**: `VideoFileInput`, `XjadeoApplication`, `VideoLayer`
- **Used by C++**: ❌ No
- **Used by C code**: ⚠️ Possibly (legacy event loop)
- **Can be removed**: ⏳ After testing, if no C code depends on it

### `midi.c`
- **Status**: MIDI implementation (C++ version complete)
- **C++ Replacement**: `MIDISyncSource`, `ALSASeqMIDIDriver`, `MTCDecoder`
- **Used by C++**: ❌ No (C++ uses pure C++ implementation)
- **Used by C code**: ⚠️ Possibly (legacy sync functions)
- **Can be removed**: ⏳ After testing, if no C code depends on it

### `common.c`
- **Status**: Common utilities and UI sync functions
- **C++ Replacement**: N/A (UI functions not migrated, OSD handled by `OSDManager`)
- **Used by C++**: ❌ No
- **Used by C code**: ✅ Yes (legacy UI functions, sync source switching)
- **Can be removed**: ❌ No (still needed by legacy C code)

### `ltc-jack.c`
- **Status**: LTC sync over JACK (JACK support removed)
- **C++ Replacement**: N/A (LTC not implemented in C++)
- **Used by C++**: ❌ No
- **Used by C code**: ⚠️ Possibly (legacy sync functions reference it)
- **Can be removed**: ⏳ After verifying no C code depends on it (JACK removed, so likely safe)

## Files Used by C++ Codebase (via Wrappers)

### `SMPTEWrapper.cpp` dependencies
- **C globals used**: `framerate`, `want_dropframes`, `want_autodrop`, `have_dropframes`, `midi_clkconvert`
- **Source**: These globals are defined in C code (`xjadeo.c`, `configfile.c` remnants)
- **Purpose**: C-compatible wrapper for C++ `SMPTEUtils` (allows C code to use C++ implementation)
- **Status**: ✅ Working as intended (bridge between C and C++)

## Files Kept in C (Per Hybrid Strategy)

### Display Backends
- `display_glx.c`, `display_x11.c`, `display_x_dnd.c`, `display_x_dialog.c`
- **Rationale**: Platform APIs (X11, GLX) are C-only, performance-critical
- **C++ Wrapper**: `OpenGLDisplay` class wraps C display code for management
- **Status**: ✅ Keep in C (per migration strategy)

### Freetype
- `freetype.c`
- **Rationale**: Freetype is C library, C++ `OSDRenderer` uses Freetype C API directly
- **Status**: ✅ Keep in C (library compatibility)

## Migration Notes

1. **CLegacyBridge.h**: Only used by `SMPTEWrapper.cpp` for C compatibility. C++ codebase does not use it directly.

2. **C Globals**: The C++ codebase does not access C globals directly (except via `SMPTEWrapper.cpp` which needs them for compatibility).

3. **Legacy Functions**: Functions in `common.c` like `ui_sync_*`, `ui_osd_*` are not used by C++ codebase. They exist for legacy C code compatibility.

4. **Removal Strategy**: 
   - Files not used by C++ can be removed once we verify no C code depends on them
   - `ltc-jack.c` is likely safe to remove (JACK removed, LTC not implemented)
   - `midi.c` can be removed after testing (C++ version complete)
   - `xjadeo.c` can be removed after testing (core logic migrated)

## Recommendations

1. ✅ **Keep**: Display backends, Freetype (per hybrid strategy)
2. ⏳ **Test and remove**: `midi.c`, `xjadeo.c` (after verification)
3. ⏳ **Evaluate removal**: `ltc-jack.c` (JACK removed, likely unused)
4. ❌ **Keep for now**: `common.c` (still used by legacy C code)

