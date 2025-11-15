#include "TestFramework.h"
#include "../layer/LayerManager.h"
#include "../layer/VideoLayer.h"
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

bool test_LayerManager_AddLayer() {
    LayerManager manager;
    
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    layer->setLayerId(1);
    
    int id = manager.addLayer(std::move(layer));
    TEST_ASSERT_EQ(id, 1);
    TEST_ASSERT_EQ(manager.getLayerCount(), 1);
    
    VideoLayer* retrieved = manager.getLayer(1);
    TEST_ASSERT_TRUE(retrieved != nullptr);
    TEST_ASSERT_EQ(retrieved->getLayerId(), 1);
    
    return true;
}

bool test_LayerManager_RemoveLayer() {
    LayerManager manager;
    
    auto layer1 = std::make_unique<VideoLayer>();
    layer1->setInputSource(std::make_unique<MockInputSource>());
    layer1->setLayerId(1);
    
    auto layer2 = std::make_unique<VideoLayer>();
    layer2->setInputSource(std::make_unique<MockInputSource>());
    layer2->setLayerId(2);
    
    manager.addLayer(std::move(layer1));
    manager.addLayer(std::move(layer2));
    
    TEST_ASSERT_EQ(manager.getLayerCount(), 2);
    
    bool removed = manager.removeLayer(1);
    TEST_ASSERT_TRUE(removed);
    TEST_ASSERT_EQ(manager.getLayerCount(), 1);
    TEST_ASSERT_TRUE(manager.getLayer(1) == nullptr);
    TEST_ASSERT_TRUE(manager.getLayer(2) != nullptr);
    
    return true;
}

bool test_LayerManager_ZOrder() {
    LayerManager manager;
    
    auto layer1 = std::make_unique<VideoLayer>();
    layer1->setInputSource(std::make_unique<MockInputSource>());
    layer1->properties().zOrder = 10;
    
    auto layer2 = std::make_unique<VideoLayer>();
    layer2->setInputSource(std::make_unique<MockInputSource>());
    layer2->properties().zOrder = 5;
    
    auto layer3 = std::make_unique<VideoLayer>();
    layer3->setInputSource(std::make_unique<MockInputSource>());
    layer3->properties().zOrder = 15;
    
    manager.addLayer(std::move(layer1));
    manager.addLayer(std::move(layer2));
    manager.addLayer(std::move(layer3));
    
    auto sorted = manager.getLayersSortedByZOrder();
    TEST_ASSERT_EQ(sorted.size(), 3);
    TEST_ASSERT_EQ(sorted[0]->properties().zOrder, 5);
    TEST_ASSERT_EQ(sorted[1]->properties().zOrder, 10);
    TEST_ASSERT_EQ(sorted[2]->properties().zOrder, 15);
    
    return true;
}

bool test_LayerManager_DuplicateLayer() {
    LayerManager manager;
    
    auto layer = std::make_unique<VideoLayer>();
    layer->setInputSource(std::make_unique<MockInputSource>());
    layer->setLayerId(1);
    layer->properties().opacity = 0.5f;
    layer->properties().zOrder = 5;
    
    int id1 = manager.addLayer(std::move(layer));
    TEST_ASSERT_EQ(id1, 1);
    
    int id2 = manager.duplicateLayer(1);
    TEST_ASSERT_TRUE(id2 > 0);
    TEST_ASSERT_EQ(manager.getLayerCount(), 2);
    
    VideoLayer* original = manager.getLayer(1);
    VideoLayer* duplicate = manager.getLayer(id2);
    TEST_ASSERT_TRUE(original != nullptr);
    TEST_ASSERT_TRUE(duplicate != nullptr);
    TEST_ASSERT_EQ(duplicate->properties().opacity, 0.5f);
    TEST_ASSERT_EQ(duplicate->properties().zOrder, 5);
    
    return true;
}

bool test_LayerManager_Reorder() {
    LayerManager manager;
    
    auto layer1 = std::make_unique<VideoLayer>();
    layer1->setInputSource(std::make_unique<MockInputSource>());
    layer1->setLayerId(1);
    layer1->properties().zOrder = 1;
    
    auto layer2 = std::make_unique<VideoLayer>();
    layer2->setInputSource(std::make_unique<MockInputSource>());
    layer2->setLayerId(2);
    layer2->properties().zOrder = 2;
    
    manager.addLayer(std::move(layer1));
    manager.addLayer(std::move(layer2));
    
    bool moved = manager.moveLayerToTop(1);
    TEST_ASSERT_TRUE(moved);
    
    auto sorted = manager.getLayersSortedByZOrder();
    TEST_ASSERT_EQ(sorted[0]->getLayerId(), 1);
    TEST_ASSERT_EQ(sorted[1]->getLayerId(), 2);
    
    return true;
}

