/**
 * @file board_esp32s3_supermini.h
 * @brief ESP32-S3-SuperMini board configuration
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * GPIO configuration and hardware defaults for ESP32-S3-SuperMini development board.
 *
 * ESP32-S3-SuperMini Board Characteristics:
 * - Dual core Xtensa LX7 @ 240MHz
 * - GPIOs 0-48 (49 total on chip)
 * - **ONLY GPIO 1-13 exposed on solderable headers**
 * - GPIOs >13 are NOT routed to headers (cannot be used)
 * - USB D−/D+ use GPIO 19/20 (avoid if USB enabled)
 * - Flash/PSRAM signals GPIO 8-11 (not safe to repurpose)
 * - GPIO 0 is boot-strap pin (do not drive at reset)
 *
 * Pin Configuration (SuperMini constraints):
 * - Hardware buttons: GPIO 5, 6, 7, 8 (sequential layout on headers)
 * - Printer UART: TX=GPIO 10, RX=GPIO 9 (UART1 on headers)
 * - Status LED: GPIO 48 (built-in RGB LED - wired internally)
 * - LED strip data: GPIO 1 (on header)
 * - No eFuse circuits on SuperMini
 */

#ifndef BOARD_ESP32S3_SUPERMINI_H
#define BOARD_ESP32S3_SUPERMINI_H

#include "board_interface.h"

// ============================================================================
// GPIO MAP (SuperMini-specific: Only GPIO 1-13 + 48 are physically accessible)
// ============================================================================

static const GPIOInfo ESP32S3_GPIO_MAP[] = {
    // GPIO 0: Strapping pin - avoid
    {0, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},

    // GPIO 1-13: Exposed on solderable headers - SAFE for user configuration
    {1, GPIO_TYPE_SAFE, "Safe (on header)"},
    {2, GPIO_TYPE_SAFE, "Safe (on header)"},
    {3, GPIO_TYPE_SAFE, "Safe (on header)"},
    {4, GPIO_TYPE_SAFE, "Safe (on header)"},
    {5, GPIO_TYPE_SAFE, "Safe (on header)"},
    {6, GPIO_TYPE_SAFE, "Safe (on header)"},
    {7, GPIO_TYPE_SAFE, "Safe (on header)"},
    {8, GPIO_TYPE_SAFE, "Safe (on header)"},
    {9, GPIO_TYPE_SAFE, "Safe (on header)"},
    {10, GPIO_TYPE_SAFE, "Safe (on header)"},
    {11, GPIO_TYPE_SAFE, "Safe (on header)"},
    {12, GPIO_TYPE_SAFE, "Safe (on header)"},
    {13, GPIO_TYPE_SAFE, "Safe (on header)"},

    // GPIO 14-18: NOT exposed on SuperMini headers - UNAVAILABLE
    {14, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {15, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {16, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {17, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {18, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},

    // GPIO 19-20: USB - avoid
    {19, GPIO_TYPE_AVOID, "Avoid: USB D- (OTG)"},
    {20, GPIO_TYPE_AVOID, "Avoid: USB D+ (OTG)"},

    // GPIO 21: NOT exposed on SuperMini headers - UNAVAILABLE
    {21, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},

    // GPIO 26-32: Flash SPI - avoid
    {26, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {27, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {28, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {29, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {30, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {31, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},
    {32, GPIO_TYPE_AVOID, "Avoid: Flash SPI"},

    // GPIO 33-44: NOT exposed on SuperMini headers - UNAVAILABLE
    {33, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {34, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {35, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {36, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {37, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {38, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {39, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {40, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {41, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {42, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {43, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},
    {44, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},

    // GPIO 45-46: Strapping pins - avoid
    {45, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {46, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},

    // GPIO 47: NOT exposed on SuperMini headers - UNAVAILABLE
    {47, GPIO_TYPE_AVOID, "Unavailable: Not on SuperMini headers"},

    // GPIO 48: Built-in RGB LED (wired internally) - SAFE
    {48, GPIO_TYPE_SAFE, "Safe (built-in RGB LED)"}
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

// Default button configuration (SuperMini - sequential GPIO 5,6,7,8)
static const ButtonConfig ESP32S3_DEFAULT_BUTTONS[] = {
    {5, "JOKE", "", "chase_single", "CHARACTER_TEST", "", "pulse"},
    {6, "RIDDLE", "", "chase_single", "", "", "pulse"},
    {7, "QUOTE", "", "chase_single", "", "", "pulse"},
    {8, "QUIZ", "", "chase_single", "", "", "pulse"}  // GPIO 8 - sequential button layout
};

// Board pin defaults
static const BoardPinDefaults ESP32S3_DEFAULTS = {
    .boardName = "ESP32-S3-SuperMini",
    .boardIdentifier = "S3_SUPERMINI",
    .printer = {
        .tx = 10,  // UART1 TX (to printer RX) - SuperMini header GPIO 1-13 only
        .rx = 9,   // UART1 RX (from printer TX) - SuperMini header GPIO 1-13 only
        .dtr = -1  // DTR not used
    },
    .ledDataPin = 1,       // LED strip data on GPIO 1 (available, safe, on header)
    .statusLedPin = 48,    // Status LED on GPIO 48 (built-in RGB LED - wired internally)
    .buttons = ESP32S3_DEFAULT_BUTTONS,
    .buttonCount = sizeof(ESP32S3_DEFAULT_BUTTONS) / sizeof(ButtonConfig),
    .efuse = {
        .printer = -1,  // No eFuse on SuperMini
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

#define BOARD_NAME "ESP32-S3-SuperMini"
#define BOARD_IDENTIFIER "S3_SUPERMINI"
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

#endif // BOARD_ESP32S3_SUPERMINI_H
