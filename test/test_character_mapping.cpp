/**
 * @file test_character_mapping.cpp
 * @brief Unit tests for character mapping functionality
 */

#include <unity.h>
#include <Arduino.h>
#include "../src/utils/character_mapping.h"

void test_basic_ascii_characters()
{
    // Test basic ASCII characters pass through unchanged
    String input = "Hello World 123!";
    String result = cleanString(input);

    // Should contain the basic text (might be cleaned but should be recognizable)
    TEST_ASSERT_TRUE(result.indexOf("Hello") != -1);
    TEST_ASSERT_TRUE(result.indexOf("World") != -1);
    TEST_ASSERT_TRUE(result.indexOf("123") != -1);
}

void test_special_character_cleaning()
{
    // Test that special characters are cleaned or replaced
    String input = "Test™©®";
    String result = cleanString(input);

    // Should have some output and contain "Test"
    TEST_ASSERT_TRUE(result.length() > 0);
    TEST_ASSERT_TRUE(result.indexOf("Test") != -1);
}

void test_string_mapping_basic()
{
    String input = "Hello World";
    String result = cleanString(input);
    TEST_ASSERT_TRUE(result.indexOf("Hello") != -1);
    TEST_ASSERT_TRUE(result.indexOf("World") != -1);
}

void test_string_mapping_with_special_chars()
{
    String input = "Hello™World©";
    String result = cleanString(input);

    // Should still contain the basic text
    TEST_ASSERT_TRUE(result.indexOf("Hello") != -1);
    TEST_ASSERT_TRUE(result.indexOf("World") != -1);

    // Should have some output
    TEST_ASSERT_TRUE(result.length() > 0);
}

void test_empty_string_mapping()
{
    String input = "";
    String result = cleanString(input);
    TEST_ASSERT_EQUAL_STRING("", result.c_str());
}

void test_unicode_characters()
{
    // Test various Unicode characters that might appear in messages
    String input = "Café naïve résumé";
    String result = cleanString(input);

    // Should have some output (not empty)
    TEST_ASSERT_TRUE(result.length() > 0);

    // Basic Latin characters should remain or be recognizable
    TEST_ASSERT_TRUE(result.indexOf("Caf") != -1);
}

void test_long_string_mapping()
{
    // Test with a longer string to ensure no buffer overflows
    String input = "";
    for (int i = 0; i < 50; i++)
    {
        input += "Test" + String(i) + " ";
    }

    String result = cleanString(input);
    TEST_ASSERT_TRUE(result.length() > 0);
    TEST_ASSERT_TRUE(result.indexOf("Test0") != -1);
}

void run_character_mapping_tests()
{
    RUN_TEST(test_basic_ascii_characters);
    RUN_TEST(test_special_character_cleaning);
    RUN_TEST(test_string_mapping_basic);
    RUN_TEST(test_string_mapping_with_special_chars);
    RUN_TEST(test_empty_string_mapping);
    RUN_TEST(test_unicode_characters);
    RUN_TEST(test_long_string_mapping);
}
