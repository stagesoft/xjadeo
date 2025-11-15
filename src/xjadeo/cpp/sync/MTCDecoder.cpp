#include "MTCDecoder.h"
#include <cmath>

namespace xjadeo {

const char* MTCDecoder::MTCTYPE[4] = {
    "24fps",
    "25fps",
    "29fps",
    "30fps"
};

MTCDecoder::MTCDecoder()
    : fullTC_(0)
    , prevTick_(0)
{
    reset();
}

void MTCDecoder::reset() {
    memset(&tc_, 0, sizeof(tc_));
    memset(&lastTC_, 0, sizeof(lastTC_));
    fullTC_ = 0;
    prevTick_ = 0;
}

bool MTCDecoder::processByte(uint8_t data) {
    // MTC quarter-frame messages come as: status byte 0xF1, then data byte
    // The data byte contains the quarter-frame information in the lower 7 bits
    // Upper nibble (bits 4-7) indicates which quarter-frame (0-7)
    // Lower nibble (bits 0-3) contains the data
    
    // Check if this is a quarter-frame data byte (0x00-0x7F, with upper nibble 0-7)
    uint8_t nibble = (data >> 4) & 0x0F;
    if (nibble <= 7 && (data & 0x80) == 0) {
        // This is a valid quarter-frame data byte
        parseTimecode(data);
        
        // Check if we have a complete timecode (all 8 quarter-frames)
        if (fullTC_ == 0xFF) {
            lastTC_ = tc_;
            // Reset for next timecode
            tc_.type = tc_.min = tc_.frame = tc_.sec = tc_.hour = tc_.tick = 0;
            fullTC_ = 0;
            return true;
        }
    }
    
    return false;
}

void MTCDecoder::parseTimecode(uint8_t data) {
    int nibble = (data >> 4) & 0x0F;
    
    // Set tick based on which quarter-frame this is
    prevTick_ = tc_.tick;
    
    switch (nibble) {
        case 0x0: // Frame LSN (Least Significant Nibble)
            tc_.tick = 1;
            tc_.frame = (tc_.frame & 0xF0) | (data & 0x0F);
            fullTC_ |= (1 << 1);
            break;
        case 0x1: // Frame MSN (Most Significant Nibble)
            tc_.tick = 2;
            tc_.frame = (tc_.frame & 0x0F) | ((data & 0x0F) << 4);
            fullTC_ |= (1 << 2);
            break;
        case 0x2: // Seconds LSN
            tc_.tick = 3;
            tc_.sec = (tc_.sec & 0xF0) | (data & 0x0F);
            fullTC_ |= (1 << 3);
            break;
        case 0x3: // Seconds MSN
            tc_.tick = 4;
            tc_.sec = (tc_.sec & 0x0F) | ((data & 0x0F) << 4);
            fullTC_ |= (1 << 4);
            break;
        case 0x4: // Minutes LSN
            tc_.tick = 5;
            tc_.min = (tc_.min & 0xF0) | (data & 0x0F);
            fullTC_ |= (1 << 5);
            break;
        case 0x5: // Minutes MSN
            tc_.tick = 6;
            tc_.min = (tc_.min & 0x0F) | ((data & 0x0F) << 4);
            fullTC_ |= (1 << 6);
            break;
        case 0x6: // Hours LSN
            tc_.tick = 7;
            tc_.hour = (tc_.hour & 0xF0) | (data & 0x0F);
            fullTC_ |= (1 << 7);
            break;
        case 0x7: // Hours MSN and type
            tc_.tick = 0;
            tc_.hour = (tc_.hour & 0x0F) | ((data & 0x01) << 4);
            tc_.type = (data >> 1) & 0x03;
            fullTC_ |= (1 << 0);
            break;
    }
}

int64_t MTCDecoder::timecodeToFrame(double framerate) const {
    // Use lastTC_ which is set when complete timecode is received
    const auto& tc = lastTC_;
    
    // Calculate total seconds
    int64_t totalSeconds = tc.hour * 3600 + tc.min * 60 + tc.sec;
    
    // Get frames per second based on MTC type
    double fps;
    switch (tc.type) {
        case 0: fps = 24.0; break;
        case 1: fps = 25.0; break;
        case 2: fps = 29.97; break;  // Drop-frame (29.97 fps)
        case 3: fps = 30.0; break;
        default: fps = framerate; break;
    }
    
    // Convert to frame number
    int64_t frame = static_cast<int64_t>(totalSeconds * fps) + tc.frame;
    
    // Apply framerate adjustment if needed
    if (std::abs(framerate - fps) > 0.01) {
        frame = static_cast<int64_t>(frame * (framerate / fps));
    }
    
    return frame;
}

} // namespace xjadeo

