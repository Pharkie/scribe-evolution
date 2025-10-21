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
#define BOARD_HAS_EFUSE true  // Custom PCB has eFuse power control

// ============================================================================
// PIN ASSIGNMENTS
// ============================================================================

#define BOARD_LED_STRIP_PIN 48      // Safe (custom PCB LED output)
#define BOARD_PRINTER_TX_PIN 43     // Safe
#define BOARD_STATUS_LED_PIN 47     // Safe (custom PCB status LED)

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

// Key GPIO map for ESP32-S3 (not exhaustive - most GPIOs are safe)
static const GPIOInfo BOARD_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Strapping pin"},
    {3, GPIO_TYPE_AVOID, "Strapping pin"},
    {4, GPIO_TYPE_SAFE, "Safe"},
    {5, GPIO_TYPE_SAFE, "Safe"},
    {6, GPIO_TYPE_SAFE, "Safe"},
    {7, GPIO_TYPE_SAFE, "Safe"},
    {19, GPIO_TYPE_AVOID, "USB D- (Serial/JTAG)"},
    {20, GPIO_TYPE_AVOID, "USB D+ (Serial/JTAG)"},
    {43, GPIO_TYPE_SAFE, "Safe (Printer TX)"},
    {44, GPIO_TYPE_SAFE, "Safe"},
    {45, GPIO_TYPE_AVOID, "Strapping pin"},
    {46, GPIO_TYPE_AVOID, "Strapping pin"},
    {47, GPIO_TYPE_SAFE, "Safe (Status LED)"},
    {48, GPIO_TYPE_SAFE, "Safe (LED strip)"}
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
