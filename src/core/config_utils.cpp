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
 */

#include "config_utils.h"
#include <config/config.h>
#include "config_loader.h"
#include "logging.h"
#include <Arduino.h>

// This file contains implementations that cannot be inlined
// Most functions are inline in the header file

void validateConfig()
{
    if (!loadRuntimeConfig())
    {
        // First-time startup: Loading default configuration from config.h
    }

    // Validate board match (hardware vs firmware)
    // if (!validateBoardMatch())
    {
        Serial.println("[BOOT] ⚠️  Board mismatch detected - see warning above");
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
    const RuntimeConfig &config = getRuntimeConfig();
    // const BoardConstraints &constraints = getBoardConstraints();
    int usedGpios[64]; // Support up to 64 GPIOs (ESP32-S3 has up to 48)
    int usedCount = 0;
    bool gpioConflict = false;

    // Add button GPIOs
    for (int i = 0; i < numHardwareButtons; i++)
    {
        usedGpios[usedCount++] = config.buttonGpios[i];
    }

    // Add status LED GPIO
    usedGpios[usedCount++] = statusLEDPin;

#if ENABLE_LEDS
    // Add LED strip GPIO from runtime config
    usedGpios[usedCount++] = config.ledPin;
#endif

    // Check for duplicate GPIOs
    for (int i = 0; i < usedCount && !gpioConflict; i++)
    {
        for (int j = i + 1; j < usedCount; j++)
        {
            if (usedGpios[i] == usedGpios[j])
            {
                Serial.printf("❌ GPIO CONFLICT: GPIO %d is used multiple times!\n", usedGpios[i]);
#if ENABLE_LEDS
                Serial.println("  Check button configurations, status LED, and LED strip GPIO assignments");
#else
                Serial.println("  Check button configurations and status LED GPIO assignments");
#endif
                gpioConflict = true;
                break;
            }
        }
    }

    // Validate GPIO ranges (board-specific)
    for (int i = 0; i < usedCount && !gpioConflict; i++)
    {
        // Check if GPIO is valid for this board
        if (!isValidGPIO(usedGpios[i]))
        {
            Serial.printf("❌ Invalid GPIO %d: %s only supports GPIOs 0-%d\n",
                         usedGpios[i], BOARD_NAME, BOARD_MAX_GPIO);
            gpioConflict = true;
        }

        // Warn about unsafe GPIOs (strapping, USB, flash, etc.)
        if (!isSafeGPIO(usedGpios[i]))
        {
            Serial.printf("⚠️  GPIO %d warning: %s\n", usedGpios[i], getGPIODescription(usedGpios[i]));
        }
    }

    if (gpioConflict)
    {
        Serial.println("[BOOT] ⚠️  Hardware: GPIO conflicts detected");
    }
    else
    {
        Serial.printf("[BOOT] ✅ Hardware: %d GPIOs validated on %s\n", usedCount, BOARD_NAME);
    }
}

void logGPIOUsageSummary()
{
    LOG_VERBOSE("BOOT", "📍 GPIO Usage Summary (Board: %s):", BOARD_NAME);

    // Get current configuration
    const RuntimeConfig &config = getRuntimeConfig();

    // Button GPIOs
    LOG_VERBOSE("BOOT", "  Buttons:");
    for (int i = 0; i < numHardwareButtons; i++)
    {
        int gpio = config.buttonGpios[i];
        LOG_VERBOSE("BOOT", "    GPIO %d: Button %d (%s) - %s",
                      gpio, i + 1, config.buttonShortActions[i].c_str(), getGPIODescription(gpio));
    }

    // Status LED GPIO
    LOG_VERBOSE("BOOT", "  Status LED:");
    LOG_VERBOSE("BOOT", "    GPIO %d: Status LED - %s", statusLEDPin, getGPIODescription(statusLEDPin));

// LED Strip GPIO (if LEDs enabled)
#if ENABLE_LEDS
    LOG_VERBOSE("BOOT", "  LED Strip:");
    LOG_VERBOSE("BOOT", "    GPIO %d: LED Strip - %s", config.ledPin, getGPIODescription(config.ledPin));
#endif

    // Printer GPIO
    LOG_VERBOSE("BOOT", "  Printer:");
    LOG_VERBOSE("BOOT", "    GPIO %d: Printer TX - %s", config.printerTxPin, getGPIODescription(config.printerTxPin));
}
