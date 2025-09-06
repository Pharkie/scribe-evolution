/**
 * @file test_config_validation.cpp
 * @brief Unit tests for configuration validation functions
 */

#include <unity.h>
#include <Arduino.h>
#include "../src/config/config.h"

void test_config_constants()
{
    // Test that essential configuration constants are reasonable
    TEST_ASSERT_GREATER_THAN(0, maxCharacters);
    TEST_ASSERT_LESS_THAN(10000, maxCharacters); // Reasonable upper limit

    TEST_ASSERT_GREATER_THAN(0, maxPromptCharacters);
    TEST_ASSERT_LESS_THAN(2000, maxPromptCharacters);

    TEST_ASSERT_EQUAL(80, webServerPort); // HTTP port for web server

    TEST_ASSERT_EQUAL(8883, defaultMqttPort); // TLS port
}

void test_device_owner_validation()
{
    // Test that default device owner is set and reasonable
    TEST_ASSERT_NOT_NULL(defaultDeviceOwner);
    TEST_ASSERT_GREATER_THAN(0, strlen(defaultDeviceOwner));
    TEST_ASSERT_LESS_THAN(50, strlen(defaultDeviceOwner)); // Reasonable length
}

void test_mqtt_configuration()
{
    // Test MQTT configuration defaults
    TEST_ASSERT_NOT_NULL(defaultMqttServer);
    TEST_ASSERT_GREATER_THAN(0, strlen(defaultMqttServer));

    // MQTT port should be valid
    TEST_ASSERT_GREATER_OR_EQUAL(1, defaultMqttPort);
    TEST_ASSERT_LESS_OR_EQUAL(65535, defaultMqttPort);
}

void test_api_endpoints()
{
    // Test API endpoint constants
    TEST_ASSERT_NOT_NULL(jokeAPI);
    TEST_ASSERT_NOT_NULL(quoteAPI);
    TEST_ASSERT_NOT_NULL(triviaAPI);

    // Should start with https://
    String jokeUrl = String(jokeAPI);
    TEST_ASSERT_TRUE(jokeUrl.startsWith("https://"));
}

void test_hardware_configuration()
{
    // Test hardware pin configuration
    TEST_ASSERT_GREATER_THAN(0, defaultPrinterTxPin);
    TEST_ASSERT_LESS_THAN(50, defaultPrinterTxPin); // ESP32-C3 doesn't have 50+ GPIO pins

    // Test button configuration
    TEST_ASSERT_EQUAL(4, numHardwareButtons);
    TEST_ASSERT_GREATER_THAN(0, buttonDebounceMs);
    TEST_ASSERT_GREATER_THAN(buttonDebounceMs, buttonLongPressMs);
}

void test_logging_configuration()
{
    // Test that logging configuration is reasonable
    TEST_ASSERT_GREATER_OR_EQUAL(0, logLevel);
    TEST_ASSERT_LESS_THAN(10, logLevel); // Reasonable upper bound

    // At least one logging output should be enabled for testing
    bool anyLoggingEnabled = enableSerialLogging || enableFileLogging ||
                             enableMQTTLogging || enableBetterStackLogging;
    TEST_ASSERT_TRUE(anyLoggingEnabled);
}

void test_timing_constants()
{
    // Test timing-related constants
    TEST_ASSERT_GREATER_THAN(1, watchdogTimeoutSeconds);
    TEST_ASSERT_LESS_THAN(300, watchdogTimeoutSeconds); // Should be reasonable

    TEST_ASSERT_GREATER_THAN(1000, memCheckIntervalMs);
    TEST_ASSERT_LESS_THAN(3600000, memCheckIntervalMs); // Less than 1 hour
}

void test_validation_limits()
{
    // Test input validation limits
    TEST_ASSERT_GREATER_THAN(0, maxJsonPayloadSize);
    TEST_ASSERT_GREATER_THAN(0, maxMqttTopicLength);
    TEST_ASSERT_GREATER_THAN(0, maxParameterLength);

    // Test reasonable limits
    TEST_ASSERT_LESS_THAN(100000, maxJsonPayloadSize); // Under 100KB
    TEST_ASSERT_LESS_THAN(1000, maxMqttTopicLength);   // MQTT topics shouldn't be huge
}

void run_config_validation_tests()
{
    RUN_TEST(test_config_constants);
    RUN_TEST(test_device_owner_validation);
    RUN_TEST(test_mqtt_configuration);
    RUN_TEST(test_api_endpoints);
    RUN_TEST(test_hardware_configuration);
    RUN_TEST(test_logging_configuration);
    RUN_TEST(test_timing_constants);
    RUN_TEST(test_validation_limits);
}
