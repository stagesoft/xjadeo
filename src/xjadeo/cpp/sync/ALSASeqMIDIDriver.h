#ifndef XJADEO_ALSASEQMIDIDRIVER_H
#define XJADEO_ALSASEQMIDIDRIVER_H

#include "MIDIDriver.h"
#include "MTCDecoder.h"
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>

// Forward declarations for ALSA
struct snd_seq;
typedef struct _snd_seq snd_seq_t;

namespace xjadeo {

/**
 * ALSASeqMIDIDriver - ALSA Sequencer MIDI driver implementation
 * 
 * Implements MIDIDriver using ALSA Sequencer API.
 * Supports MTC (MIDI Time Code) synchronization via ALSA sequencer ports.
 */
class ALSASeqMIDIDriver : public MIDIDriver {
public:
    ALSASeqMIDIDriver();
    virtual ~ALSASeqMIDIDriver();

    // MIDIDriver interface
    bool open(const std::string& portId) override;
    void close() override;
    bool isConnected() const override;
    int64_t pollFrame() override;
    const char* getName() const override { return "ALSA-Sequencer"; }
    bool isSupported() const override;

    /**
     * Set framerate for frame calculation
     * @param fps Framerate in frames per second
     */
    void setFramerate(double fps) { framerate_ = fps; }

    /**
     * Set MIDI clock adjustment mode
     * @param enabled If true, enable clock adjustment for smoother sync
     */
    void setClockAdjustment(bool enabled) { midiClkAdj_ = enabled; }

    /**
     * Set frame delay (for clock adjustment)
     * @param delay Delay in seconds (1.0/fps, or 0 to calculate from framerate)
     */
    void setDelay(double delay) { delay_ = delay; }

    /**
     * Set verbose mode
     * @param verbose If true, enable verbose output
     */
    void setVerbose(bool verbose) { verbose_ = verbose; }

    /**
     * Detect and list available MIDI devices
     * @param verbose If true, print device list
     * @return Number of devices found
     */
    int detectDevices(bool verbose = false);

private:
    // ALSA sequencer functions
    bool openSequencer();
    void closeSequencer();
    bool createPort();
    bool connectToPort(const std::string& portName);
    void processEvent(void* event);
    
    // Background thread function
    void runThread();

    snd_seq_t* seq_;
    int portId_;
    MTCDecoder mtcDecoder_;
    double framerate_;
    
    // Thread management
    std::thread thread_;
    std::mutex mutex_;
    std::atomic<bool> stopThread_;
    std::atomic<bool> connected_;
    
    // Frame tracking
    int64_t lastFrame_;
    int64_t currentFrame_;
    int stopCount_;
    
    // Configuration
    bool midiClkAdj_;
    double delay_;
    bool verbose_;
};

} // namespace xjadeo

#endif // XJADEO_ALSASEQMIDIDRIVER_H

