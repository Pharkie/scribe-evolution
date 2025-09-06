#include <unity.h>
#include "../src/config/config.h"

// Test ESP32-C3 button GPIO safety and configuration
void test_button_gpio_esp32c3_compatibility()
{
    // Test that all button GPIOs are within ESP32-C3 valid range (0-21)
    for (int i = 0; i < numHardwareButtons; i++)
    {
        int gpio = defaultButtons[i].gpio;
        
        // ESP32-C3 has GPIOs 0-21
        TEST_ASSERT_TRUE_MESSAGE(gpio >= 0, "Button GPIO must be >= 0");
        TEST_ASSERT_TRUE_MESSAGE(gpio <= 21, "Button GPIO must be <= 21 for ESP32-C3");
        
        // Should avoid problematic GPIOs
        TEST_ASSERT_NOT_EQUAL_MESSAGE(18, gpio, "GPIO 18 is USB D- and not available for buttons");
        TEST_ASSERT_NOT_EQUAL_MESSAGE(19, gpio, "GPIO 19 is USB D+ and not available for buttons");
    }
}

void test_button_gpio_no_strapping_pin_gpio9()
{
    // Specifically test that we're not using GPIO 9 (strapping pin that causes crashes)
    for (int i = 0; i < numHardwareButtons; i++)
    {
        int gpio = defaultButtons[i].gpio;
        TEST_ASSERT_NOT_EQUAL_MESSAGE(9, gpio, "GPIO 9 is a strapping pin that causes ESP32-C3 crashes");
    }
}

void test_button_gpio_assignments()
{
    // Test expected GPIO assignments after our fix
    TEST_ASSERT_EQUAL_MESSAGE(4, numHardwareButtons, "Should have 4 hardware buttons");
    
    // Check specific GPIO assignments (Button 4 should now be GPIO 4, not GPIO 9)
    TEST_ASSERT_EQUAL_MESSAGE(5, defaultButtons[0].gpio, "Button 1 should be GPIO 5");
    TEST_ASSERT_EQUAL_MESSAGE(6, defaultButtons[1].gpio, "Button 2 should be GPIO 6"); 
    TEST_ASSERT_EQUAL_MESSAGE(7, defaultButtons[2].gpio, "Button 3 should be GPIO 7");
    TEST_ASSERT_EQUAL_MESSAGE(4, defaultButtons[3].gpio, "Button 4 should be GPIO 4 (not GPIO 9)");
}

void test_button_configuration_consistency()
{
    // Test that button actions are properly configured
    for (int i = 0; i < numHardwareButtons; i++)
    {
        TEST_ASSERT_NOT_NULL_MESSAGE(defaultButtons[i].shortAction, "Button short action should not be null");
        TEST_ASSERT_TRUE_MESSAGE(strlen(defaultButtons[i].shortAction) > 0, "Button short action should not be empty");
        
        // All buttons should have LED effects configured
        TEST_ASSERT_NOT_NULL_MESSAGE(defaultButtons[i].shortLedEffect, "Button LED effect should not be null");
        TEST_ASSERT_TRUE_MESSAGE(strlen(defaultButtons[i].shortLedEffect) > 0, "Button LED effect should not be empty");
    }
}
