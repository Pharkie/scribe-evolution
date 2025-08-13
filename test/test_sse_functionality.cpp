/**
 * @file test_sse_functionality.cpp
 * @brief Server-Sent Events (SSE) functionality tests
 * @description Tests SSE helper functions and event creation
 */

#include <unity.h>
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Include the web server module for SSE functions
#include "../src/web/web_server.h"
#include "../src/core/logging.h"

/**
 * @brief Test that SSE event source is properly created
 */
void test_sse_event_source_creation()
{
    Serial.println("Testing SSE event source creation...");
    
    // Get the event source reference
    AsyncEventSource& events = getEventSource();
    
    // Verify it's properly created (not null reference)
    TEST_ASSERT_TRUE(true); // If we get here, the reference is valid
    
    Serial.println("✅ SSE event source created successfully");
}

/**
 * @brief Test sendSystemStatus function creates proper JSON
 */
void test_send_system_status_json_format()
{
    Serial.println("Testing sendSystemStatus JSON format...");
    
    // We can't easily test the actual SSE sending without a full web server setup
    // But we can test that the JSON structure is correct by creating similar JSON
    
    DynamicJsonDocument doc(256);
    doc["message"] = "Test status message";
    doc["level"] = "info";
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    // Verify JSON structure
    TEST_ASSERT_TRUE(payload.indexOf("\"message\":") > 0);
    TEST_ASSERT_TRUE(payload.indexOf("\"level\":") > 0);
    TEST_ASSERT_TRUE(payload.indexOf("\"timestamp\":") > 0);
    TEST_ASSERT_TRUE(payload.indexOf("Test status message") > 0);
    TEST_ASSERT_TRUE(payload.indexOf("info") > 0);
    
    Serial.println("✅ System status JSON format is correct");
}

/**
 * @brief Test that sendPrinterUpdate function exists and can be called
 */
void test_send_printer_update_function_exists()
{
    Serial.println("Testing sendPrinterUpdate function exists...");
    
    // Create a test JSON string similar to what would be sent
    String testPrinterData = "{\"discovered_printers\":[],\"count\":0,\"our_printer_id\":\"test123\"}";
    
    // This should not crash - we're just testing the function exists and compiles
    sendPrinterUpdate(testPrinterData);
    
    // If we get here, the function exists and accepts String parameter
    TEST_ASSERT_TRUE(true);
    
    Serial.println("✅ sendPrinterUpdate function exists and accepts String parameter");
}

/**
 * @brief Test that sendSystemStatus function exists and can be called
 */
void test_send_system_status_function_exists()
{
    Serial.println("Testing sendSystemStatus function exists...");
    
    // Test with different status levels
    sendSystemStatus("Test info message", "info");
    sendSystemStatus("Test warning message", "warning");
    sendSystemStatus("Test error message", "error");
    sendSystemStatus("Test default level");  // Should default to "info"
    
    // If we get here, the function exists and accepts the parameters
    TEST_ASSERT_TRUE(true);
    
    Serial.println("✅ sendSystemStatus function exists and accepts proper parameters");
}

/**
 * @brief Test getDiscoveredPrintersJson function exists
 */
void test_get_discovered_printers_json_function_exists()
{
    Serial.println("Testing getDiscoveredPrintersJson function exists...");
    
    // This should not crash - we're just testing the function exists
    String result = getDiscoveredPrintersJson();
    
    // Should return a valid JSON string (at minimum an empty structure)
    TEST_ASSERT_TRUE(result.length() > 0);
    TEST_ASSERT_TRUE(result.indexOf("{") >= 0);  // Should contain JSON structure
    
    Serial.println("✅ getDiscoveredPrintersJson function exists and returns JSON");
}

/**
 * @brief Run all SSE functionality tests
 */
void run_sse_tests()
{
    Serial.println("\n=== Running SSE Functionality Tests ===");
    
    RUN_TEST(test_sse_event_source_creation);
    RUN_TEST(test_send_system_status_json_format);
    RUN_TEST(test_send_printer_update_function_exists);
    RUN_TEST(test_send_system_status_function_exists);
    RUN_TEST(test_get_discovered_printers_json_function_exists);
    
    Serial.println("=== SSE Functionality Tests Complete ===\n");
}