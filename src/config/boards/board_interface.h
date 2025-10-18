/**
 * @file board_interface.h
 * @brief Board abstraction interface for multi-board GPIO configuration
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file defines the common interface that all board configurations must implement.
 * It provides structures and functions for board-specific GPIO mapping, validation,
 * and hardware feature detection.
 *
 * Supported boards:
 * - ESP32-C3-mini: Standard ESP32-C3 development board
 * - ESP32-S3-mini: Standard ESP32-S3 development board
 * - ESP32-S3-custom-PCB: Custom PCB with eFuse protection circuits
 *
 * To add a new board:
 * 1. Create board_yourboard.h implementing this interface
 * 2. Add detection logic to board_config.h
 * 3. Add PlatformIO environment with -DBOARD_YOURBOARD flag
 */

#ifndef BOARD_INTERFACE_H
#define BOARD_INTERFACE_H

#include <Arduino.h>

// ============================================================================
// GPIO PIN CLASSIFICATION
// ============================================================================

/**
 * @brief GPIO pin safety classification
 */
enum GPIOType
{
    GPIO_TYPE_AVOID = 0, // Strapping pins, USB, flash - avoid using
    GPIO_TYPE_SAFE = 1   // Safe for general purpose use
};

/**
 * @brief GPIO pin information structure
 */
struct GPIOInfo
{
    int pin;                 // GPIO pin number (-1 = not connected)
    GPIOType type;           // Safety classification
    const char *description; // Human-readable description
};

// ============================================================================
// BOARD CONSTRAINTS
// ============================================================================

/**
 * @brief Board-specific hardware constraints and pin classifications
 */
struct BoardConstraints
{
    int maxGPIO;                // Maximum GPIO number for this chip
    const int *strappingPins;   // Array of strapping pins (boot config)
    int strappingPinCount;      // Number of strapping pins
    const int *avoidPins;       // Array of pins to avoid (USB, flash, etc.)
    int avoidPinCount;          // Number of avoid pins
    const GPIOInfo *gpioMap;    // Complete GPIO map for this board
    int gpioMapSize;            // Number of entries in GPIO map
};

// ============================================================================
// BOARD HARDWARE DEFAULTS
// ============================================================================

/**
 * @brief Printer hardware configuration
 */
struct PrinterPinDefaults
{
    int tx;  // UART TX pin (to printer RX) - required
    int rx;  // UART RX pin (from printer TX) - used for printer status/feedback
    int dtr; // DTR pin for flow control - optional, -1 if not used
};

/**
 * @brief Button hardware configuration
 */
struct ButtonConfig
{
    int gpio;                // GPIO pin number
    const char *shortAction; // Short press action (content type)
    const char *shortMqttTopic;
    const char *shortLedEffect;
    const char *longAction; // Long press action (content type)
    const char *longMqttTopic;
    const char *longLedEffect;
};

/**
 * @brief eFuse protection circuit pins (custom PCB feature)
 */
struct EFusePins
{
    int printer; // Printer power enable pin (-1 if not present)
    int ledStrip; // LED strip power enable pin (-1 if not present)
};

/**
 * @brief Board-specific hardware pin defaults
 */
struct BoardPinDefaults
{
    const char *boardName;           // Human-readable board name
    const char *boardIdentifier;     // Short identifier for NVS (e.g., "C3_MINI")
    PrinterPinDefaults printer;      // Printer UART pins
    int ledDataPin;                  // LED strip data pin
    int statusLedPin;                // Status LED pin (built-in LED)
    const ButtonConfig *buttons;     // Button configuration array
    int buttonCount;                 // Number of buttons
    EFusePins efuse;                 // eFuse enable pins (custom PCB only)
};

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================
// These must be implemented by each board configuration header

/**
 * @brief Check if GPIO pin exists on this board
 * @param pin GPIO pin number to check
 * @return true if pin exists, false otherwise
 */
bool isValidGPIO(int pin);

/**
 * @brief Check if GPIO pin is safe to use (not strapping/USB/flash)
 * @param pin GPIO pin number to check
 * @return true if pin is safe, false if should be avoided
 */
bool isSafeGPIO(int pin);

/**
 * @brief Get human-readable description of GPIO pin
 * @param pin GPIO pin number
 * @return Description string (e.g., "Safe", "Avoid: Strapping pin")
 */
const char *getGPIODescription(int pin);

/**
 * @brief Get board constraints (max GPIO, strapping pins, etc.)
 * @return Reference to board constraints structure
 */
const BoardConstraints &getBoardConstraints();

/**
 * @brief Get board-specific pin defaults
 * @return Reference to board pin defaults structure
 */
const BoardPinDefaults &getBoardDefaults();

#endif // BOARD_INTERFACE_H
