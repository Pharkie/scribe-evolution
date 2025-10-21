/**
 * @file config.h
 * @brief Main configuration file for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file orchestrates all configuration by including device-specific settings,
 * system constants, and GPIO mappings. It serves as the single entry point
 * for all configuration needs throughout the codebase.
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// FIRMWARE VERSION
// ============================================================================
#define FIRMWARE_VERSION "0.2.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// ============================================================================
// CONFIGURATION INCLUDES
// ============================================================================

// Device-specific configuration (WiFi, MQTT, API keys, personal settings)
#include "device_config.h"

// Board-specific pin configuration (NEW minimal system - must be before system_constants.h)
#include "board_pins.h"

// System constants and hardware settings (uses BOARD_* macros from board_pins.h)
#include "system_constants.h"

// GPIO pin mappings
#include "gpio_map.h"

// ============================================================================
// LED SUPPORT (CONDITIONAL)
// ============================================================================

// LEDs: Optional FastLED support
// Enabled via build flags (-DENABLE_LEDS=1 in platformio.ini)
#if ENABLE_LEDS
#include <core/led_config.h>
#endif

#endif