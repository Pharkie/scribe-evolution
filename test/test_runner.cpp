/**
 * @file test_runner.cpp
 * @brief Unified test runner for all test suites
 */

#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// Include core modules for global test setup
#include "../src/core/config.h"
#include "../src/core/config_utils.h"
#include "../src/core/network.h"
#include "../src/web/web_server.h"
#include "../src/core/logging.h"

// Include all test headers - external function declarations
extern void run_basic_tests();
extern void run_character_mapping_tests();
extern void run_config_validation_tests();
extern void run_web_validation_tests();
extern void run_time_utils_tests();
extern void run_endpoint_integration_tests();

// Test stubs for variables normally defined in main.cpp
String deviceBootTime = "2025-08-17T12:00:00Z"; // Test stub

// Web server instance for all tests
WebServer server(webServerPort);

// Global test state
bool globalTestEnvironmentInitialized = false;

/**
 * @brief Initialize global test environment once for all test suites
 */
void initializeGlobalTestEnvironment()
{
    if (globalTestEnvironmentInitialized)
    {
        return;
    }

    Serial.println("=== Initializing Global Test Environment ===");

    // Initialize configuration
    validateConfig();
    initializePrinterConfig();

    // Connect to WiFi
    connectToWiFi();

    // Initialize logging
    setupLogging();
    LOG_NOTICE("TEST", "Global test environment WiFi connected");

    // Initialize LittleFS
    if (!LittleFS.begin(true))
    {
        LOG_ERROR("TEST", "LittleFS Mount Failed");
        Serial.println("Warning: LittleFS mount failed - content tests may fail");
    }
    else
    {
        LOG_VERBOSE("TEST", "LittleFS mounted successfully for all tests");
    }

    // Setup web server routes
    setupWebServerRoutes(maxCharacters);

    // Start the server
    server.begin();
    LOG_NOTICE("TEST", "Web server initialized for all tests");

    globalTestEnvironmentInitialized = true;
    Serial.println("=== Global Test Environment Ready ===");
    delay(1000); // Give system time to stabilize
}

void setUp(void)
{
    // Set up before each test
}

void tearDown(void)
{
    // Clean up after each test
}

void setup()
{
    delay(2000); // Wait for serial monitor

    UNITY_BEGIN();

    // Initialize WiFi, LittleFS, and WebServer ONCE for all tests
    initializeGlobalTestEnvironment();

    // Run all test suites - all now have access to WiFi/LittleFS/WebServer
    Serial.println("=== Running Basic Framework Tests ===");
    run_basic_tests();

    Serial.println("=== Running Character Mapping Tests ===");
    run_character_mapping_tests();

    Serial.println("=== Running Config Validation Tests ===");
    run_config_validation_tests();

    Serial.println("=== Running Web Validation Tests ===");
    run_web_validation_tests();

    Serial.println("=== Running Time Utils Tests ===");
    run_time_utils_tests();

    Serial.println("=== Running Endpoint Integration Tests ===");
    run_endpoint_integration_tests();

    UNITY_END();
}

void loop()
{
    // Unity handles everything in setup()
}
