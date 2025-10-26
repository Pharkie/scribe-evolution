/**
 * @file StatusLed.h
 * @brief WS2812 RGB status LED controller for custom PCB
 * @author Adam Knowles
 * @date 2025
 *
 * Provides simple interface for controlling single WS2812 RGB LED
 * used for WiFi status indication on custom PCB.
 *
 * Only compiled when BOARD_ESP32S3_CUSTOM_PCB and ENABLE_LEDS are both defined.
 */

#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <FastLED.h>

// Only compiled for custom PCB with LED support
#if defined(BOARD_ESP32S3_CUSTOM_PCB) && ENABLE_LEDS

class StatusLed {
public:
    /**
     * Initialize the status LED hardware
     * Must be called once during setup
     */
    static void begin();

    /**
     * Set status LED to solid color
     * @param color RGB color value
     */
    static void setSolid(CRGB color);

    /**
     * Set status LED to blink with specified color and interval
     * @param color RGB color value
     * @param intervalMs Blink interval in milliseconds (on/off cycle time)
     */
    static void setBlink(CRGB color, uint16_t intervalMs);

    /**
     * Turn off status LED
     */
    static void off();

    /**
     * Update status LED state (handles blinking)
     * Must be called repeatedly from loop() when blinking is active
     */
    static void update();

private:
    static CRGB led[1];              // Single LED array
    static CRGB currentColor;        // Current display color
    static uint16_t blinkInterval;   // Blink interval in ms
    static unsigned long lastBlink;  // Last blink toggle time
    static bool blinkState;          // Current blink on/off state
    static bool isBlinking;          // Whether blinking is active
};

#endif // BOARD_ESP32S3_CUSTOM_PCB && ENABLE_LEDS
#endif // STATUS_LED_H
