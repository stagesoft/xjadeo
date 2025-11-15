#include "SMPTEUtils.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iomanip>

namespace xjadeo {

#define FIX_SMPTE_OVERFLOW(THIS, NEXT, INC) \
    if (s->v[(THIS)] >= (INC)) { \
        int ov = static_cast<int>(std::floor(static_cast<double>(s->v[(THIS)]) / (INC))); \
        s->v[(THIS)] -= ov * (INC); \
        s->v[(NEXT)] += ov; \
    } \
    if (s->v[(THIS)] < 0) { \
        int ov = static_cast<int>(std::floor(static_cast<double>(s->v[(THIS)]) / (INC))); \
        s->v[(THIS)] -= ov * (INC); \
        s->v[(NEXT)] += ov; \
    }

void SMPTEUtils::parseInt(BCD* s, int val, double framerate) {
    for (int i = 0; i < SMPTE_LAST; i++) {
        s->v[i] = 0;
    }
    
    s->v[SMPTE_FRAME] = val;
    
    FIX_SMPTE_OVERFLOW(SMPTE_FRAME, SMPTE_SEC, std::ceil(framerate));
    FIX_SMPTE_OVERFLOW(SMPTE_SEC, SMPTE_MIN, 60);
    FIX_SMPTE_OVERFLOW(SMPTE_MIN, SMPTE_HOUR, 60);
    FIX_SMPTE_OVERFLOW(SMPTE_HOUR, SMPTE_OVERFLOW, 24);
}

void SMPTEUtils::parseString(BCD* s, const std::string& val, double framerate) {
    for (int i = 0; i < SMPTE_LAST; i++) {
        s->v[i] = 0;
    }
    
    std::string buf = val;
    int i = 0;
    
    while (i < SMPTE_OVERFLOW) {
        size_t pos = buf.find_last_of(':');
        if (pos == std::string::npos) {
            pos = buf.find_last_of(';');
        }
        
        if (pos == std::string::npos) {
            s->v[i] = std::atoi(buf.c_str());
            break;
        }
        
        std::string tmp = buf.substr(pos + 1);
        s->v[i] = std::atoi(tmp.c_str());
        buf = buf.substr(0, pos);
        i++;
    }
    
    if (i < SMPTE_OVERFLOW) {
        s->v[i] = std::atoi(buf.c_str());
    }
    
    FIX_SMPTE_OVERFLOW(SMPTE_FRAME, SMPTE_SEC, std::ceil(framerate));
    FIX_SMPTE_OVERFLOW(SMPTE_SEC, SMPTE_MIN, 60);
    FIX_SMPTE_OVERFLOW(SMPTE_MIN, SMPTE_HOUR, 60);
    FIX_SMPTE_OVERFLOW(SMPTE_HOUR, SMPTE_OVERFLOW, 24);
}

int64_t SMPTEUtils::toFrame(const BCD* s, double framerate) {
    int frame = 0;
    int sec = 0;
    
    sec = ((((s->v[SMPTE_HOUR] * 60) + s->v[SMPTE_MIN]) * 60) + s->v[SMPTE_SEC]);
    if (s->v[SMPTE_OVERFLOW] < 0) {
        sec = 86400 - sec;
        sec *= -1;
    }
    
    frame = static_cast<int>(std::floor(sec * std::ceil(framerate))) + s->v[SMPTE_FRAME];
    return frame;
}

int64_t SMPTEUtils::insertDropFrames(int64_t frames) {
    int64_t minutes = (frames / 17982L) * 10L; // 17982 frames in 10 mins base
    int64_t off_f = frames % 17982L;
    int64_t off_adj = 0;
    
    if (off_f >= 1800L) { // nothing to do in the first minute
        off_adj = 2 + 2 * static_cast<int64_t>(std::floor((off_f - 1800L) / 1798L));
    }
    
    return (1800L * minutes + off_f + off_adj);
}

int64_t SMPTEUtils::smpteToFrame(int type, int f, int s, int m, int h, int overflow,
                                  double framerate, bool wantDropframes, int midiClkConvert) {
    int64_t frame = 0;
    double fps = framerate;
    
    // Determine FPS based on type
    switch (type) {
        case 0: fps = 24.0; break;
        case 1: fps = 25.0; break;
        case 2: fps = 30.0 * 1000.0 / 1001.0; break; // 29.97fps
        case 3: fps = 30.0; break;
        default: fps = framerate; break;
    }
    
    bool useDropframes = (type == 2 || wantDropframes);
    
    if (useDropframes) {
        /*
         * Drop frame numbers (not frames) 00:00 and 00:01 at the
         * start of every minute except the tenth.
         */
        int64_t base_time = static_cast<int64_t>((h * 3600) + ((m / 10) * 10 * 60)) * fps;
        int64_t off_m = m % 10;
        int64_t off_s = (off_m * 60) + s;
        int64_t off_f = (30 * off_s) + f - (2 * off_m);
        frame = base_time + off_f;
        fps = 30.0;
    } else {
        frame = f + static_cast<int64_t>(fps * (s + 60 * m + 3600 * h));
    }
    
    // Apply MIDI clock conversion
    switch (midiClkConvert) {
        case 1: // force video fps
            frame = f + static_cast<int64_t>(std::floor(framerate * (s + 60 * m + 3600 * h)));
            break;
        case 2: // 'convert' FPS
            frame = static_cast<int64_t>(std::rint(frame * framerate / fps));
            break;
        default: // use MTC fps info
            break;
    }
    
    return frame;
}

int64_t SMPTEUtils::smpteStringToFrame(const std::string& str, double framerate,
                                        bool haveDropframes, bool wantDropframes, bool wantAutodrop) {
    BCD s;
    int64_t frame;
    
    parseString(&s, str, framerate);
    
    // Check if string contains ';' (drop-frame separator) or if drop-frames are enabled
    bool useDropframes = (str.find(';') != std::string::npos && haveDropframes && wantAutodrop) || wantDropframes;
    
    if (useDropframes) {
        frame = smpteToFrame(2 /* 29.97fps */, s.v[SMPTE_FRAME], s.v[SMPTE_SEC],
                            s.v[SMPTE_MIN], s.v[SMPTE_HOUR], 0, framerate, wantDropframes);
        
        if (s.v[SMPTE_OVERFLOW] < 0) {
            frame = 30 * 86400 - frame;
            frame *= -1;
        }
    } else {
        frame = toFrame(&s, framerate);
    }
    
    return frame;
}

std::string SMPTEUtils::frameToSmpteString(int64_t frame, double framerate,
                                             bool addSign, bool haveDropframes,
                                             bool wantDropframes, bool wantAutodrop) {
    BCD s;
    int64_t frames = frame;
    char sep = ':';
    
    bool useDropframes = (haveDropframes && wantAutodrop) || wantDropframes;
    
    if (useDropframes) {
        frames = insertDropFrames(frames);
        sep = ';';
    }
    
    if (addSign && frames < 0) {
        parseInt(&s, static_cast<int>(-frames), framerate);
    } else {
        parseInt(&s, static_cast<int>(frames), framerate);
    }
    
    std::ostringstream oss;
    oss << std::setfill('0');
    
    if (addSign) {
        oss << ((frames < 0) ? '-' : ' ') << std::setw(2) << s.v[SMPTE_HOUR]
            << ':' << std::setw(2) << s.v[SMPTE_MIN]
            << ':' << std::setw(2) << s.v[SMPTE_SEC]
            << sep << std::setw(2) << s.v[SMPTE_FRAME];
    } else {
        oss << std::setw(2) << s.v[SMPTE_HOUR]
            << ':' << std::setw(2) << s.v[SMPTE_MIN]
            << ':' << std::setw(2) << s.v[SMPTE_SEC]
            << sep << std::setw(2) << s.v[SMPTE_FRAME];
    }
    
    return oss.str();
}

} // namespace xjadeo

