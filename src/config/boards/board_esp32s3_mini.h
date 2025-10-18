/**
 * @file board_esp32s3_mini.h
 * @brief ESP32-S3-mini board configuration
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * GPIO configuration and hardware defaults for ESP32-S3-mini development board.
 *
 * ESP32-S3 Characteristics:
 * - Dual core Xtensa LX7 @ 240MHz
 * - GPIOs 0-48 (49 total)
 * - 3x UART, 2x I2C, 4x SPI, 8x PWM
 * - USB OTG on GPIO 19/20
 * - Strapping pins: GPIO 0, 45, 46
 * - Flash pins: GPIO 26-37 (octal SPI flash)
 * - More RMT channels (better for LED strips)
 *
 * Pin Configuration (from Andrius):
 * - Hardware buttons: GPIO 5, 6, 7, 8 (via JST connector)
 * - LED strip data: GPIO 14
 * - Printer UART: TX=GPIO 44, RX=GPIO 43, DTR=GPIO 15
 * - Status LED: GPIO 8
 * - No eFuse circuits on standard S3-mini
 */

#ifndef BOARD_ESP32S3_MINI_H
#define BOARD_ESP32S3_MINI_H

#include "board_interface.h"

// ============================================================================
// GPIO MAP
// ============================================================================

static const GPIOInfo ESP32S3_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {1, GPIO_TYPE_SAFE, "Safe"},
    {2, GPIO_TYPE_SAFE, "Safe"},
    {3, GPIO_TYPE_SAFE, "Safe"},
    {4, GPIO_TYPE_SAFE, "Safe"},
    {5, GPIO_TYPE_SAFE, "Safe"},
    {6, GPIO_TYPE_SAFE, "Safe"},
    {7, GPIO_TYPE_SAFE, "Safe"},
    {8, GPIO_TYPE_SAFE, "Safe"},
    {9, GPIO_TYPE_SAFE, "Safe"},
    {10, GPIO_TYPE_SAFE, "Safe"},
    {11, GPIO_TYPE_SAFE, "Safe"},
    {12, GPIO_TYPE_SAFE, "Safe"},
    {13, GPIO_TYPE_SAFE, "Safe"},
    {14, GPIO_TYPE_SAFE, "Safe"},
    {15, GPIO_TYPE_SAFE, "Safe"},
    {16, GPIO_TYPE_SAFE, "Safe"},
    {17, GPIO_TYPE_SAFE, "Safe"},
    {18, GPIO_TYPE_SAFE, "Safe"},
    {19, GPIO_TYPE_AVOID, "Avoid: USB D- (OTG)"},
    {20, GPIO_TYPE_AVOID, "Avoid: USB D+ (OTG)"},
    {21, GPIO_TYPE_SAFE, "Safe"},
    {26, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {27, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {28, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {29, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {30, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {31, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {32, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {33, GPIO_TYPE_SAFE, "Safe"},
    {34, GPIO_TYPE_SAFE, "Safe"},
    {35, GPIO_TYPE_SAFE, "Safe"},
    {36, GPIO_TYPE_SAFE, "Safe"},
    {37, GPIO_TYPE_SAFE, "Safe"},
    {38, GPIO_TYPE_SAFE, "Safe"},
    {39, GPIO_TYPE_SAFE, "Safe"},
    {40, GPIO_TYPE_SAFE, "Safe"},
    {41, GPIO_TYPE_SAFE, "Safe"},
    {42, GPIO_TYPE_SAFE, "Safe"},
    {43, GPIO_TYPE_SAFE, "Safe"},
    {44, GPIO_TYPE_SAFE, "Safe"},
    {45, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {46, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {47, GPIO_TYPE_SAFE, "Safe"},
    {48, GPIO_TYPE_SAFE, "Safe"}
};

static const int ESP32S3_GPIO_MAP_SIZE = sizeof(ESP32S3_GPIO_MAP) / sizeof(ESP32S3_GPIO_MAP[0]);

// ============================================================================
// HARDWARE CONSTRAINTS
// ============================================================================

static const int ESP32S3_STRAPPING_PINS[] = {0, 45, 46};
static const int ESP32S3_AVOID_PINS[] = {0, 19, 20, 26, 27, 28, 29, 30, 31, 32, 45, 46};

static const BoardConstraints ESP32S3_CONSTRAINTS = {
    .maxGPIO = 48,
    .strappingPins = ESP32S3_STRAPPING_PINS,
    .strappingPinCount = sizeof(ESP32S3_STRAPPING_PINS) / sizeof(int),
    .avoidPins = ESP32S3_AVOID_PINS,
    .avoidPinCount = sizeof(ESP32S3_AVOID_PINS) / sizeof(int),
    .gpioMap = ESP32S3_GPIO_MAP,
    .gpioMapSize = ESP32S3_GPIO_MAP_SIZE
};

// ============================================================================
// BOARD DEFAULT PIN ASSIGNMENTS (from Andrius)
// ============================================================================

// Default button configuration (same actions as C3, different GPIOs)
static const ButtonConfig ESP32S3_DEFAULT_BUTTONS[] = {
    {5, "JOKE", "", "chase_single", "CHARACTER_TEST", "", "pulse"},
    {6, "RIDDLE", "", "chase_single", "", "", "pulse"},
    {7, "QUOTE", "", "chase_single", "", "", "pulse"},
    {8, "QUIZ", "", "chase_single", "", "", "pulse"}
};

// Board pin defaults
static const BoardPinDefaults ESP32S3_DEFAULTS = {
    .boardName = "ESP32-S3-mini",
    .boardIdentifier = "S3_MINI",
    .printer = {
        .tx = 44,  // UART1 TX
        .rx = 43,  // UART1 RX (optional)
        .dtr = 15  // DTR for flow control (optional)
    },
    .ledDataPin = 14,      // LED strip data
    .statusLedPin = 48,    // Status LED (RGB LED on DevKitC-1)
    .buttons = ESP32S3_DEFAULT_BUTTONS,
    .buttonCount = sizeof(ESP32S3_DEFAULT_BUTTONS) / sizeof(ButtonConfig),
    .efuse = {
        .printer = -1,  // No eFuse on standard S3-mini
        .ledStrip = -1
    }
};

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

inline bool isValidGPIO(int pin)
{
    for (int i = 0; i < ESP32S3_GPIO_MAP_SIZE; i++)
    {
        if (ESP32S3_GPIO_MAP[i].pin == pin)
        {
            return true;
        }
    }
    return false;
}

inline bool isSafeGPIO(int pin)
{
    for (int i = 0; i < ESP32S3_GPIO_MAP_SIZE; i++)
    {
        if (ESP32S3_GPIO_MAP[i].pin == pin)
        {
            return ESP32S3_GPIO_MAP[i].type == GPIO_TYPE_SAFE;
        }
    }
    return false;
}

inline const char *getGPIODescription(int pin)
{
    for (int i = 0; i < ESP32S3_GPIO_MAP_SIZE; i++)
    {
        if (ESP32S3_GPIO_MAP[i].pin == pin)
        {
            return ESP32S3_GPIO_MAP[i].description;
        }
    }
    return "Unknown GPIO";
}

inline const BoardConstraints &getBoardConstraints()
{
    return ESP32S3_CONSTRAINTS;
}

// Only define getBoardDefaults() if not using custom PCB variant
// Custom PCB header will provide its own implementation
#ifndef BOARD_ESP32S3_CUSTOM_PCB
inline const BoardPinDefaults &getBoardDefaults()
{
    return ESP32S3_DEFAULTS;
}
#endif

// ============================================================================
// BOARD-SPECIFIC MACROS (for conditional compilation)
// ============================================================================

#define BOARD_NAME "ESP32-S3-mini"
#define BOARD_IDENTIFIER "S3_MINI"
#define BOARD_MAX_GPIO 48
#define BOARD_HAS_PRINTER_EFUSE false
#define BOARD_HAS_LED_EFUSE false

// Convenient accessors
#define BOARD_DEFAULT_PRINTER_TX ESP32S3_DEFAULTS.printer.tx
#define BOARD_DEFAULT_PRINTER_RX ESP32S3_DEFAULTS.printer.rx
#define BOARD_DEFAULT_PRINTER_DTR ESP32S3_DEFAULTS.printer.dtr
#define BOARD_DEFAULT_LED_PIN ESP32S3_DEFAULTS.ledDataPin
#define BOARD_STATUS_LED_PIN ESP32S3_DEFAULTS.statusLedPin
#define BOARD_DEFAULT_BUTTONS ESP32S3_DEFAULT_BUTTONS
#define BOARD_BUTTON_COUNT ESP32S3_DEFAULTS.buttonCount

#endif // BOARD_ESP32S3_MINI_H
