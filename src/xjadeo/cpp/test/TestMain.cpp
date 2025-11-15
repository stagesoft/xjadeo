#include "TestFramework.h"

// Forward declarations of test functions
extern bool test_LayerManager_AddLayer();
extern bool test_LayerManager_RemoveLayer();
extern bool test_LayerManager_ZOrder();
extern bool test_LayerManager_DuplicateLayer();
extern bool test_LayerManager_Reorder();

extern bool test_VideoLayer_PlayPause();
extern bool test_VideoLayer_Seek();
extern bool test_VideoLayer_TimeOffset();
extern bool test_VideoLayer_TimeScale();
extern bool test_VideoLayer_Wraparound();
extern bool test_VideoLayer_Reverse();
extern bool test_VideoLayer_SyncUpdate();

extern bool test_ConfigurationManager_Defaults();
extern bool test_ConfigurationManager_SetGet();
extern bool test_ConfigurationManager_Override();
extern bool test_ConfigurationManager_NonExistent();

extern bool test_Integration_LayerManagerWithMultipleLayers();
extern bool test_Integration_VideoLayerTimeScaling();
extern bool test_Integration_LayerProperties();

using namespace xjadeo::test;

int main() {
    // Register all tests
    TestFramework::instance().addTest("LayerManager_AddLayer", test_LayerManager_AddLayer);
    TestFramework::instance().addTest("LayerManager_RemoveLayer", test_LayerManager_RemoveLayer);
    TestFramework::instance().addTest("LayerManager_ZOrder", test_LayerManager_ZOrder);
    TestFramework::instance().addTest("LayerManager_DuplicateLayer", test_LayerManager_DuplicateLayer);
    TestFramework::instance().addTest("LayerManager_Reorder", test_LayerManager_Reorder);
    
    TestFramework::instance().addTest("VideoLayer_PlayPause", test_VideoLayer_PlayPause);
    TestFramework::instance().addTest("VideoLayer_Seek", test_VideoLayer_Seek);
    TestFramework::instance().addTest("VideoLayer_TimeOffset", test_VideoLayer_TimeOffset);
    TestFramework::instance().addTest("VideoLayer_TimeScale", test_VideoLayer_TimeScale);
    TestFramework::instance().addTest("VideoLayer_Wraparound", test_VideoLayer_Wraparound);
    TestFramework::instance().addTest("VideoLayer_Reverse", test_VideoLayer_Reverse);
    TestFramework::instance().addTest("VideoLayer_SyncUpdate", test_VideoLayer_SyncUpdate);
    
    TestFramework::instance().addTest("ConfigurationManager_Defaults", test_ConfigurationManager_Defaults);
    TestFramework::instance().addTest("ConfigurationManager_SetGet", test_ConfigurationManager_SetGet);
    TestFramework::instance().addTest("ConfigurationManager_Override", test_ConfigurationManager_Override);
    TestFramework::instance().addTest("ConfigurationManager_NonExistent", test_ConfigurationManager_NonExistent);
    
    TestFramework::instance().addTest("Integration_LayerManagerWithMultipleLayers", test_Integration_LayerManagerWithMultipleLayers);
    TestFramework::instance().addTest("Integration_VideoLayerTimeScaling", test_Integration_VideoLayerTimeScaling);
    TestFramework::instance().addTest("Integration_LayerProperties", test_Integration_LayerProperties);
    
    return TestFramework::instance().runAll();
}

