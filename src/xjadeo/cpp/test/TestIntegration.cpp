#include "TestFramework.h"
#include "../layer/LayerManager.h"
#include "../layer/VideoLayer.h"
#include "../config/ConfigurationManager.h"
#include "../input/InputSource.h"
#include "../sync/SyncSource.h"
#include <memory>

using namespace xjadeo;
using namespace xjadeo::test;

// Mock InputSource for testing
class MockInputSource : public InputSource {
public:
    bool open(const std::string& source) override { return true; }
    void close() override {}
    bool seek(int64_t frameNumber) override { return true; }
    bool readFrame(int64_t frameNumber, FrameBuffer& buffer) override { return true; }
    FrameInfo getFrameInfo() const override {
        FrameInfo info;
        info.width = 1920;
        info.height = 1080;
        info.framerate = 25.0;
        info.totalFrames = 1000;
        return info;
    }
    bool isReady() const override { return true; }
    int64_t getCurrentFrame() const override { return 0; }
};

bool test_Integration_LayerManagerWithMultipleLayers() {
    LayerManager manager;
    
    // Create multiple layers
    for (int i = 0; i < 5; ++i) {
        auto layer = std::make_unique<VideoLayer>();
        layer->setInputSource(std::make_unique<MockInputSource>());
        layer->setLayerId(i + 1);
        layer->properties().zOrder = i;
        layer->properties().opacity = 0.5f + (i * 0.1f);
        
        int id = manager.addLayer(std::move(layer));
        TEST_ASSERT_EQ(id, i + 1);
    }
    
    TEST_ASSERT_EQ(manager.getLayerCount(), 5);
    
    // Test z-order sorting
    auto sorted = manager.getLayersSortedByZOrder();
    TEST_ASSERT_EQ(sorted.size(), 5);
    for (size_t i = 0; i < sorted.size(); ++i) {
        TEST_ASSERT_EQ(sorted[i]->properties().zOrder, static_cast<int>(i));
    }
    
    // Test layer removal
    bool removed = manager.removeLayer(3);
    TEST_ASSERT_TRUE(removed);
    TEST_ASSERT_EQ(manager.getLayerCount(), 4);
    
    return true;
}

bool test_Integration_VideoLayerTimeScaling() {
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    
    // Test time-scaling combination
    layer->setTimeScale(2.0);
    layer->setTimeOffset(100);
    layer->setWraparound(true);
    
    TEST_ASSERT_EQ(layer->getTimeScale(), 2.0);
    TEST_ASSERT_EQ(layer->getTimeOffset(), 100);
    TEST_ASSERT_TRUE(layer->getWraparound());
    
    // Test reverse
    layer->reverse();
    TEST_ASSERT_EQ(layer->getTimeScale(), -2.0);
    
    return true;
}

bool test_Integration_LayerProperties() {
    auto layer = std::make_unique<VideoLayer>();
    auto& props = layer->properties();
    
    // Test all property types
    props.x = 100;
    props.y = 200;
    props.width = 1920;
    props.height = 1080;
    props.opacity = 0.75f;
    props.zOrder = 5;
    props.visible = true;
    props.scaleX = 1.5f;
    props.scaleY = 0.8f;
    props.rotation = 45.0f;
    
    TEST_ASSERT_EQ(props.x, 100);
    TEST_ASSERT_EQ(props.y, 200);
    TEST_ASSERT_EQ(props.width, 1920);
    TEST_ASSERT_EQ(props.height, 1080);
    TEST_ASSERT_EQ(props.opacity, 0.75f);
    TEST_ASSERT_EQ(props.zOrder, 5);
    TEST_ASSERT_TRUE(props.visible);
    TEST_ASSERT_EQ(props.scaleX, 1.5f);
    TEST_ASSERT_EQ(props.scaleY, 0.8f);
    TEST_ASSERT_EQ(props.rotation, 45.0f);
    
    // Test crop
    props.crop.enabled = true;
    props.crop.x = 100;
    props.crop.y = 50;
    props.crop.width = 800;
    props.crop.height = 600;
    
    TEST_ASSERT_TRUE(props.crop.enabled);
    TEST_ASSERT_EQ(props.crop.x, 100);
    TEST_ASSERT_EQ(props.crop.y, 50);
    TEST_ASSERT_EQ(props.crop.width, 800);
    TEST_ASSERT_EQ(props.crop.height, 600);
    
    // Test panorama mode
    props.panoramaMode = true;
    props.panOffset = 500;
    
    TEST_ASSERT_TRUE(props.panoramaMode);
    TEST_ASSERT_EQ(props.panOffset, 500);
    
    return true;
}

