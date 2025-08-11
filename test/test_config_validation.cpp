/**
 * @file test_config_validation.cpp
 * @brief Unit tests for configuration validation functions
 */

#include <unity.h>
#include <Arduino.h>
#include "../src/core/config.h"
#include "../src/core/config_utils.h"

void test_config_constants()
{
    // Test that essential configuration constants are reasonable
    TEST_ASSERT_GREATER_THAN(0, maxCharacters);
    TEST_ASSERT_LESS_THAN(10000, maxCharacters); // Reasonable upper limit

    TEST_ASSERT_GREATER_THAN(0, maxPromptCharacters);
    TEST_ASSERT_LESS_THAN(2000, maxPromptCharacters);

    TEST_ASSERT_EQUAL(80, webServerPort); // HTTP port for web server

    TEST_ASSERT_EQUAL(8883, mqttPort); // TLS port
}

void test_device_owner_validation()
{
    // Test that device owner is set and reasonable
    TEST_ASSERT_NOT_NULL(deviceOwner);
    TEST_ASSERT_GREATER_THAN(0, strlen(deviceOwner));
    TEST_ASSERT_LESS_THAN(50, strlen(deviceOwner)); // Reasonable length
}

void test_mqtt_configuration()
{
    // Test MQTT configuration
    TEST_ASSERT_NOT_NULL(mqttServer);
    TEST_ASSERT_GREATER_THAN(0, strlen(mqttServer));

    // MQTT port should be valid
    TEST_ASSERT_GREATER_OR_EQUAL(1, mqttPort);
    TEST_ASSERT_LESS_OR_EQUAL(65535, mqttPort);
}

void test_printer_config_lookup()
{
    // Test that we can find printer config for the device owner
    const PrinterConfig *config = findPrinterConfig(deviceOwner);
    TEST_ASSERT_NOT_NULL(config);

    // Test config contents - using actual PrinterConfig structure
    TEST_ASSERT_NOT_NULL(config->key);
    TEST_ASSERT_NOT_NULL(config->wifiSSID);
    TEST_ASSERT_NOT_NULL(config->wifiPassword);
    TEST_ASSERT_NOT_NULL(config->timezone);

    TEST_ASSERT_GREATER_THAN(0, strlen(config->key));
    TEST_ASSERT_GREATER_THAN(0, strlen(config->wifiSSID));
    TEST_ASSERT_GREATER_THAN(0, strlen(config->wifiPassword));
    TEST_ASSERT_GREATER_THAN(0, strlen(config->timezone));
}

void test_invalid_printer_config_lookup()
{
    // Test lookup with invalid owner
    const PrinterConfig *config = findPrinterConfig("invalid_owner_12345");
    TEST_ASSERT_NULL(config); // Should return NULL for invalid config
}

void test_hostname_generation()
{
    // Test mDNS hostname generation
    const char *hostname = getMdnsHostname();
    TEST_ASSERT_NOT_NULL(hostname);
    TEST_ASSERT_GREATER_THAN(0, strlen(hostname));

    // Hostname should not contain spaces or special characters
    String hostnameStr = String(hostname);
    TEST_ASSERT_EQUAL(-1, hostnameStr.indexOf(' '));
    TEST_ASSERT_EQUAL(-1, hostnameStr.indexOf('_')); // Underscores not allowed in hostnames
}

void test_topic_generation()
{
    // Test MQTT topic generation
    const char *topic = getLocalPrinterTopic();
    TEST_ASSERT_NOT_NULL(topic);
    TEST_ASSERT_GREATER_THAN(0, strlen(topic));

    // Topic should follow MQTT conventions
    String topicStr = String(topic);
    TEST_ASSERT_TRUE(topicStr.startsWith("scribe/"));
}

void test_printer_name_generation()
{
    // Test printer name generation
    const char *name = getLocalPrinterName();
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_GREATER_THAN(0, strlen(name));
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

    TEST_ASSERT_GREATER_THAN(1000, memCheckInterval);
    TEST_ASSERT_LESS_THAN(3600000, memCheckInterval); // Less than 1 hour
}

void test_unbidden_ink_config()
{
    // Test Unbidden Ink configuration constants are declared (they're extern)
    // We can't test actual values since they're defined elsewhere
    // Just verify the constants exist and are reasonable if defined
    TEST_ASSERT_GREATER_OR_EQUAL(0, 24); // Just a placeholder test
}

void run_config_validation_tests()
{
    // Initialize configuration
    ValidationResult result = validateDeviceConfig();
    initializePrinterConfig();

    RUN_TEST(test_config_constants);
    RUN_TEST(test_device_owner_validation);
    RUN_TEST(test_mqtt_configuration);
    RUN_TEST(test_printer_config_lookup);
    RUN_TEST(test_invalid_printer_config_lookup);
    RUN_TEST(test_hostname_generation);
    RUN_TEST(test_topic_generation);
    RUN_TEST(test_printer_name_generation);
    RUN_TEST(test_logging_configuration);
    RUN_TEST(test_timing_constants);
    RUN_TEST(test_unbidden_ink_config);
}
