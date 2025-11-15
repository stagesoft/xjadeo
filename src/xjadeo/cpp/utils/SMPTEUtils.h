#ifndef XJADEO_SMPTE_UTILS_H
#define XJADEO_SMPTE_UTILS_H

#include <cstdint>
#include <string>

namespace xjadeo {

/**
 * SMPTE (Society of Motion Picture and Television Engineers) timecode utilities
 * 
 * Provides conversion between frame numbers and SMPTE timecode strings,
 * with support for drop-frame timecode (29.97fps).
 */
class SMPTEUtils {
public:
    /**
     * Convert SMPTE timecode components to frame number
     * 
     * @param type SMPTE type: 0=24fps, 1=25fps, 2=29.97fps (drop-frame), 3=30fps
     * @param f Frame (0-29)
     * @param s Second (0-59)
     * @param m Minute (0-59)
     * @param h Hour (0-23)
     * @param overflow Overflow flag (for negative timecodes)
     * @param framerate Video framerate (for FPS conversion)
     * @param wantDropframes Force drop-frame mode
     * @param midiClkConvert MIDI clock conversion mode (0=use MTC fps, 1=force video fps, 2=convert FPS)
     * @return Frame number
     */
    static int64_t smpteToFrame(int type, int f, int s, int m, int h, int overflow,
                                 double framerate, bool wantDropframes = false,
                                 int midiClkConvert = 0);

    /**
     * Convert SMPTE timecode string to frame number
     * 
     * Format: [[[HH:]MM:]SS:]FF
     * Supports drop-frame timecode (uses ';' separator)
     * 
     * @param str Timecode string (e.g., "01:23:45:12" or "01;23;45;12" for drop-frame)
     * @param framerate Video framerate
     * @param haveDropframes Whether the video has drop-frames
     * @param wantDropframes Force drop-frame mode
     * @param wantAutodrop Auto-detect drop-frame from separator
     * @return Frame number
     */
    static int64_t smpteStringToFrame(const std::string& str, double framerate,
                                       bool haveDropframes = false,
                                       bool wantDropframes = false,
                                       bool wantAutodrop = true);

    /**
     * Convert frame number to SMPTE timecode string
     * 
     * @param frame Frame number
     * @param framerate Video framerate
     * @param addSign Add sign prefix for negative frames
     * @param haveDropframes Whether the video has drop-frames
     * @param wantDropframes Force drop-frame mode
     * @param wantAutodrop Auto-detect drop-frame
     * @return SMPTE timecode string (e.g., "01:23:45:12" or "01;23;45;12")
     */
    static std::string frameToSmpteString(int64_t frame, double framerate,
                                          bool addSign = false,
                                          bool haveDropframes = false,
                                          bool wantDropframes = false,
                                          bool wantAutodrop = true);

private:
    // Internal BCD structure for timecode components
    enum { SMPTE_FRAME = 0, SMPTE_SEC, SMPTE_MIN, SMPTE_HOUR, SMPTE_OVERFLOW, SMPTE_LAST };
    
    struct BCD {
        int v[SMPTE_LAST];
    };
    
    static void parseInt(BCD* s, int val, double framerate);
    static void parseString(BCD* s, const std::string& val, double framerate);
    static int64_t toFrame(const BCD* s, double framerate);
    static int64_t insertDropFrames(int64_t frames);
};

} // namespace xjadeo

#endif // XJADEO_SMPTE_UTILS_H

