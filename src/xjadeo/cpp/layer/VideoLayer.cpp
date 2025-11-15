#include "VideoLayer.h"
#include <algorithm>
#include <cmath>

namespace xjadeo {

VideoLayer::VideoLayer()
    : playing_(false)
    , currentFrame_(-1)
    , lastSyncFrame_(-1)
    , timeOffset_(0)
    , timeScale_(1.0)
    , wraparound_(false)
    , layerId_(-1)
{
}

VideoLayer::~VideoLayer() {
    pause();
}

void VideoLayer::setInputSource(std::unique_ptr<InputSource> input) {
    pause();
    inputSource_ = std::move(input);
    currentFrame_ = -1;
    lastSyncFrame_ = -1;
}

void VideoLayer::setSyncSource(std::unique_ptr<SyncSource> sync) {
    syncSource_ = std::move(sync);
    lastSyncFrame_ = -1;
}

bool VideoLayer::play() {
    if (!isReady()) {
        return false;
    }
    playing_ = true;
    return true;
}

bool VideoLayer::pause() {
    playing_ = false;
    return true;
}

bool VideoLayer::seek(int64_t frameNumber) {
    if (!inputSource_) {
        return false;
    }
    
    if (inputSource_->seek(frameNumber)) {
        currentFrame_ = frameNumber;
        lastSyncFrame_ = -1;
        return true;
    }
    
    return false;
}

void VideoLayer::update() {
    if (!isReady()) {
        return;
    }

    if (playing_) {
        updateFromSyncSource();
    }
}

void VideoLayer::updateFromSyncSource() {
    if (!syncSource_ || !syncSource_->isConnected()) {
        // No sync source - manual playback or paused
        return;
    }

    uint8_t rolling = 0;
    int64_t syncFrame = syncSource_->pollFrame(&rolling);
    
    if (syncFrame >= 0) {
        // Apply time-scaling: multiply by timescale, then add offset
        int64_t adjustedFrame = static_cast<int64_t>(std::floor(static_cast<double>(syncFrame) * timeScale_)) + timeOffset_;
        
        // Apply wraparound if enabled
        if (wraparound_ && inputSource_) {
            FrameInfo info = inputSource_->getFrameInfo();
            int64_t totalFrames = info.totalFrames;
            
            if (totalFrames > 0) {
                // Wrap around if frame is beyond duration
                while (adjustedFrame > totalFrames) {
                    adjustedFrame -= totalFrames;
                }
                // Wrap around if frame is negative
                while (adjustedFrame < 0) {
                    adjustedFrame += totalFrames;
                }
            }
        }
        
        // Only load frame if it's different from the last one
        // This avoids unnecessary seeks and frame loads
        if (adjustedFrame != lastSyncFrame_) {
            // Check for large jumps (more than 1 frame difference)
            // This might indicate a seek or transport jump
            bool isLargeJump = (lastSyncFrame_ >= 0 && 
                               std::abs(adjustedFrame - lastSyncFrame_) > 1);
            
            if (loadFrame(adjustedFrame)) {
                currentFrame_ = adjustedFrame;
                lastSyncFrame_ = adjustedFrame;
            } else if (isLargeJump) {
                // On large jumps, if load fails, try seeking first
                // This helps with keyframe-based codecs
                if (inputSource_ && inputSource_->seek(adjustedFrame)) {
                    if (loadFrame(adjustedFrame)) {
                        currentFrame_ = adjustedFrame;
                        lastSyncFrame_ = adjustedFrame;
                    }
                }
            }
        }
    }
    
    // Update playing state based on rolling
    if (rolling == 0 && playing_) {
        // Not rolling - pause playback
        playing_ = false;
    } else if (rolling != 0 && !playing_) {
        // Started rolling - resume playback
        playing_ = true;
    }
}

bool VideoLayer::loadFrame(int64_t frameNumber) {
    if (!inputSource_ || !inputSource_->isReady()) {
        return false;
    }

    return inputSource_->readFrame(frameNumber, frameBuffer_);
}

bool VideoLayer::render(FrameBuffer& outputBuffer) {
    if (!isReady() || !frameBuffer_.isValid()) {
        return false;
    }

    // For now, simple copy - full compositing will be done in display backend
    // This is a placeholder that ensures the frame is loaded
    if (currentFrame_ >= 0 && frameBuffer_.isValid()) {
        // Frame is ready for compositing
        return true;
    }

    return false;
}

bool VideoLayer::isReady() const {
    return inputSource_ != nullptr && inputSource_->isReady();
}

FrameInfo VideoLayer::getFrameInfo() const {
    if (inputSource_) {
        return inputSource_->getFrameInfo();
    }
    return FrameInfo();
}

void VideoLayer::reverse() {
    // Reverse playback: multiply timescale by -1.0 and adjust offset
    // to keep current frame displayed
    if (currentFrame_ >= 0) {
        // Calculate new offset to maintain current frame position
        // newFrame = (syncFrame * -timeScale) + newOffset
        // We want: currentFrame = (syncFrame * -timeScale) + newOffset
        // So: newOffset = currentFrame - (syncFrame * -timeScale)
        // But we don't have syncFrame here, so we use a simpler approach:
        // Set offset to currentFrame and negate timescale
        timeOffset_ = currentFrame_;
        timeScale_ = -timeScale_;
    } else {
        // Just negate timescale if no current frame
        timeScale_ = -timeScale_;
    }
}

} // namespace xjadeo

