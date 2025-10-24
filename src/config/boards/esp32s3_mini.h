/**
 * @file esp32s3_mini.h
 * @brief Pin definitions for ESP32-S3-SuperMini board
 * @author Adam Knowles
 * @date 2025
 *
 * ESP32-S3 Characteristics:
 * - Dual core Xtensa LX7 @ 240MHz
 * - GPIOs 0-48 (49 total)
 * - 3x UART, 2x I2C, 4x SPI, 8x PWM
 * - 8x RMT channels (for FastLED)
 * - Built-in USB Serial/JTAG on GPIO 19/20
 * - Strapping pins: GPIO 0, 3, 45, 46
 * - Built-in RGB LED on GPIO 48
 *
 * Pin Assignments (from Andrius):
 * - Printer: TX=10, RX=9 (UART1 on SuperMini header)
 * - LED Strip: GPIO 1 (data line)
 * - Buttons: 5 (JOKE), 6 (RIDDLE), 7 (QUOTE), 8 (QUIZ)
 * - Status LED: GPIO 48 (built-in RGB)
 *
 * GPIO Safety Guide:
 * - SAFE: Most GPIOs 1-46 except strapping pins and USB
 * - AVOID: 0 (strapping), 3 (strapping), 19 (USB D-), 20 (USB D+),
 *          45 (strapping), 46 (strapping)
 */

#ifndef ESP32S3_MINI_H
#define ESP32S3_MINI_H

// ============================================================================
// BOARD IDENTIFICATION
// ============================================================================

#define BOARD_NAME "ESP32-S3-mini"
#define BOARD_MAX_GPIO 48
#define BOARD_HAS_EFUSES false

// ============================================================================
// PIN ASSIGNMENTS (from Andrius - ESP32-S3-SuperMini)
// ============================================================================

#define BOARD_LED_STRIP_PIN 1       // LED strip data on GPIO 1 (available, safe, on header)
#define BOARD_PRINTER_TX_PIN 10     // UART1 TX (to printer RX) - SuperMini header GPIO 1-13 only
#define BOARD_STATUS_LED_PIN 48     // Status LED on GPIO 48 (built-in RGB LED - wired internally)

// Button pins (physical hardware order: sequential GPIO 5,6,7,8)
static const int BOARD_BUTTON_PINS[4] = {5, 6, 7, 8};  // JOKE, RIDDLE, QUOTE, QUIZ

// ============================================================================
// GPIO VALIDATION DATA
// ============================================================================

// GPIO types
enum GPIOType {
    GPIO_TYPE_SAFE = 0,    // Safe to use
    GPIO_TYPE_AVOID = 1    // Should avoid (strapping, USB, etc.)
};

// GPIO information
struct GPIOInfo {
    int pin;
    GPIOType type;
    const char* description;
};

// Key GPIO map for ESP32-S3 (not exhaustive - most GPIOs are safe)
static const GPIOInfo BOARD_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Strapping pin"},
    {1, GPIO_TYPE_SAFE, "LED strip data"},
    {3, GPIO_TYPE_AVOID, "Strapping pin"},
    {5, GPIO_TYPE_SAFE, "Button 1 (JOKE)"},
    {6, GPIO_TYPE_SAFE, "Button 2 (RIDDLE)"},
    {7, GPIO_TYPE_SAFE, "Button 3 (QUOTE)"},
    {8, GPIO_TYPE_SAFE, "Button 4 (QUIZ)"},
    {9, GPIO_TYPE_SAFE, "Printer RX (UART1)"},
    {10, GPIO_TYPE_SAFE, "Printer TX (UART1)"},
    {19, GPIO_TYPE_AVOID, "USB D- (Serial/JTAG)"},
    {20, GPIO_TYPE_AVOID, "USB D+ (Serial/JTAG)"},
    {45, GPIO_TYPE_AVOID, "Strapping pin"},
    {46, GPIO_TYPE_AVOID, "Strapping pin"},
    {48, GPIO_TYPE_SAFE, "Status LED (built-in RGB)"}
};

static const int BOARD_GPIO_MAP_SIZE = sizeof(BOARD_GPIO_MAP) / sizeof(BOARD_GPIO_MAP[0]);

// Strapping and avoid pins
static const int BOARD_STRAPPING_PINS[] = {0, 3, 45, 46};
static const int BOARD_AVOID_PINS[] = {0, 3, 19, 20, 45, 46};

// ============================================================================
// VALIDATION HELPER FUNCTIONS
// ============================================================================

// Check if GPIO number is valid for this board
inline bool isValidGPIO(int pin) {
    return (pin >= 0 && pin <= BOARD_MAX_GPIO);
}

// Check if GPIO is safe to use (not strapping, USB, etc.)
inline bool isSafeGPIO(int pin) {
    // For S3, most GPIOs are safe - check against avoid list
    for (int i = 0; i < sizeof(BOARD_AVOID_PINS) / sizeof(int); i++) {
        if (pin == BOARD_AVOID_PINS[i]) {
            return false;
        }
    }
    return isValidGPIO(pin);
}

// Get human-readable description of GPIO
inline const char* getGPIODescription(int pin) {
    for (int i = 0; i < BOARD_GPIO_MAP_SIZE; i++) {
        if (BOARD_GPIO_MAP[i].pin == pin) {
            return BOARD_GPIO_MAP[i].description;
        }
    }
    return "Safe (not in map)";  // Most S3 GPIOs are safe
}

#endif // ESP32S3_MINI_H
