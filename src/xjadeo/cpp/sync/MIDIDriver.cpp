#include "MIDIDriver.h"
#include "NullMIDIDriver.h"
#include "ALSASeqMIDIDriver.h"
#include <algorithm>

namespace xjadeo {

std::unique_ptr<MIDIDriver> MIDIDriverFactory::create(const std::string& driverName) {
    if (driverName == "None" || driverName.empty()) {
        return std::make_unique<NullMIDIDriver>();
    }
    
    // Try case-insensitive match
    std::string lowerName = driverName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    // ALSA Sequencer driver (only driver being implemented)
    if (lowerName == "alsa-sequencer" || lowerName == "alsa-seq" || lowerName == "alsa") {
        auto driver = std::make_unique<ALSASeqMIDIDriver>();
        if (driver->isSupported()) {
            return driver;
        }
        return nullptr;
    }
    
    return nullptr;
}

std::unique_ptr<MIDIDriver> MIDIDriverFactory::createFirstAvailable() {
    // Try drivers in order of preference
    // 1. ALSA Sequencer (primary driver, Linux only)
    auto alsaDriver = std::make_unique<ALSASeqMIDIDriver>();
    if (alsaDriver->isSupported()) {
        return alsaDriver;
    }
    
    // Fallback to null driver (always available)
    return std::make_unique<NullMIDIDriver>();
}

std::vector<std::string> MIDIDriverFactory::getAvailableDrivers() {
    std::vector<std::string> drivers;
    
    // Null driver is always available
    drivers.push_back("None");
    
    // ALSA Sequencer (check if supported)
    auto alsaDriver = std::make_unique<ALSASeqMIDIDriver>();
    if (alsaDriver->isSupported()) {
        drivers.push_back("ALSA-Sequencer");
    }
    
    return drivers;
}

} // namespace xjadeo

