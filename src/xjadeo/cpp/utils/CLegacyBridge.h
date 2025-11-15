#ifndef XJADEO_CLEGACYBRIDGE_H
#define XJADEO_CLEGACYBRIDGE_H

/**
 * CLegacyBridge - Bridge between C++ code and legacy C code
 * 
 * Provides access to C globals and functions that need to be shared
 * between the old C codebase and new C++ code during migration.
 * 
 * This header should be included in C++ files that need to interact
 * with existing C code.
 */

extern "C" {
    // Video file information (from xjadeo.c)
    extern int movie_width;
    extern int movie_height;
    extern float movie_aspect;
    extern double framerate;
    extern int64_t frames;
    extern int have_dropframes;
    
    // Configuration flags
    extern int want_quiet;
    extern int want_verbose;
    extern int want_debug;
    
    // MIDI functions (DEPRECATED - use MIDISyncSource C++ implementation instead)
    // These are kept for backward compatibility with legacy C code only
    // The C++ MIDI implementation is complete and does not use these C functions
    // All configuration is now handled through ConfigurationManager
    // NOTE: C++ code should use MIDISyncSource class, not these functions
    int midi_connected(void);
    int64_t midi_poll_frame(void);
    void midi_open(char* midiid);
    void midi_close(void);
    const char* midi_driver_name();
}

#endif // XJADEO_CLEGACYBRIDGE_H

