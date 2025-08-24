/**
 * @file test_memo_handler.cpp
 * @brief Unit tests for memo handler functionality
 * @author Adam Knowles
 * @date 2025
 */

#include <unity.h>
#include "../src/content/memo_handler.h"
#include "../src/utils/time_utils.h"

void setUp() {
    // Initialize random seed for consistent testing
    randomSeed(12345);
}

void tearDown() {
    // Clean up after each test
}

void test_simple_placeholders() {
    // Test basic placeholder expansion
    String testMemo = "Today is [weekday] at [time]";
    String result = processMemoPlaceholders(testMemo);
    
    // Should contain no brackets after processing
    TEST_ASSERT_TRUE(result.indexOf('[') == -1);
    TEST_ASSERT_TRUE(result.indexOf(']') == -1);
    TEST_ASSERT_TRUE(result.length() > testMemo.length());
}

void test_pick_placeholder() {
    // Test pick placeholder with multiple options
    String options = "apple|banana|cherry";
    String result = processPickPlaceholder(options);
    
    TEST_ASSERT_TRUE(result == "apple" || result == "banana" || result == "cherry");
    TEST_ASSERT_TRUE(result.length() > 0);
}

void test_pick_placeholder_single() {
    // Test pick placeholder with single option
    String options = "only_option";
    String result = processPickPlaceholder(options);
    
    TEST_ASSERT_EQUAL_STRING("only_option", result.c_str());
}

void test_pick_placeholder_empty() {
    // Test pick placeholder with empty options
    String options = "";
    String result = processPickPlaceholder(options);
    
    TEST_ASSERT_EQUAL_STRING("???", result.c_str());
}

void test_dice_placeholder() {
    // Test dice placeholder
    String result = processDicePlaceholder(6);
    int value = result.toInt();
    
    TEST_ASSERT_TRUE(value >= 1 && value <= 6);
}

void test_dice_placeholder_custom() {
    // Test dice placeholder with custom sides
    String result = processDicePlaceholder(20);
    int value = result.toInt();
    
    TEST_ASSERT_TRUE(value >= 1 && value <= 20);
}

void test_coin_placeholder() {
    // Test coin placeholder
    String result = processCoinPlaceholder();
    
    TEST_ASSERT_TRUE(result == "Heads" || result == "Tails");
}

void test_complex_memo_with_multiple_placeholders() {
    // Test memo with multiple different placeholders
    String testMemo = "Roll: [dice:6], Choice: [pick:A|B|C], Flip: [coin]";
    String result = processMemoPlaceholders(testMemo);
    
    // Should contain no brackets after processing
    TEST_ASSERT_TRUE(result.indexOf('[') == -1);
    TEST_ASSERT_TRUE(result.indexOf(']') == -1);
    
    // Should contain expected text parts
    TEST_ASSERT_TRUE(result.indexOf("Roll:") != -1);
    TEST_ASSERT_TRUE(result.indexOf("Choice:") != -1);
    TEST_ASSERT_TRUE(result.indexOf("Flip:") != -1);
}

void test_unknown_placeholder() {
    // Test unknown placeholder - should remain unchanged
    String testMemo = "Unknown: [unknown_placeholder]";
    String result = processMemoPlaceholders(testMemo);
    
    TEST_ASSERT_TRUE(result.indexOf("[unknown_placeholder]") != -1);
}

void test_malformed_placeholder() {
    // Test malformed placeholder (no closing bracket)
    String testMemo = "Malformed: [no_closing_bracket and more text";
    String result = processMemoPlaceholders(testMemo);
    
    // Should remain unchanged since no closing bracket
    TEST_ASSERT_EQUAL_STRING("Malformed: [no_closing_bracket and more text", result.c_str());
}

void test_nested_brackets() {
    // Test nested brackets (should not cause issues)
    String testMemo = "Test: [pick:option[1]|option[2]]";
    String result = processMemoPlaceholders(testMemo);
    
    // Should process the outer placeholder
    TEST_ASSERT_TRUE(result.indexOf("option[1]") != -1 || result.indexOf("option[2]") != -1);
}

void run_memo_handler_tests() {
    RUN_TEST(test_simple_placeholders);
    RUN_TEST(test_pick_placeholder);
    RUN_TEST(test_pick_placeholder_single);
    RUN_TEST(test_pick_placeholder_empty);
    RUN_TEST(test_dice_placeholder);
    RUN_TEST(test_dice_placeholder_custom);
    RUN_TEST(test_coin_placeholder);
    RUN_TEST(test_complex_memo_with_multiple_placeholders);
    RUN_TEST(test_unknown_placeholder);
    RUN_TEST(test_malformed_placeholder);
    RUN_TEST(test_nested_brackets);
}