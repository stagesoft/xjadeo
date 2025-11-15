#include "TestFramework.h"
#include "../layer/VideoLayer.h"
#include "../input/InputSource.h"
#include "../sync/SyncSource.h"
#include "../video/FrameBuffer.h"
#include <memory>

using namespace xjadeo;
using namespace xjadeo::test;

// Mock InputSource for testing
class MockInputSource : public InputSource {
public:
    bool open(const std::string& source) override { return true; }
    void close() override {}
    bool seek(int64_t frameNumber) override { 
        lastSeekFrame_ = frameNumber;
        return true; 
    }
    bool readFrame(int64_t frameNumber, FrameBuffer& buffer) override { 
        lastReadFrame_ = frameNumber;
        return true; 
    }
    FrameInfo getFrameInfo() const override {
        FrameInfo info;
        info.width = 1920;
        info.height = 1080;
        info.framerate = 25.0;
        info.totalFrames = 1000;
        return info;
    }
    bool isReady() const override { return true; }
    
    int64_t lastSeekFrame_ = -1;
    int64_t lastReadFrame_ = -1;
};

// Mock SyncSource for testing
class MockSyncSource : public SyncSource {
public:
    bool connect() override { connected_ = true; return true; }
    void disconnect() override { connected_ = false; }
    bool isConnected() const override { return connected_; }
    int64_t pollFrame(uint8_t* rolling) override {
        if (rolling) *rolling = rolling_ ? 1 : 0;
        return currentFrame_;
    }
    int64_t getCurrentFrame() const override { return currentFrame_; }
    std::string getName() const override { return "MockSync"; }
    
    void setCurrentFrame(int64_t frame) { currentFrame_ = frame; }
    void setRolling(bool rolling) { rolling_ = rolling; }
    
private:
    bool connected_ = false;
    int64_t currentFrame_ = 0;
    bool rolling_ = false;
};

bool test_VideoLayer_PlayPause() {
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    
    TEST_ASSERT_FALSE(layer->isPlaying());
    
    bool played = layer->play();
    TEST_ASSERT_TRUE(played);
    TEST_ASSERT_TRUE(layer->isPlaying());
    
    bool paused = layer->pause();
    TEST_ASSERT_TRUE(paused);
    TEST_ASSERT_FALSE(layer->isPlaying());
    
    return true;
}

bool test_VideoLayer_Seek() {
    auto layer = std::make_unique<VideoLayer>();
    auto mockInput = std::make_unique<MockInputSource>();
    MockInputSource* inputPtr = mockInput.get();
    layer->setInputSource(std::move(mockInput));
    
    bool seeked = layer->seek(100);
    TEST_ASSERT_TRUE(seeked);
    TEST_ASSERT_EQ(layer->getCurrentFrame(), 100);
    TEST_ASSERT_EQ(inputPtr->lastSeekFrame_, 100);
    
    return true;
}

bool test_VideoLayer_TimeOffset() {
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    
    TEST_ASSERT_EQ(layer->getTimeOffset(), 0);
    
    layer->setTimeOffset(50);
    TEST_ASSERT_EQ(layer->getTimeOffset(), 50);
    
    return true;
}

bool test_VideoLayer_TimeScale() {
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    
    TEST_ASSERT_EQ(layer->getTimeScale(), 1.0);
    
    layer->setTimeScale(2.0);
    TEST_ASSERT_EQ(layer->getTimeScale(), 2.0);
    
    layer->setTimeScale(0.5);
    TEST_ASSERT_EQ(layer->getTimeScale(), 0.5);
    
    return true;
}

bool test_VideoLayer_Wraparound() {
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    
    TEST_ASSERT_FALSE(layer->getWraparound());
    
    layer->setWraparound(true);
    TEST_ASSERT_TRUE(layer->getWraparound());
    
    return true;
}

bool test_VideoLayer_Reverse() {
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    layer->seek(100);
    
    layer->setTimeScale(1.0);
    layer->setTimeOffset(0);
    
    layer->reverse();
    TEST_ASSERT_EQ(layer->getTimeScale(), -1.0);
    
    return true;
}

bool test_VideoLayer_SyncUpdate() {
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    
    auto mockSync = std::make_unique<MockSyncSource>();
    MockSyncSource* syncPtr = mockSync.get();
    layer->setSyncSource(std::move(mockSync));
    
    syncPtr->connect();
    syncPtr->setCurrentFrame(50);
    syncPtr->setRolling(true);
    
    layer->play();
    layer->update();
    
    // Frame should be updated from sync source
    TEST_ASSERT_EQ(layer->getCurrentFrame(), 50);
    
    return true;
}

