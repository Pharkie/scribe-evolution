/**
 * @file board_esp32c3_mini.h
 * @brief ESP32-C3-mini board configuration
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * GPIO configuration and hardware defaults for ESP32-C3-mini development board.
 *
 * ESP32-C3 Characteristics:
 * - Single core RISC-V 32-bit @ 160MHz
 * - GPIOs 0-21 (22 total)
 * - 2x UART, 1x I2C, 1x SPI, 6x PWM
 * - Built-in USB Serial/JTAG on GPIO 18/19
 * - Strapping pins: GPIO 0, 9
 * - Built-in LED on GPIO 8
 */

#ifndef BOARD_ESP32C3_MINI_H
#define BOARD_ESP32C3_MINI_H

#include "board_interface.h"

// ============================================================================
// GPIO MAP
// ============================================================================

static const GPIOInfo ESP32C3_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {1, GPIO_TYPE_AVOID, "Avoid: TX for UART0 (USB-Serial)"},
    {2, GPIO_TYPE_SAFE, "Safe"},
    {3, GPIO_TYPE_AVOID, "Avoid: RX for UART0 (USB-Serial)"},
    {4, GPIO_TYPE_SAFE, "Safe"},
    {5, GPIO_TYPE_SAFE, "Safe"},
    {6, GPIO_TYPE_SAFE, "Safe"},
    {7, GPIO_TYPE_SAFE, "Safe"},
    {8, GPIO_TYPE_AVOID, "Avoid: Onboard LED"},
    {9, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {10, GPIO_TYPE_SAFE, "Safe"},
    {18, GPIO_TYPE_AVOID, "Avoid: USB D- (Serial/JTAG)"},
    {19, GPIO_TYPE_AVOID, "Avoid: USB D+ (Serial/JTAG)"},
    {20, GPIO_TYPE_SAFE, "Safe (UART1 TX)"},
    {21, GPIO_TYPE_SAFE, "Safe (UART1 RX)"}
};

static const int ESP32C3_GPIO_MAP_SIZE = sizeof(ESP32C3_GPIO_MAP) / sizeof(ESP32C3_GPIO_MAP[0]);

// ============================================================================
// HARDWARE CONSTRAINTS
// ============================================================================

static const int ESP32C3_STRAPPING_PINS[] = {0, 9};
static const int ESP32C3_AVOID_PINS[] = {0, 1, 3, 8, 9, 18, 19};

static const BoardConstraints ESP32C3_CONSTRAINTS = {
    .maxGPIO = 21,
    .strappingPins = ESP32C3_STRAPPING_PINS,
    .strappingPinCount = sizeof(ESP32C3_STRAPPING_PINS) / sizeof(int),
    .avoidPins = ESP32C3_AVOID_PINS,
    .avoidPinCount = sizeof(ESP32C3_AVOID_PINS) / sizeof(int),
    .gpioMap = ESP32C3_GPIO_MAP,
    .gpioMapSize = ESP32C3_GPIO_MAP_SIZE
};

// ============================================================================
// BOARD DEFAULT PIN ASSIGNMENTS
// ============================================================================

// Default button configuration
static const ButtonConfig ESP32C3_DEFAULT_BUTTONS[] = {
    {5, "JOKE", "", "chase_single", "CHARACTER_TEST", "", "pulse"},
    {6, "RIDDLE", "", "chase_single", "", "", "pulse"},
    {7, "QUOTE", "", "chase_single", "", "", "pulse"},
    {4, "QUIZ", "", "chase_single", "", "", "pulse"}
};

// Board pin defaults
static const BoardPinDefaults ESP32C3_DEFAULTS = {
    .boardName = "ESP32-C3-mini",
    .boardIdentifier = "C3_MINI",
    .printer = {
        .tx = 21,  // UART1 TX (to printer RX)
        .rx = -1,  // UART1 RX (not connected on C3 builds - bidirectional support available if wired)
        .dtr = -1  // DTR not used
    },
    .ledDataPin = 20,
    .statusLedPin = 8, // Built-in LED
    .buttons = ESP32C3_DEFAULT_BUTTONS,
    .buttonCount = sizeof(ESP32C3_DEFAULT_BUTTONS) / sizeof(ButtonConfig),
    .efuse = {
        .printer = -1,  // No eFuse on standard C3-mini
        .ledStrip = -1
    }
};

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

inline bool isValidGPIO(int pin)
{
    for (int i = 0; i < ESP32C3_GPIO_MAP_SIZE; i++)
    {
        if (ESP32C3_GPIO_MAP[i].pin == pin)
        {
            return true;
        }
    }
    return false;
}

inline bool isSafeGPIO(int pin)
{
    for (int i = 0; i < ESP32C3_GPIO_MAP_SIZE; i++)
    {
        if (ESP32C3_GPIO_MAP[i].pin == pin)
        {
            return ESP32C3_GPIO_MAP[i].type == GPIO_TYPE_SAFE;
        }
    }
    return false;
}

inline const char *getGPIODescription(int pin)
{
    for (int i = 0; i < ESP32C3_GPIO_MAP_SIZE; i++)
    {
        if (ESP32C3_GPIO_MAP[i].pin == pin)
        {
            return ESP32C3_GPIO_MAP[i].description;
        }
    }
    return "Unknown GPIO";
}

inline const BoardConstraints &getBoardConstraints()
{
    return ESP32C3_CONSTRAINTS;
}

inline const BoardPinDefaults &getBoardDefaults()
{
    return ESP32C3_DEFAULTS;
}

// ============================================================================
// BOARD-SPECIFIC MACROS (for conditional compilation)
// ============================================================================

#define BOARD_NAME "ESP32-C3-mini"
#define BOARD_IDENTIFIER "C3_MINI"
#define BOARD_MAX_GPIO 21
#define BOARD_HAS_PRINTER_EFUSE false
#define BOARD_HAS_LED_EFUSE false

// Convenient accessors
#define BOARD_DEFAULT_PRINTER_TX ESP32C3_DEFAULTS.printer.tx
#define BOARD_DEFAULT_PRINTER_RX ESP32C3_DEFAULTS.printer.rx
#define BOARD_DEFAULT_PRINTER_DTR ESP32C3_DEFAULTS.printer.dtr
#define BOARD_DEFAULT_LED_PIN ESP32C3_DEFAULTS.ledDataPin
#define BOARD_STATUS_LED_PIN ESP32C3_DEFAULTS.statusLedPin
#define BOARD_DEFAULT_BUTTONS ESP32C3_DEFAULT_BUTTONS
#define BOARD_BUTTON_COUNT ESP32C3_DEFAULTS.buttonCount

#endif // BOARD_ESP32C3_MINI_H
