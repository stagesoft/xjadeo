#ifndef XJADEO_SYNCSOURCE_H
#define XJADEO_SYNCSOURCE_H

#include <cstdint>

namespace xjadeo {

/**
 * Abstract base class for all synchronization sources.
 * 
 * Each layer can have its own sync source, allowing independent
 * synchronization (JACK, LTC, MIDI, manual, etc.)
 */
class SyncSource {
public:
    virtual ~SyncSource() = default;

    /**
     * Connect to the sync source
     * @param param Optional parameter (e.g., MIDI port name)
     * @return true on success, false on failure
     */
    virtual bool connect(const char* param = nullptr) = 0;

    /**
     * Disconnect from the sync source
     */
    virtual void disconnect() = 0;

    /**
     * Check if connected to sync source
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const = 0;

    /**
     * Poll for current frame number from sync source
     * @param rolling Optional pointer to set rolling state (true if playing)
     * @return Current frame number, or -1 if not available
     */
    virtual int64_t pollFrame(uint8_t* rolling = nullptr) = 0;

    /**
     * Get the current frame number (last polled value)
     * @return Current frame number, or -1 if not available
     */
    virtual int64_t getCurrentFrame() const = 0;

    /**
     * Get name/identifier of this sync source type
     * @return String identifier (e.g., "MIDI", "LTC")
     */
    virtual const char* getName() const = 0;
};

} // namespace xjadeo

#endif // XJADEO_SYNCSOURCE_H

