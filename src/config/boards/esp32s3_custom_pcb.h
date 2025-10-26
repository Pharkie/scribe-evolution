/**
 * @file esp32s3_custom_pcb.h
 * @brief Pin definitions for ESP32-S3 custom PCB board
 * @author Adam Knowles
 * @date 2025
 *
 * ESP32-S3 Custom PCB Characteristics:
 * - Dual core Xtensa LX7 @ 240MHz
 * - GPIOs 0-48 (49 total)
 * - 3x UART, 2x I2C, 4x SPI, 8x PWM
 * - 8x RMT channels (for FastLED)
 * - Built-in USB Serial/JTAG on GPIO 19/20
 * - Strapping pins: GPIO 0, 3, 45, 46
 * - eFuse support for printer and LED power control
 *
 * GPIO Safety Guide:
 * - SAFE: Most GPIOs 1-46 except strapping pins and USB
 * - AVOID: 0 (strapping), 3 (strapping), 19 (USB D-), 20 (USB D+),
 *          45 (strapping), 46 (strapping)
 */

#ifndef ESP32S3_CUSTOM_PCB_H
#define ESP32S3_CUSTOM_PCB_H

// ============================================================================
// BOARD IDENTIFICATION
// ============================================================================

#define BOARD_NAME "ESP32-S3-custom-PCB"
#define BOARD_MAX_GPIO 48
#define BOARD_HAS_EFUSES true  // Custom PCB has eFuse power control

// eFuse GPIO pins for power control
#define BOARD_EFUSE_PRINTER_PIN 9
#define BOARD_EFUSE_LED_PIN 10

// ============================================================================
// PIN ASSIGNMENTS
// ============================================================================

#define BOARD_LED_STRIP_PIN 14      // Safe (custom PCB LED strip data)
#define BOARD_PRINTER_TX_PIN 43     // Safe (UART TX to printer RX)
#define BOARD_PRINTER_RX_PIN 44     // Safe (UART RX from printer TX)
#define BOARD_PRINTER_DTR_PIN 15    // Safe (DTR for hardware flow control)
#define BOARD_STATUS_LED_PIN 16     // Safe (WS2812 RGB status LED)

// Button pins (button 0=GPIO5, button 1=GPIO6, button 2=GPIO7, button 3=GPIO8)
static const int BOARD_BUTTON_PINS[4] = {5, 6, 7, 8};  // All safe GPIOs

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

// Key GPIO map for ESP32-S3 custom PCB (not exhaustive - most GPIOs are safe)
static const GPIOInfo BOARD_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Strapping pin"},
    {3, GPIO_TYPE_AVOID, "Strapping pin"},
    {5, GPIO_TYPE_SAFE, "Button 1 (JOKE)"},
    {6, GPIO_TYPE_SAFE, "Button 2 (RIDDLE)"},
    {7, GPIO_TYPE_SAFE, "Button 3 (QUOTE)"},
    {8, GPIO_TYPE_SAFE, "Button 4 (QUIZ)"},
    {9, GPIO_TYPE_SAFE, "Printer eFuse enable"},
    {10, GPIO_TYPE_SAFE, "LED strip eFuse enable"},
    {14, GPIO_TYPE_SAFE, "LED strip data"},
    {15, GPIO_TYPE_SAFE, "Printer DTR"},
    {16, GPIO_TYPE_SAFE, "Status LED (WS2812 RGB)"},
    {19, GPIO_TYPE_AVOID, "USB D- (Serial/JTAG)"},
    {20, GPIO_TYPE_AVOID, "USB D+ (Serial/JTAG)"},
    {43, GPIO_TYPE_SAFE, "Printer TX"},
    {44, GPIO_TYPE_SAFE, "Printer RX"},
    {45, GPIO_TYPE_AVOID, "Strapping pin"},
    {46, GPIO_TYPE_AVOID, "Strapping pin"}
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

#endif // ESP32S3_CUSTOM_PCB_H
