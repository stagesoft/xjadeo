#ifndef XJADEO_MIDIDRIVER_H
#define XJADEO_MIDIDRIVER_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace xjadeo {

/**
 * MIDIDriver - Abstract base class for MIDI driver implementations
 * 
 * Provides a common interface for different MIDI backends:
 * - ALSA Sequencer
 * - PortMidi
 * - ALSA Raw MIDI
 */
class MIDIDriver {
public:
    virtual ~MIDIDriver() = default;

    /**
     * Open MIDI connection
     * @param portId Port identifier (device name, port number, or "-1" for autodetect)
     * @return true if connection successful
     */
    virtual bool open(const std::string& portId) = 0;

    /**
     * Close MIDI connection
     */
    virtual void close() = 0;

    /**
     * Check if MIDI is connected
     * @return true if connected
     */
    virtual bool isConnected() const = 0;

    /**
     * Poll for MIDI messages and return current frame number
     * @return Frame number, or -1 if not available
     */
    virtual int64_t pollFrame() = 0;

    /**
     * Get driver name
     * @return Driver name string
     */
    virtual const char* getName() const = 0;

    /**
     * Check if this driver is supported (compiled in)
     * @return true if supported
     */
    virtual bool isSupported() const = 0;
};

/**
 * MIDIDriverFactory - Factory for creating MIDI drivers
 */
class MIDIDriverFactory {
public:
    /**
     * Create a MIDI driver by name
     * @param driverName Driver name (e.g., "ALSA-Sequencer", "PORTMIDI")
     * @return MIDI driver instance, or nullptr if not found/unsupported
     */
    static std::unique_ptr<MIDIDriver> create(const std::string& driverName);

    /**
     * Create the first available MIDI driver
     * @return MIDI driver instance, or nullptr if none available
     */
    static std::unique_ptr<MIDIDriver> createFirstAvailable();

    /**
     * Get list of available driver names
     * @return Vector of driver names
     */
    static std::vector<std::string> getAvailableDrivers();
};

} // namespace xjadeo

#endif // XJADEO_MIDIDRIVER_H

