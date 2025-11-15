#include "SMPTEWrapper.h"
#include "SMPTEUtils.h"
#include <string>
#include <cstring>

extern "C" {

// External globals from C codebase (needed for compatibility)
extern double framerate;
extern int want_dropframes;
extern int want_autodrop;
extern int have_dropframes;
#ifdef HAVE_MIDI
extern int midi_clkconvert;
#else
static int midi_clkconvert = 0;
#endif

int64_t smptestring_to_frame(const char* str) {
    if (!str) {
        return 0;
    }
    
    std::string smpteStr(str);
    return xjadeo::SMPTEUtils::smpteStringToFrame(
        smpteStr, 
        framerate, 
        have_dropframes != 0,
        want_dropframes != 0,
        want_autodrop != 0
    );
}

int frame_to_smptestring(char* smptestring, int64_t frame, uint8_t add_sign) {
    if (!smptestring) {
        return 0;
    }
    
    std::string result = xjadeo::SMPTEUtils::frameToSmpteString(
        frame,
        framerate,
        add_sign != 0,
        have_dropframes != 0,
        want_dropframes != 0,
        want_autodrop != 0
    );
    
    // Copy to output buffer (ensure null termination)
    strncpy(smptestring, result.c_str(), 13);
    smptestring[13] = '\0';
    
    // Return overflow value (from original implementation)
    // The original function returned s.v[SMPTE_OVERFLOW], but our implementation
    // doesn't expose this. Return 0 for compatibility.
    return 0;
}

int64_t smpte_to_frame(int type, int f, int s, int m, int h, int overflow) {
    return xjadeo::SMPTEUtils::smpteToFrame(
        type, f, s, m, h, overflow,
        framerate,
        want_dropframes != 0,
        midi_clkconvert
    );
}

} // extern "C"

