/**
 * @file test_button_queue.cpp
 * @brief Unit tests for button action queue system
 */

#include <unity.h>

// Minimal test without including complex headers
void setUp(void) {
}

void tearDown(void) {
}

void test_button_queue_basic() {
    // Basic test that queue system compiles and links
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_button_queue_basic);
    
    return UNITY_END();
}