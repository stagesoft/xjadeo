#ifndef XJADEO_MIDISYNCSOURCE_H
#define XJADEO_MIDISYNCSOURCE_H

#include "SyncSource.h"
#include "MIDIDriver.h"
#include "MTCDecoder.h"
#include <string>
#include <memory>
#include <cstdint>

namespace xjadeo {

/**
 * MIDISyncSource - MIDI Time Code (MTC) synchronization
 * 
 * Implements SyncSource interface for MTC over MIDI.
 * Uses pure C++ implementation with driver abstraction.
 */
class MIDISyncSource : public SyncSource {
public:
    MIDISyncSource();
    virtual ~MIDISyncSource();

    // SyncSource interface
    bool connect(const char* param = nullptr) override;
    void disconnect() override;
    bool isConnected() const override;
    int64_t pollFrame(uint8_t* rolling = nullptr) override;
    int64_t getCurrentFrame() const override;
    const char* getName() const override;

    /**
     * Set framerate for frame calculation
     * @param fps Framerate in frames per second
     */
    void setFramerate(double fps) { framerate_ = fps; }

    /**
     * Set MIDI clock adjustment mode
     * @param enabled If true, enable clock adjustment
     */
    void setClockAdjustment(bool enabled);

    /**
     * Set frame delay for clock adjustment
     * @param delay Delay in seconds (1.0/fps, or 0 to calculate from framerate)
     */
    void setDelay(double delay);

    /**
     * Set verbose mode
     * @param verbose If true, enable verbose output
     */
    void setVerbose(bool verbose);

    /**
     * Choose MIDI driver by name
     * @param driverName Driver name (e.g., "ALSA-Sequencer", "PORTMIDI")
     * @return true if driver selected successfully
     */
    bool chooseDriver(const std::string& driverName);

private:
    std::unique_ptr<MIDIDriver> driver_;
    MTCDecoder mtcDecoder_;
    std::string midiPort_;
    double framerate_;
    int64_t currentFrame_;
    bool connected_;
};

} // namespace xjadeo

#endif // XJADEO_MIDISYNCSOURCE_H

