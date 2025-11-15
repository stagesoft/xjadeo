#include "MIDISyncSource.h"
#include "NullMIDIDriver.h"
#include "ALSASeqMIDIDriver.h"
#include <cstring>

namespace xjadeo {

MIDISyncSource::MIDISyncSource()
    : framerate_(25.0)
    , currentFrame_(-1)
    , connected_(false)
{
    // Start with null driver (will be replaced when driver is chosen)
    driver_ = std::make_unique<NullMIDIDriver>();
}

MIDISyncSource::~MIDISyncSource() {
    disconnect();
}

bool MIDISyncSource::connect(const char* param) {
    if (connected_) {
        disconnect();
    }

    if (param) {
        midiPort_ = param;
    } else {
        midiPort_ = "-1"; // Default: autodetect
    }

    // If no driver selected, try to get first available
    if (!driver_ || dynamic_cast<NullMIDIDriver*>(driver_.get())) {
        driver_ = MIDIDriverFactory::createFirstAvailable();
        if (!driver_) {
            driver_ = std::make_unique<NullMIDIDriver>();
            return false;
        }
    }

    // Open MIDI connection
    connected_ = driver_->open(midiPort_);
    if (connected_) {
        currentFrame_ = -1;
        mtcDecoder_.reset();
    }

    return connected_;
}

void MIDISyncSource::disconnect() {
    if (connected_ && driver_) {
        driver_->close();
        connected_ = false;
        currentFrame_ = -1;
        mtcDecoder_.reset();
    }
    midiPort_.clear();
}

bool MIDISyncSource::isConnected() const {
    return connected_ && driver_ && driver_->isConnected();
}

void MIDISyncSource::setClockAdjustment(bool enabled) {
    ALSASeqMIDIDriver* alsaDriver = dynamic_cast<ALSASeqMIDIDriver*>(driver_.get());
    if (alsaDriver) {
        alsaDriver->setClockAdjustment(enabled);
    }
}

void MIDISyncSource::setDelay(double delay) {
    ALSASeqMIDIDriver* alsaDriver = dynamic_cast<ALSASeqMIDIDriver*>(driver_.get());
    if (alsaDriver) {
        alsaDriver->setDelay(delay);
    }
}

void MIDISyncSource::setVerbose(bool verbose) {
    ALSASeqMIDIDriver* alsaDriver = dynamic_cast<ALSASeqMIDIDriver*>(driver_.get());
    if (alsaDriver) {
        alsaDriver->setVerbose(verbose);
    }
}

int64_t MIDISyncSource::pollFrame(uint8_t* rolling) {
    if (!isConnected()) {
        return -1;
    }

    // Set framerate on driver if it supports it (for ALSA driver)
    ALSASeqMIDIDriver* alsaDriver = dynamic_cast<ALSASeqMIDIDriver*>(driver_.get());
    if (alsaDriver) {
        alsaDriver->setFramerate(framerate_);
    }

    // Poll driver for frame (driver handles MIDI message parsing)
    int64_t frame = driver_->pollFrame();
    
    if (frame >= 0) {
        currentFrame_ = frame;
    }

    // Note: MIDI doesn't provide rolling state directly
    // This would need to be tracked separately if needed
    if (rolling) {
        *rolling = (frame >= 0) ? 1 : 0;
    }

    return frame;
}

int64_t MIDISyncSource::getCurrentFrame() const {
    return currentFrame_;
}

const char* MIDISyncSource::getName() const {
    return "MIDI";
}

bool MIDISyncSource::chooseDriver(const std::string& driverName) {
    if (connected_) {
        return false; // Can't change driver while connected
    }

    auto newDriver = MIDIDriverFactory::create(driverName);
    if (newDriver && newDriver->isSupported()) {
        driver_ = std::move(newDriver);
        return true;
    }

    return false;
}

} // namespace xjadeo

