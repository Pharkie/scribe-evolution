/**
 * @file test_time_utils.cpp
 * @brief Unit tests for time utility functions
 */

#include <unity.h>
#include <Arduino.h>
#include "../src/utils/time_utils.h"
#include "../src/core/config.h"

void test_formatted_datetime()
{
    // Test that getFormattedDateTime returns a non-empty string
    String datetime = getFormattedDateTime();
    TEST_ASSERT_TRUE(datetime.length() > 0);

    // Should contain some basic date/time elements (this is a basic check)
    // Note: Without a working clock, this might return a default string
}

void test_custom_date_formatting()
{
    // Test formatCustomDate function with simple input
    // Note: This might be slow if it triggers NTP sync, but should only happen once
    Serial.println("Testing date formatting (may take time if NTP sync is needed)...");

    String result = formatCustomDate("2025-01-01");
    TEST_ASSERT_TRUE(result.length() > 0);

    // Test with empty string - should handle gracefully and not hang
    Serial.println("Testing empty date string...");
    result = formatCustomDate("");
    TEST_ASSERT_TRUE(result.length() > 0); // Should return current time

    Serial.println("Date formatting tests completed");
}

void test_timezone_setup()
{
    // Test timezone setup with real hardware and WiFi connectivity
    Serial.println("Testing setupTime() with real hardware...");

    // Call the actual timezone setup function
    setupTime();

    // Test that timezone was set up (even if NTP sync failed, timezone should be configured)
    String currentTime = getFormattedDateTime();
    TEST_ASSERT_TRUE(currentTime.length() > 0);

    // Test that we can format dates after timezone setup
    String customDate = formatCustomDate("2025-01-01");
    TEST_ASSERT_TRUE(customDate.length() > 0);

    Serial.printf("Timezone setup test passed - current time: %s\n", currentTime.c_str());
}
void test_millis_basic()
{
    // Test basic Arduino millis() function works
    unsigned long start = millis();
    delay(1); // Reduce delay to speed up tests
    unsigned long end = millis();

    TEST_ASSERT_GREATER_OR_EQUAL(start, end); // Should have advanced (even by 0ms is OK)
}

void test_time_constants_from_config()
{
    // Test that time-related constants from config are reasonable
    TEST_ASSERT_GREATER_THAN(1000, memCheckInterval);   // Should be at least 1 second
    TEST_ASSERT_GREATER_THAN(10, buttonDebounceMs);     // Should be at least 10ms
    TEST_ASSERT_GREATER_THAN(100, buttonLongPressMs);   // Should be at least 100ms
    TEST_ASSERT_GREATER_THAN(1, ntpSyncTimeoutSeconds); // Should be at least 1 second
}

void run_time_utils_tests()
{
    RUN_TEST(test_formatted_datetime);
    RUN_TEST(test_custom_date_formatting);
    RUN_TEST(test_timezone_setup);
    RUN_TEST(test_millis_basic);
    RUN_TEST(test_time_constants_from_config);
}
