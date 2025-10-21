/**
 * @file board_pins.h
 * @brief Minimal board-specific pin configuration
 * @author Adam Knowles
 * @date 2025
 *
 * Simple compile-time board pin definitions for ESP32-C3, ESP32-S3-mini, and ESP32-S3-custom-PCB
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

// ============================================================================
// BOARD TYPE DETECTION (set by platformio.ini build flags)
// ============================================================================

#if defined(BOARD_ESP32C3_MINI)
    // ESP32-C3-mini (4MB flash)
    #define BOARD_NAME "ESP32-C3-mini"
    #define BOARD_LED_STRIP_PIN 20
    #define BOARD_PRINTER_TX_PIN 21
    #define BOARD_STATUS_LED_PIN 8

    static const int BOARD_BUTTON_PINS[4] = {4, 5, 6, 7};

#elif defined(BOARD_ESP32S3_MINI)
    // ESP32-S3-mini (4MB flash)
    #define BOARD_NAME "ESP32-S3-mini"
    #define BOARD_LED_STRIP_PIN 48
    #define BOARD_PRINTER_TX_PIN 43
    #define BOARD_STATUS_LED_PIN 47

    static const int BOARD_BUTTON_PINS[4] = {4, 5, 6, 7};

#elif defined(BOARD_ESP32S3_CUSTOM_PCB)
    // ESP32-S3-custom-PCB (8MB flash)
    #define BOARD_NAME "ESP32-S3-custom-PCB"
    #define BOARD_LED_STRIP_PIN 48
    #define BOARD_PRINTER_TX_PIN 43
    #define BOARD_STATUS_LED_PIN 47

    static const int BOARD_BUTTON_PINS[4] = {4, 5, 6, 7};

#else
    #error "No board type defined! Set BOARD_ESP32C3_MINI, BOARD_ESP32S3_MINI, or BOARD_ESP32S3_CUSTOM_PCB in platformio.ini"
#endif

// ============================================================================
// SIMPLE ACCESSOR FUNCTIONS
// ============================================================================

inline const char* getBoardName() {
    return BOARD_NAME;
}

inline int getButtonPin(int buttonIndex) {
    if (buttonIndex < 0 || buttonIndex >= 4) return -1;
    return BOARD_BUTTON_PINS[buttonIndex];
}

inline int getLedStripPin() {
    return BOARD_LED_STRIP_PIN;
}

inline int getPrinterTxPin() {
    return BOARD_PRINTER_TX_PIN;
}

inline int getStatusLedPin() {
    return BOARD_STATUS_LED_PIN;
}

#endif // BOARD_PINS_H
