#ifndef XJADEO_NULLMIDIDRIVER_H
#define XJADEO_NULLMIDIDRIVER_H

#include "MIDIDriver.h"

namespace xjadeo {

/**
 * NullMIDIDriver - Null/dummy MIDI driver implementation
 * 
 * Used when no MIDI drivers are available or as a fallback.
 */
class NullMIDIDriver : public MIDIDriver {
public:
    bool open(const std::string& portId) override { return false; }
    void close() override {}
    bool isConnected() const override { return false; }
    int64_t pollFrame() override { return -1; }
    const char* getName() const override { return "None"; }
    bool isSupported() const override { return true; }  // Always available
};

} // namespace xjadeo

#endif // XJADEO_NULLMIDIDRIVER_H

