#include "TestFramework.h"
#include "../config/ConfigurationManager.h"
#include <cstring>

using namespace xjadeo;
using namespace xjadeo::test;

bool test_ConfigurationManager_Defaults() {
    ConfigurationManager config;
    
    // Test default values
    int oscPort = config.getInt("osc_port", 0);
    TEST_ASSERT_EQ(oscPort, 7000); // Default OSC port
    
    bool letterbox = config.getBool("letterbox", false);
    TEST_ASSERT_TRUE(letterbox); // Default letterbox enabled
    
    return true;
}

bool test_ConfigurationManager_SetGet() {
    ConfigurationManager config;
    
    // Test string
    config.setString("test_string", "hello");
    std::string value = config.getString("test_string", "");
    TEST_ASSERT_EQ(value, "hello");
    
    // Test int
    config.setInt("test_int", 42);
    int intValue = config.getInt("test_int", 0);
    TEST_ASSERT_EQ(intValue, 42);
    
    // Test bool
    config.setBool("test_bool", true);
    bool boolValue = config.getBool("test_bool", false);
    TEST_ASSERT_TRUE(boolValue);
    
    return true;
}

bool test_ConfigurationManager_Override() {
    ConfigurationManager config;
    
    config.setInt("test_value", 10);
    TEST_ASSERT_EQ(config.getInt("test_value", 0), 10);
    
    config.setInt("test_value", 20);
    TEST_ASSERT_EQ(config.getInt("test_value", 0), 20);
    
    return true;
}

bool test_ConfigurationManager_NonExistent() {
    ConfigurationManager config;
    
    // Non-existent values should return defaults
    std::string str = config.getString("nonexistent", "default");
    TEST_ASSERT_EQ(str, "default");
    
    int i = config.getInt("nonexistent", 99);
    TEST_ASSERT_EQ(i, 99);
    
    bool b = config.getBool("nonexistent", false);
    TEST_ASSERT_FALSE(b);
    
    return true;
}

