/**
 * @file StatusLed.cpp
 * @brief WS2812 RGB status LED controller implementation
 * @author Adam Knowles
 * @date 2025
 */

#include "StatusLed.h"
#include "config/config.h"
#include "core/logging.h"

// Only compiled for custom PCB with LED support
#if defined(BOARD_ESP32S3_CUSTOM_PCB) && ENABLE_LEDS

// Static member initialization
CRGB StatusLed::led[1];
CRGB StatusLed::currentColor = CRGB::Black;
uint16_t StatusLed::blinkInterval = 0;
unsigned long StatusLed::lastBlink = 0;
bool StatusLed::blinkState = false;
bool StatusLed::isBlinking = false;

// Heartbeat state tracking
bool StatusLed::isHeartbeat = false;
uint16_t StatusLed::heartbeatPeriod = 0;
uint16_t StatusLed::heartbeatDuration = 0;
unsigned long StatusLed::lastHeartbeat = 0;
bool StatusLed::heartbeatState = false;

// Previous state tracking for logging
CRGB StatusLed::previousColor = CRGB::Black;
bool StatusLed::previousIsBlinking = false;
bool StatusLed::previousIsHeartbeat = false;
uint16_t StatusLed::previousBlinkInterval = 0;
uint16_t StatusLed::previousHeartbeatPeriod = 0;
uint16_t StatusLed::previousHeartbeatDuration = 0;

// Thread safety
SemaphoreHandle_t StatusLed::mutex = nullptr;

void StatusLed::begin()
{
    // Create mutex for thread safety (critical for dual-core ESP32-S3)
    // Must be created in begin(), NOT in static initialization (initialization order issue)
    if (mutex == nullptr)
    {
        mutex = xSemaphoreCreateMutex();
        if (mutex == nullptr)
        {
            LOG_ERROR("STATUS_LED", "Failed to create mutex");
            return;
        }
    }

    // Initialize FastLED for single WS2812 LED on status LED pin
    // Use compile-time switch based on GPIO pin number (FastLED requirement)
    #define STATUS_LED_INIT(pin) case pin: FastLED.addLeds<WS2812B, pin, GRB>(led, 1); break;

    switch (BOARD_STATUS_LED_PIN)
    {
        STATUS_LED_INIT(0)
        STATUS_LED_INIT(1)
        STATUS_LED_INIT(2)
        STATUS_LED_INIT(3)
        STATUS_LED_INIT(4)
        STATUS_LED_INIT(5)
        STATUS_LED_INIT(6)
        STATUS_LED_INIT(7)
        STATUS_LED_INIT(8)
        STATUS_LED_INIT(9)
        STATUS_LED_INIT(10)
        STATUS_LED_INIT(11)
        STATUS_LED_INIT(12)
        STATUS_LED_INIT(13)
        STATUS_LED_INIT(14)
        STATUS_LED_INIT(15)
        STATUS_LED_INIT(16)
        STATUS_LED_INIT(17)
        STATUS_LED_INIT(18)
        STATUS_LED_INIT(19)
        STATUS_LED_INIT(20)
        STATUS_LED_INIT(21)
        default:
            LOG_ERROR("STATUS_LED", "Unsupported GPIO pin %d for status LED", BOARD_STATUS_LED_PIN);
            return;
    }

    #undef STATUS_LED_INIT

    // Set brightness to maximum (100%) for clear visibility
    FastLED.setBrightness(255);

    // Initialize to off
    led[0] = CRGB::Black;
    FastLED.show();

    LOG_VERBOSE("STATUS_LED", "Initialized WS2812 status LED on GPIO %d", BOARD_STATUS_LED_PIN);
}

// Helper to convert CRGB to human-readable color name for logging
static const char* colorToString(CRGB color)
{
    // Check common named colors
    if (color == CRGB::Black)       return "Black";
    if (color == CRGB::Red)         return "Red";
    if (color == CRGB::Green)       return "Green";
    if (color == CRGB::Blue)        return "Blue";
    if (color == CRGB::Yellow)      return "Yellow";
    if (color == CRGB::Orange)      return "Orange";
    if (color == CRGB::Purple)      return "Purple";
    if (color == CRGB::Cyan)        return "Cyan";
    if (color == CRGB::Magenta)     return "Magenta";
    if (color == CRGB::White)       return "White";

    // For unknown colors, return hex
    static char buffer[16];
    snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", color.r, color.g, color.b);
    return buffer;
}

void StatusLed::logColorChange(const char* mode, CRGB newColor, uint16_t intervalMs, const char* reason)
{
    // Build previous state description
    char prevState[64];
    if (previousColor == CRGB::Black && !previousIsBlinking && !previousIsHeartbeat)
    {
        snprintf(prevState, sizeof(prevState), "OFF");
    }
    else if (previousIsHeartbeat)
    {
        snprintf(prevState, sizeof(prevState), "HEARTBEAT %s (%dms/%dms)",
                 colorToString(previousColor), previousHeartbeatDuration, previousHeartbeatPeriod);
    }
    else if (previousIsBlinking)
    {
        snprintf(prevState, sizeof(prevState), "BLINK %s (%dms)",
                 colorToString(previousColor), previousBlinkInterval);
    }
    else
    {
        snprintf(prevState, sizeof(prevState), "SOLID %s", colorToString(previousColor));
    }

    // Build new state description
    char newState[64];
    if (newColor == CRGB::Black)
    {
        snprintf(newState, sizeof(newState), "OFF");
    }
    else if (intervalMs > 0)
    {
        snprintf(newState, sizeof(newState), "BLINK %s (%dms)",
                 colorToString(newColor), intervalMs);
    }
    else
    {
        snprintf(newState, sizeof(newState), "SOLID %s", colorToString(newColor));
    }

    // Log with or without reason
    if (reason && reason[0] != '\0')
    {
        LOG_VERBOSE("STATUS_LED", "%s -> %s (%s)", prevState, newState, reason);
    }
    else
    {
        LOG_VERBOSE("STATUS_LED", "%s -> %s", prevState, newState);
    }
}

void StatusLed::setSolid(CRGB color, const char* reason)
{
    // Thread-safe access (critical for dual-core ESP32-S3)
    if (mutex != nullptr && xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        // Skip if already in this exact state (prevents logging spam)
        if (!isBlinking && !isHeartbeat && currentColor == color)
        {
            xSemaphoreGive(mutex);
            return;
        }

        logColorChange("setSolid", color, 0, reason);

        // Update previous state
        previousColor = currentColor;
        previousIsBlinking = isBlinking;
        previousIsHeartbeat = isHeartbeat;
        previousBlinkInterval = blinkInterval;
        previousHeartbeatPeriod = heartbeatPeriod;
        previousHeartbeatDuration = heartbeatDuration;

        // Set new state
        isBlinking = false;
        isHeartbeat = false;
        currentColor = color;
        led[0] = color;
        FastLED.show();

        xSemaphoreGive(mutex);
    }
}

void StatusLed::setBlink(CRGB color, uint16_t intervalMs, const char* reason)
{
    // Thread-safe access (critical for dual-core ESP32-S3)
    if (mutex != nullptr && xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        // Skip if already in this exact state (prevents logging spam)
        if (isBlinking && !isHeartbeat && currentColor == color && blinkInterval == intervalMs)
        {
            xSemaphoreGive(mutex);
            return;
        }

        logColorChange("setBlink", color, intervalMs, reason);

        // Update previous state
        previousColor = currentColor;
        previousIsBlinking = isBlinking;
        previousIsHeartbeat = isHeartbeat;
        previousBlinkInterval = blinkInterval;
        previousHeartbeatPeriod = heartbeatPeriod;
        previousHeartbeatDuration = heartbeatDuration;

        // Set new state
        currentColor = color;
        blinkInterval = intervalMs;
        isBlinking = true;
        isHeartbeat = false;
        // Don't update immediately - let update() handle timing

        xSemaphoreGive(mutex);
    }
}

void StatusLed::off(const char* reason)
{
    // Thread-safe access (critical for dual-core ESP32-S3)
    if (mutex != nullptr && xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        // Skip if already off (prevents logging spam)
        if (!isBlinking && !isHeartbeat && currentColor == CRGB::Black)
        {
            xSemaphoreGive(mutex);
            return;
        }

        logColorChange("off", CRGB::Black, 0, reason);

        // Update previous state
        previousColor = currentColor;
        previousIsBlinking = isBlinking;
        previousIsHeartbeat = isHeartbeat;
        previousBlinkInterval = blinkInterval;
        previousHeartbeatPeriod = heartbeatPeriod;
        previousHeartbeatDuration = heartbeatDuration;

        // Set new state
        isBlinking = false;
        isHeartbeat = false;
        currentColor = CRGB::Black;
        led[0] = CRGB::Black;
        FastLED.show();

        xSemaphoreGive(mutex);
    }
}

void StatusLed::setHeartbeat(CRGB color, uint16_t flashDurationMs, uint16_t periodMs, const char* reason)
{
    // Thread-safe access (critical for dual-core ESP32-S3)
    if (mutex != nullptr && xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        // Skip if already in this exact heartbeat state (prevents logging spam)
        if (isHeartbeat && currentColor == color && heartbeatDuration == flashDurationMs && heartbeatPeriod == periodMs)
        {
            xSemaphoreGive(mutex);
            return;
        }

        // Log using custom heartbeat format (logColorChange doesn't handle heartbeat in "new state")
        char prevState[64];
        if (previousColor == CRGB::Black && !previousIsBlinking && !previousIsHeartbeat)
        {
            snprintf(prevState, sizeof(prevState), "OFF");
        }
        else if (previousIsHeartbeat)
        {
            snprintf(prevState, sizeof(prevState), "HEARTBEAT %s (%dms/%dms)",
                     colorToString(previousColor), previousHeartbeatDuration, previousHeartbeatPeriod);
        }
        else if (previousIsBlinking)
        {
            snprintf(prevState, sizeof(prevState), "BLINK %s (%dms)",
                     colorToString(previousColor), previousBlinkInterval);
        }
        else
        {
            snprintf(prevState, sizeof(prevState), "SOLID %s", colorToString(previousColor));
        }

        char newState[64];
        snprintf(newState, sizeof(newState), "HEARTBEAT %s (%dms/%dms)", colorToString(color), flashDurationMs, periodMs);

        if (reason && reason[0] != '\0')
        {
            LOG_VERBOSE("STATUS_LED", "%s -> %s (%s)", prevState, newState, reason);
        }
        else
        {
            LOG_VERBOSE("STATUS_LED", "%s -> %s", prevState, newState);
        }

        // Update previous state
        previousColor = currentColor;
        previousIsBlinking = isBlinking;
        previousIsHeartbeat = isHeartbeat;
        previousBlinkInterval = blinkInterval;
        previousHeartbeatPeriod = heartbeatPeriod;
        previousHeartbeatDuration = heartbeatDuration;

        // Set new heartbeat state
        isBlinking = false;
        isHeartbeat = true;
        currentColor = color;
        heartbeatDuration = flashDurationMs;
        heartbeatPeriod = periodMs;
        lastHeartbeat = millis();
        heartbeatState = false; // Start with LED off
        led[0] = CRGB::Black;
        FastLED.show();

        xSemaphoreGive(mutex);
    }
}

void StatusLed::update()
{
    // Thread-safe access (critical for dual-core ESP32-S3)
    if (mutex != nullptr && xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        unsigned long now = millis();

        // Handle blinking mode
        if (isBlinking)
        {
            if (now - lastBlink >= blinkInterval)
            {
                blinkState = !blinkState;
                led[0] = blinkState ? currentColor : CRGB::Black;
                FastLED.show();
                lastBlink = now;
            }
            xSemaphoreGive(mutex);
            return;
        }

        // Handle heartbeat mode
        if (isHeartbeat)
        {
            if (heartbeatState)
            {
                // LED is currently on - check if flash duration has elapsed
                if (now - lastHeartbeat >= heartbeatDuration)
                {
                    // Turn LED off
                    heartbeatState = false;
                    led[0] = CRGB::Black;
                    FastLED.show();
                }
            }
            else
            {
                // LED is currently off - check if period has elapsed
                if (now - lastHeartbeat >= heartbeatPeriod)
                {
                    // Start new flash
                    heartbeatState = true;
                    led[0] = currentColor;
                    FastLED.show();
                    lastHeartbeat = now;
                }
            }
        }

        xSemaphoreGive(mutex);
    }
}

#endif // BOARD_ESP32S3_CUSTOM_PCB && ENABLE_LEDS
