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
     * @param reason Optional description of what this state signifies (for logging)
     */
    static void setSolid(CRGB color, const char* reason = nullptr);

    /**
     * Set status LED to blink with specified color and interval
     * @param color RGB color value
     * @param intervalMs Blink interval in milliseconds (on/off cycle time)
     * @param reason Optional description of what this state signifies (for logging)
     */
    static void setBlink(CRGB color, uint16_t intervalMs, const char* reason = nullptr);

    /**
     * Turn off status LED
     * @param reason Optional description of what this state signifies (for logging)
     */
    static void off(const char* reason = nullptr);

    /**
     * Set status LED to heartbeat mode (brief flash at regular intervals)
     * @param color RGB color value for the flash
     * @param flashDurationMs Duration of flash in milliseconds
     * @param periodMs Time between flashes in milliseconds
     * @param reason Optional description of what this state signifies (for logging)
     */
    static void setHeartbeat(CRGB color, uint16_t flashDurationMs, uint16_t periodMs, const char* reason = nullptr);

    /**
     * Update status LED state (handles blinking and heartbeat)
     * Must be called repeatedly from loop() when blinking or heartbeat is active
     */
    static void update();

private:
    static CRGB led[1];              // Single LED array
    static CRGB currentColor;        // Current display color
    static uint16_t blinkInterval;   // Blink interval in ms
    static unsigned long lastBlink;  // Last blink toggle time
    static bool blinkState;          // Current blink on/off state
    static bool isBlinking;          // Whether blinking is active

    // Heartbeat state
    static bool isHeartbeat;         // Whether heartbeat mode is active
    static uint16_t heartbeatPeriod; // Time between heartbeat flashes in ms
    static uint16_t heartbeatDuration; // Duration of heartbeat flash in ms
    static unsigned long lastHeartbeat; // Last heartbeat flash time
    static bool heartbeatState;      // Current heartbeat flash on/off state

    // Previous state for logging transitions
    static CRGB previousColor;
    static bool previousIsBlinking;
    static uint16_t previousBlinkInterval;

    // Helper to log color changes
    static void logColorChange(const char* mode, CRGB newColor, uint16_t intervalMs = 0, const char* reason = nullptr);
};

#endif // BOARD_ESP32S3_CUSTOM_PCB && ENABLE_LEDS
#endif // STATUS_LED_H
