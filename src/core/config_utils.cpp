/**
 * @file config_utils.cpp
 * @brief Implementation of configuration validation and utilities
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 * Based on the original Project Scribe by UrbanCircles.
 */

#include "config_utils.h"
#include "config.h"
#include "config_loader.h"
#include <Arduino.h>

// This file contains implementations that cannot be inlined
// Most functions are inline in the header file

void validateConfig()
{
    if (!loadRuntimeConfig())
    {
        // First-time startup: Loading default configuration from config.h
    }

    ValidationResult result = validateDeviceConfig();

    if (!result.isValid)
    {
        Serial.println("❌ Configuration validation FAILED:");
        Serial.print("  ERROR: ");
        Serial.println(result.errorMessage);
        // Critical configuration error - must be visible
    }

    // GPIO validation - check for conflicts between buttons, status LED, and LED strip
    int usedGpios[32]; // ESP32-C3 has GPIOs 0-21, but use larger array for safety
    int usedCount = 0;
    bool gpioConflict = false;

    // Add button GPIOs
    for (int i = 0; i < numHardwareButtons; i++)
    {
        usedGpios[usedCount++] = defaultButtons[i].gpio;
    }

    // Add status LED GPIO
    usedGpios[usedCount++] = statusLEDPin;

#if ENABLE_LEDS
    // Add LED strip GPIO (uses DEFAULT_LED_PIN from led_config.h)
    usedGpios[usedCount++] = DEFAULT_LED_PIN; // Now correctly references GPIO 1
#endif

    // Check for duplicate GPIOs
    for (int i = 0; i < usedCount && !gpioConflict; i++)
    {
        for (int j = i + 1; j < usedCount; j++)
        {
            if (usedGpios[i] == usedGpios[j])
            {
                Serial.printf("❌ GPIO CONFLICT: GPIO %d is used multiple times!\n", usedGpios[i]);
                Serial.println("  Check button configurations, status LED, and LED strip GPIO assignments");
                gpioConflict = true;
                break;
            }
        }
    }

    // Validate GPIO ranges for ESP32-C3 (GPIO 0-21)
    for (int i = 0; i < usedCount && !gpioConflict; i++)
    {
        if (usedGpios[i] < 0 || usedGpios[i] > 21)
        {
            Serial.printf("❌ Invalid GPIO %d: ESP32-C3 only supports GPIO 0-21\n", usedGpios[i]);
            gpioConflict = true;
        }

        // Warn about potentially problematic GPIOs
        if (usedGpios[i] == 0 || usedGpios[i] == 1)
        {
            Serial.printf("⚠️  GPIO %d warning: Used for UART0 (Serial) - may cause boot/programming issues\n", usedGpios[i]);
        }
        if (usedGpios[i] == 2 || usedGpios[i] == 3)
        {
            Serial.printf("⚠️  GPIO %d warning: Strapping pin - may cause boot issues if pulled wrong way\n", usedGpios[i]);
        }
    }

    if (gpioConflict)
    {
        Serial.println("⚠️  Continuing with degraded functionality due to GPIO conflicts...");
    }
    else
    {
        Serial.printf("✅ GPIO validation passed - %d GPIOs configured correctly\n", usedCount);
    }
}
