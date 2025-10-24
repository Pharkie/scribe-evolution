/**
 * @file esp32c3_mini.h
 * @brief Pin definitions for ESP32-C3-mini board
 * @author Adam Knowles
 * @date 2025
 *
 * ESP32-C3 Characteristics:
 * - Single core RISC-V 32-bit @ 160MHz
 * - GPIOs 0-21 (22 total)
 * - 2x UART, 1x I2C, 1x SPI, 6x PWM
 * - 4x RMT channels (for FastLED)
 * - Built-in USB Serial/JTAG on GPIO 18/19
 * - Strapping pins: GPIO 0, 9
 * - Built-in LED on GPIO 8
 *
 * GPIO Safety Guide:
 * - SAFE: 2, 4, 5, 6, 7, 10, 20, 21
 * - AVOID: 0 (strapping), 1 (TX USB-Serial), 3 (RX USB-Serial),
 *          8 (onboard LED), 9 (strapping), 18 (USB D-), 19 (USB D+)
 */

#ifndef ESP32C3_MINI_H
#define ESP32C3_MINI_H

// ============================================================================
// BOARD IDENTIFICATION
// ============================================================================

#define BOARD_NAME "ESP32-C3-mini"
#define BOARD_MAX_GPIO 21
#define BOARD_HAS_EFUSES false

// ============================================================================
// PIN ASSIGNMENTS
// ============================================================================

#define BOARD_LED_STRIP_PIN 20      // Safe: UART1 TX
#define BOARD_PRINTER_TX_PIN 21     // Safe: UART1 RX
#define BOARD_STATUS_LED_PIN 8      // Built-in LED (avoid for other uses)

// Button pins (button 0=GPIO4, button 1=GPIO5, button 2=GPIO6, button 3=GPIO7)
static const int BOARD_BUTTON_PINS[4] = {4, 5, 6, 7};  // All safe GPIOs

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

// Complete GPIO map for ESP32-C3
static const GPIOInfo BOARD_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Strapping pin"},
    {1, GPIO_TYPE_AVOID, "TX for UART0 (USB-Serial)"},
    {2, GPIO_TYPE_SAFE, "Safe"},
    {3, GPIO_TYPE_AVOID, "RX for UART0 (USB-Serial)"},
    {4, GPIO_TYPE_SAFE, "Safe"},
    {5, GPIO_TYPE_SAFE, "Safe"},
    {6, GPIO_TYPE_SAFE, "Safe"},
    {7, GPIO_TYPE_SAFE, "Safe"},
    {8, GPIO_TYPE_SAFE, "Status LED (onboard LED)"},
    {9, GPIO_TYPE_AVOID, "Strapping pin"},
    {10, GPIO_TYPE_SAFE, "Safe"},
    {18, GPIO_TYPE_AVOID, "USB D- (Serial/JTAG)"},
    {19, GPIO_TYPE_AVOID, "USB D+ (Serial/JTAG)"},
    {20, GPIO_TYPE_SAFE, "Safe (UART1 TX)"},
    {21, GPIO_TYPE_SAFE, "Safe (UART1 RX)"}
};

static const int BOARD_GPIO_MAP_SIZE = sizeof(BOARD_GPIO_MAP) / sizeof(BOARD_GPIO_MAP[0]);

// Strapping and avoid pins
static const int BOARD_STRAPPING_PINS[] = {0, 9};
static const int BOARD_AVOID_PINS[] = {0, 1, 3, 9, 18, 19};

// ============================================================================
// VALIDATION HELPER FUNCTIONS
// ============================================================================

// Check if GPIO number is valid for this board
inline bool isValidGPIO(int pin) {
    if (pin < 0 || pin > BOARD_MAX_GPIO) {
        return false;
    }
    for (int i = 0; i < BOARD_GPIO_MAP_SIZE; i++) {
        if (BOARD_GPIO_MAP[i].pin == pin) {
            return true;
        }
    }
    return false;
}

// Check if GPIO is safe to use (not strapping, USB, etc.)
inline bool isSafeGPIO(int pin) {
    for (int i = 0; i < BOARD_GPIO_MAP_SIZE; i++) {
        if (BOARD_GPIO_MAP[i].pin == pin) {
            return BOARD_GPIO_MAP[i].type == GPIO_TYPE_SAFE;
        }
    }
    return false;
}

// Get human-readable description of GPIO
inline const char* getGPIODescription(int pin) {
    for (int i = 0; i < BOARD_GPIO_MAP_SIZE; i++) {
        if (BOARD_GPIO_MAP[i].pin == pin) {
            return BOARD_GPIO_MAP[i].description;
        }
    }
    return "Unknown GPIO";
}

#endif // ESP32C3_MINI_H
