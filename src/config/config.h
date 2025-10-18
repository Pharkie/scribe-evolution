/**
 * @file config.h
 * @brief Main configuration file for Scribe Evolution Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file orchestrates all configuration by including device-specific settings,
 * system constants, and board-specific GPIO mappings. It serves as the single
 * entry point for all configuration needs throughout the codebase.
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

// Board-specific configuration (GPIO mappings, hardware features)
// NOTE: This must be included first as system_constants depends on it
#include "boards/board_config.h"

// Device-specific configuration (WiFi, MQTT, API keys, personal settings)
#include "device_config.h"

// System constants and hardware settings
#include "system_constants.h"

// ============================================================================
// LED SUPPORT (CONDITIONAL)
// ============================================================================

// LEDs: Optional FastLED support
// Enabled via build flags (-DENABLE_LEDS=1 in platformio.ini)
#if ENABLE_LEDS
#include <core/led_config.h>
#endif

#endif