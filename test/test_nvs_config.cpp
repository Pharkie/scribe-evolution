/**
 * @file test_nvs_config.cpp
 * @brief Tests for NVS-based configuration system
 */

#include <Arduino.h>
#include <unity.h>
#include <Preferences.h>
#include "../src/core/config_loader.h"
#include "../src/config/config.h"

void clearNVSTestData(void) {
    // Clear NVS before tests
    Preferences prefs;
    if (prefs.begin("scribe-app", false)) {
        prefs.clear();
        prefs.end();
    }
}

// Schema versioning removed; initialization simply inits NVS now.
void test_nvs_initialization()
{
    clearNVSTestData();
    TEST_ASSERT_TRUE(initializeNVSConfig());
}

void test_nvs_load_defaults()
{
    // Clear NVS first
    clearNVSTestData();
    
    // Initialize with defaults
    TEST_ASSERT_TRUE(initializeNVSConfig());
    
    // Load configuration and verify defaults are loaded
    TEST_ASSERT_TRUE(loadNVSConfig());
    
    const RuntimeConfig &config = getRuntimeConfig();
    TEST_ASSERT_EQUAL_STRING(defaultDeviceOwner, config.deviceOwner.c_str());
    TEST_ASSERT_EQUAL_STRING(defaultTimezone, config.timezone.c_str());
    TEST_ASSERT_EQUAL_STRING(defaultMqttServer, config.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(defaultMqttPort, config.mqttPort);
    TEST_ASSERT_EQUAL(defaultEnableUnbiddenInk, config.unbiddenInkEnabled);
}

void test_nvs_save_and_load()
{
    // Clear NVS first
    clearNVSTestData();
    
    // Initialize with defaults
    TEST_ASSERT_TRUE(initializeNVSConfig());
    
    // Create a modified configuration
    RuntimeConfig testConfig;
    testConfig.deviceOwner = "TestDevice";
    testConfig.timezone = "America/New_York"; 
    testConfig.wifiSSID = "TestNetwork";
    testConfig.wifiPassword = "TestPassword";
    testConfig.mqttServer = "test.mqtt.server";
    testConfig.mqttPort = 8883;
    testConfig.mqttUsername = "testuser";
    testConfig.mqttPassword = "testpass";
    testConfig.chatgptApiToken = "test-token";
    // maxCharacters is a fixed system constant; not persisted
    testConfig.unbiddenInkEnabled = true;
    testConfig.unbiddenInkStartHour = 9;
    testConfig.unbiddenInkEndHour = 17;
    testConfig.unbiddenInkFrequencyMinutes = 30;
    testConfig.unbiddenInkPrompt = "Test prompt";
    
    // Set button configurations
    for (int i = 0; i < 4; i++) {
        testConfig.buttonShortActions[i] = "/api/test" + String(i);
        testConfig.buttonLongActions[i] = "/api/test-long" + String(i);
        testConfig.buttonShortMqttTopics[i] = "test/short" + String(i);
        testConfig.buttonLongMqttTopics[i] = "test/long" + String(i);
    }
    
    // Save configuration
    TEST_ASSERT_TRUE(saveNVSConfig(testConfig));
    
    // Load configuration and verify it matches
    TEST_ASSERT_TRUE(loadNVSConfig());
    const RuntimeConfig &loadedConfig = getRuntimeConfig();
    
    TEST_ASSERT_EQUAL_STRING("TestDevice", loadedConfig.deviceOwner.c_str());
    TEST_ASSERT_EQUAL_STRING("America/New_York", loadedConfig.timezone.c_str());
    TEST_ASSERT_EQUAL_STRING("TestNetwork", loadedConfig.wifiSSID.c_str());
    TEST_ASSERT_EQUAL_STRING("TestPassword", loadedConfig.wifiPassword.c_str());
    TEST_ASSERT_EQUAL_STRING("test.mqtt.server", loadedConfig.mqttServer.c_str());
    TEST_ASSERT_EQUAL_INT(8883, loadedConfig.mqttPort);
    TEST_ASSERT_EQUAL_STRING("testuser", loadedConfig.mqttUsername.c_str());
    TEST_ASSERT_EQUAL_STRING("testpass", loadedConfig.mqttPassword.c_str());
    TEST_ASSERT_EQUAL_STRING("test-token", loadedConfig.chatgptApiToken.c_str());
    TEST_ASSERT_EQUAL_INT(maxCharacters, loadedConfig.maxCharacters);
    TEST_ASSERT_EQUAL(true, loadedConfig.unbiddenInkEnabled);
    TEST_ASSERT_EQUAL_INT(9, loadedConfig.unbiddenInkStartHour);
    TEST_ASSERT_EQUAL_INT(17, loadedConfig.unbiddenInkEndHour);
    TEST_ASSERT_EQUAL_INT(30, loadedConfig.unbiddenInkFrequencyMinutes);
    TEST_ASSERT_EQUAL_STRING("Test prompt", loadedConfig.unbiddenInkPrompt.c_str());
}

void test_nvs_validation_fallbacks()
{
    // Clear NVS first
    clearNVSTestData();
    
    // Initialize NVS with some invalid values
    Preferences prefs;
    TEST_ASSERT_TRUE(prefs.begin("scribe-app", false));
    prefs.putInt("prefs_version", 1);
    prefs.putInt("mqtt_port", 99999); // Invalid port
    prefs.putInt("unbidden_start_hour", 25); // Invalid hour
    prefs.putInt("unbidden_frequency", 5); // Too low frequency
    prefs.end();
    
    // Load configuration - should fall back to defaults for invalid values
    TEST_ASSERT_TRUE(loadNVSConfig());
    const RuntimeConfig &config = getRuntimeConfig();
    
    TEST_ASSERT_EQUAL_INT(defaultMqttPort, config.mqttPort); // Should use default
    TEST_ASSERT_EQUAL_INT(defaultUnbiddenInkStartHour, config.unbiddenInkStartHour); // Should use default
    TEST_ASSERT_EQUAL_INT(defaultUnbiddenInkFrequencyMinutes, config.unbiddenInkFrequencyMinutes); // Should use default
}

// Schema migration removed; no-op test retained for suite structure
void test_nvs_schema_migration_removed()
{
    TEST_ASSERT_TRUE(true);
}

void run_nvs_config_tests()
{
    RUN_TEST(test_nvs_initialization);
    RUN_TEST(test_nvs_load_defaults);
    RUN_TEST(test_nvs_save_and_load);
    RUN_TEST(test_nvs_validation_fallbacks);
    RUN_TEST(test_nvs_schema_migration_removed);
}
