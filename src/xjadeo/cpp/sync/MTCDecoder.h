#ifndef XJADEO_MTCDECODER_H
#define XJADEO_MTCDECODER_H

#include <cstdint>
#include <cstring>

namespace xjadeo {

/**
 * MTCDecoder - Decodes MIDI Time Code (MTC) messages
 * 
 * Parses MTC quarter-frame messages and converts them to frame numbers.
 */
class MTCDecoder {
public:
    struct SMPTETimecode {
        int frame;
        int sec;
        int min;
        int hour;
        int type;  // 0=24fps, 1=25fps, 2=29fps, 3=30fps
        int tick;  // Quarter-frame tick (0-7)
    };

    MTCDecoder();
    
    /**
     * Process a MIDI message byte
     * @param data MIDI message byte
     * @return true if complete timecode received
     */
    bool processByte(uint8_t data);

    /**
     * Get the decoded SMPTE timecode
     * @return SMPTE timecode structure
     */
    const SMPTETimecode& getTimecode() const { return lastTC_; }

    /**
     * Convert SMPTE timecode to frame number
     * @param framerate Video framerate
     * @return Frame number, or -1 if invalid
     */
    int64_t timecodeToFrame(double framerate) const;

    /**
     * Reset decoder state
     */
    void reset();

private:
    static const char* MTCTYPE[4];
    
    void parseTimecode(uint8_t data);
    
    SMPTETimecode tc_;
    SMPTETimecode lastTC_;
    int fullTC_;  // Bitmask tracking which quarter-frames received
    int prevTick_;
};

} // namespace xjadeo

#endif // XJADEO_MTCDECODER_H

