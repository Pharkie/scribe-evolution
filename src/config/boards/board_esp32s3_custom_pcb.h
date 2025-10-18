/**
 * @file board_esp32s3_custom_pcb.h
 * @brief ESP32-S3 custom PCB board configuration with eFuse protection
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * GPIO configuration and hardware defaults for custom ESP32-S3 PCB.
 *
 * This board extends the ESP32-S3-mini with custom eFuse protection circuits
 * for printer and LED strip power management.
 *
 * Custom PCB Features:
 * - All S3-mini capabilities (dual core, 240MHz, GPIOs 0-48)
 * - eFuse protection for printer power (GPIO 9 enable)
 * - eFuse protection for LED strip power (GPIO 10 enable)
 * - Same pin layout as S3-mini for other peripherals
 *
 * eFuse Circuit Operation:
 * - GPIO HIGH = power enabled
 * - GPIO LOW = power disabled
 * - Protects against overcurrent and short circuits
 * - Allows software control of peripheral power
 */

#ifndef BOARD_ESP32S3_CUSTOM_PCB_H
#define BOARD_ESP32S3_CUSTOM_PCB_H

// Inherit everything from S3-mini
#include "board_esp32s3_mini.h"

// ============================================================================
// CUSTOM PCB OVERRIDES
// ============================================================================

// Override board pin defaults with eFuse pins
#undef BOARD_NAME
#undef BOARD_IDENTIFIER
#undef BOARD_HAS_PRINTER_EFUSE
#undef BOARD_HAS_LED_EFUSE

#define BOARD_NAME "ESP32-S3 Custom PCB"
#define BOARD_IDENTIFIER "S3_CUSTOM_PCB"
#define BOARD_HAS_PRINTER_EFUSE true
#define BOARD_HAS_LED_EFUSE true

// eFuse enable pins (custom PCB only)
#define BOARD_PRINTER_EFUSE_PIN 9
#define BOARD_LED_EFUSE_PIN 10

// Custom PCB defaults - extends S3-mini with eFuse pins
static const BoardPinDefaults ESP32S3_CUSTOM_PCB_DEFAULTS = {
    .boardName = "ESP32-S3 Custom PCB",
    .boardIdentifier = "S3_CUSTOM_PCB",
    .printer = {
        .tx = 44,  // UART1 TX (to printer RX)
        .rx = 43,  // UART1 RX (from printer TX - status/feedback)
        .dtr = 15  // DTR for flow control (optional)
    },
    .ledDataPin = 14,      // LED strip data
    .statusLedPin = 8,     // Status LED
    .buttons = ESP32S3_DEFAULT_BUTTONS,  // Same as S3-mini
    .buttonCount = sizeof(ESP32S3_DEFAULT_BUTTONS) / sizeof(ButtonConfig),
    .efuse = {
        .printer = 9,   // Printer eFuse enable
        .ledStrip = 10  // LED strip eFuse enable
    }
};

// Wrap S3-mini's getBoardDefaults() to return custom PCB defaults instead
inline const BoardPinDefaults &getBoardDefaults()
{
    return ESP32S3_CUSTOM_PCB_DEFAULTS;
}

// ============================================================================
// EFUSE HELPER MACROS
// ============================================================================

// Convenient eFuse pin accessors
#define BOARD_EFUSE_PRINTER_PIN ESP32S3_CUSTOM_PCB_DEFAULTS.efuse.printer
#define BOARD_EFUSE_LED_PIN ESP32S3_CUSTOM_PCB_DEFAULTS.efuse.ledStrip

#endif // BOARD_ESP32S3_CUSTOM_PCB_H
