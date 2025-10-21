/**
 * @file board_pins.h
 * @brief Board-specific pin configuration selector
 * @author Adam Knowles
 * @date 2025
 *
 * Includes the appropriate board definition file based on platformio.ini build flags
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

// ============================================================================
// BOARD TYPE DETECTION (set by platformio.ini build flags)
// ============================================================================

#if defined(BOARD_ESP32C3_MINI)
    #include "boards/esp32c3_mini.h"
#elif defined(BOARD_ESP32S3_MINI)
    #include "boards/esp32s3_mini.h"
#elif defined(BOARD_ESP32S3_CUSTOM_PCB)
    #include "boards/esp32s3_custom_pcb.h"
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
