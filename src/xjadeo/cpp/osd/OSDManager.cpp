#include "OSDManager.h"
#include "../utils/SMPTEUtils.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace xjadeo {

OSDManager::OSDManager()
    : mode_(NONE)
    , frameXAlign_(1)  // Center by default
    , frameYPercent_(95)
    , smpteXAlign_(1)  // Center by default
    , smpteYPercent_(89)
    , textXAlign_(1)   // Center by default
    , textYPercent_(50)
{
}

OSDManager::~OSDManager() {
}

void OSDManager::setFramePosition(int xAlign, int yPercent) {
    frameXAlign_ = xAlign;
    frameYPercent_ = yPercent;
}

void OSDManager::setFrameNumber(int64_t frame) {
    formatFrameNumber(frame);
}

void OSDManager::formatFrameNumber(int64_t frame) {
    std::ostringstream oss;
    oss << frame;
    frameText_ = oss.str();
}

void OSDManager::setSMPTEPosition(int xAlign, int yPercent) {
    smpteXAlign_ = xAlign;
    smpteYPercent_ = yPercent;
}

void OSDManager::setSMPTETimecode(const std::string& tc) {
    smpteText_ = tc;
}

void OSDManager::setText(const std::string& text) {
    text_ = text;
    if (!text_.empty()) {
        enableMode(TEXT);
    }
}

void OSDManager::setTextPosition(int xAlign, int yPercent) {
    textXAlign_ = xAlign;
    textYPercent_ = yPercent;
}

void OSDManager::setFontFile(const std::string& fontFile) {
    fontFile_ = fontFile;
}

void OSDManager::setMessage(const std::string& msg) {
    message_ = msg;
    if (!message_.empty()) {
        enableMode(MSG);
    }
}

void OSDManager::clear() {
    mode_ = NONE;
    frameText_.clear();
    smpteText_.clear();
    text_.clear();
    message_.clear();
}

void OSDManager::formatSMPTE(int64_t frame, double framerate) {
    if (framerate <= 0.0) {
        smpteText_ = "00:00:00:00";
        return;
    }
    
    // Use SMPTEUtils for proper timecode formatting (handles drop-frames, etc.)
    smpteText_ = SMPTEUtils::frameToSmpteString(frame, framerate, false, false, false, true);
}

} // namespace xjadeo

