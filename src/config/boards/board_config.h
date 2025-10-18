/**
 * @file board_config.h
 * @brief Board configuration selector - auto-includes correct board header
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file automatically detects the board type from PlatformIO build flags
 * and includes the appropriate board-specific configuration header.
 *
 * Board Detection Hierarchy:
 * 1. Check for explicit board flag (BOARD_ESP32C3_MINI, BOARD_ESP32S3_MINI, etc.)
 * 2. Fall back to Arduino framework defines (ARDUINO_ESP32C3_DEV, etc.)
 * 3. Fall back to ESP-IDF target defines (CONFIG_IDF_TARGET_ESP32C3, etc.)
 * 4. Fail at compile time if board cannot be determined
 *
 * Usage in code:
 * - Include this file instead of individual board headers
 * - Use BOARD_* macros for compile-time configuration
 * - Use getBoardDefaults() for runtime configuration
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================================
// BOARD AUTO-DETECTION
// ============================================================================

// Custom PCB variant (highest priority - explicit flag from platformio.ini)
#if defined(BOARD_ESP32S3_CUSTOM_PCB)
    #include "board_esp32s3_custom_pcb.h"
    #define BOARD_DETECTED "ESP32-S3 Custom PCB"

// ESP32-C3-mini (explicit flag or Arduino define)
#elif defined(BOARD_ESP32C3_MINI) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
    #include "board_esp32c3_mini.h"
    #define BOARD_DETECTED "ESP32-C3-mini"

// ESP32-S3-mini (explicit flag or Arduino define)
#elif defined(BOARD_ESP32S3_MINI) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(ARDUINO_ESP32S3_DEV)
    #include "board_esp32s3_mini.h"
    #define BOARD_DETECTED "ESP32-S3-mini"

// Unknown board - compile-time error
#else
    #error "Unsupported board! Please add board configuration or set BOARD_* flag in platformio.ini"
    #error "Supported boards: BOARD_ESP32C3_MINI, BOARD_ESP32S3_MINI, BOARD_ESP32S3_CUSTOM_PCB"
#endif

// ============================================================================
// RUNTIME BOARD VALIDATION
// ============================================================================

#include <esp_chip_info.h>

/**
 * @brief Validate that firmware matches hardware at runtime
 * @return true if match confirmed, false if mismatch detected
 *
 * This function checks the actual chip model against the compiled board type
 * and warns if there's a mismatch (e.g., S3 firmware on C3 hardware).
 */
inline bool validateBoardMatch()
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    bool match = true;
    const char *detectedChip = "Unknown";
    const char *expectedChip = "Unknown";

    // Determine actual chip
    switch (chip_info.model)
    {
        case CHIP_ESP32C3:
            detectedChip = "ESP32-C3";
            #if !defined(BOARD_ESP32C3_MINI)
                match = false;
                expectedChip = BOARD_NAME;
            #endif
            break;

        case CHIP_ESP32S3:
            detectedChip = "ESP32-S3";
            #if !defined(BOARD_ESP32S3_MINI_H) && !defined(BOARD_ESP32S3_CUSTOM_PCB_H)
                match = false;
                expectedChip = BOARD_NAME;
            #endif
            break;

        default:
            detectedChip = "Unsupported";
            match = false;
            break;
    }

    if (!match)
    {
        Serial.println("╔════════════════════════════════════════════════════════════╗");
        Serial.println("║  ⚠️  BOARD MISMATCH DETECTED  ⚠️                            ║");
        Serial.println("╠════════════════════════════════════════════════════════════╣");
        Serial.printf("║  Detected Hardware: %-38s ║\n", detectedChip);
        Serial.printf("║  Compiled For:      %-38s ║\n", BOARD_NAME);
        Serial.println("║                                                            ║");
        Serial.println("║  This firmware may not work correctly!                     ║");
        Serial.println("║  Please flash the correct firmware for your board.        ║");
        Serial.println("╚════════════════════════════════════════════════════════════╝");
        Serial.println();
    }

    return match;
}

/**
 * @brief Get human-readable board information string
 * @return Board info string for logging/display
 */
inline String getBoardInfo()
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    String info = "Board: ";
    info += BOARD_NAME;
    info += " (";

    switch (chip_info.model)
    {
        case CHIP_ESP32C3:
            info += "ESP32-C3";
            break;
        case CHIP_ESP32S3:
            info += "ESP32-S3";
            break;
        default:
            info += "Unknown";
            break;
    }

    info += ", ";
    info += chip_info.cores;
    info += " core";
    if (chip_info.cores > 1) info += "s";
    info += ", ";
    info += ESP.getCpuFreqMHz();
    info += "MHz)";

    return info;
}

#endif // BOARD_CONFIG_H
