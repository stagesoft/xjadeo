#ifndef XJADEO_VIDEOLAYER_H
#define XJADEO_VIDEOLAYER_H

#include "LayerProperties.h"
#include "../input/InputSource.h"
#include "../sync/SyncSource.h"
#include "../video/FrameBuffer.h"
#include <memory>
#include <cstdint>

namespace xjadeo {

/**
 * VideoLayer - Represents a single video layer with its own input and sync
 * 
 * Each layer has:
 * - An InputSource (video file, live video, stream, etc.)
 * - A SyncSource (MIDI, LTC, manual, etc.)
 * - Display properties (position, size, opacity, etc.)
 * - Playback state (playing, paused, current frame)
 */
class VideoLayer {
public:
    VideoLayer();
    ~VideoLayer();

    // Layer management
    void setInputSource(std::unique_ptr<InputSource> input);
    void setSyncSource(std::unique_ptr<SyncSource> sync);
    
    InputSource* getInputSource() const { return inputSource_.get(); }
    SyncSource* getSyncSource() const { return syncSource_.get(); }

    // Properties
    LayerProperties& properties() { return properties_; }
    const LayerProperties& properties() const { return properties_; }

    // Playback control
    bool play();
    bool pause();
    bool isPlaying() const { return playing_; }
    
    bool seek(int64_t frameNumber);
    int64_t getCurrentFrame() const { return currentFrame_; }
    
    // Update layer (called from main loop)
    void update();
    
    // Render layer (called from display backend)
    bool render(FrameBuffer& outputBuffer);

    // Get layer state
    bool isReady() const;
    FrameInfo getFrameInfo() const;

    // Get frame buffer (for rendering)
    const FrameBuffer& getFrameBuffer() const { return frameBuffer_; }
    FrameBuffer& getFrameBuffer() { return frameBuffer_; }

    // Layer ID
    void setLayerId(int id) { layerId_ = id; }
    int getLayerId() const { return layerId_; }
    
    // Time-scaling (applied to sync source frames)
    void setTimeOffset(int64_t offset) { timeOffset_ = offset; }
    int64_t getTimeOffset() const { return timeOffset_; }
    
    void setTimeScale(double scale) { timeScale_ = scale; }
    double getTimeScale() const { return timeScale_; }
    
    void setWraparound(bool enabled) { wraparound_ = enabled; }
    bool getWraparound() const { return wraparound_; }
    
    // Reverse playback (multiplies timescale by -1.0 and adjusts offset)
    void reverse();

private:
    std::unique_ptr<InputSource> inputSource_;
    std::unique_ptr<SyncSource> syncSource_;
    LayerProperties properties_;
    
    // Playback state
    bool playing_;
    int64_t currentFrame_;
    int64_t lastSyncFrame_;
    int64_t timeOffset_;  // Time offset applied to sync frames
    double timeScale_;    // Time multiplier (default: 1.0)
    bool wraparound_;     // Enable wrap-around/loop
    
    // Frame buffer for this layer
    FrameBuffer frameBuffer_;
    
    // Layer identification
    int layerId_;
    
    // Internal methods
    void updateFromSyncSource();
    bool loadFrame(int64_t frameNumber);
};

} // namespace xjadeo

#endif // XJADEO_VIDEOLAYER_H

