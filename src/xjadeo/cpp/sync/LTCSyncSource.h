#ifndef XJADEO_LTCSYNCSOURCE_H
#define XJADEO_LTCSYNCSOURCE_H

#include "SyncSource.h"
#include <cstdint>

namespace xjadeo {

/**
 * LTCSyncSource - LTC (Linear Time Code) synchronization source
 * 
 * Implements SyncSource interface for LTC synchronization.
 * Uses libltc to decode LTC timecode from audio signal.
 */
class LTCSyncSource : public SyncSource {
public:
    LTCSyncSource();
    virtual ~LTCSyncSource();

    // SyncSource interface
    bool connect(const char* param = nullptr) override;
    void disconnect() override;
    bool isConnected() const override;
    int64_t pollFrame(uint8_t* rolling = nullptr) override;
    int64_t getCurrentFrame() const override;
    const char* getName() const override { return "LTC"; }

    /**
     * Set framerate for frame calculation
     * This should match the video file's framerate
     * @param fps Framerate in frames per second
     */
    void setFramerate(double fps);

private:
    double framerate_;
    int64_t currentFrame_;
    bool connected_;
};

} // namespace xjadeo

#endif // XJADEO_LTCSYNCSOURCE_H

