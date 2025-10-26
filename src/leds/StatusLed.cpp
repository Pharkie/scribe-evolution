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

void StatusLed::begin()
{
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

void StatusLed::setSolid(CRGB color)
{
    isBlinking = false;
    currentColor = color;
    led[0] = color;
    FastLED.show();
}

void StatusLed::setBlink(CRGB color, uint16_t intervalMs)
{
    currentColor = color;
    blinkInterval = intervalMs;
    isBlinking = true;
    // Don't update immediately - let update() handle timing
}

void StatusLed::off()
{
    isBlinking = false;
    currentColor = CRGB::Black;
    led[0] = CRGB::Black;
    FastLED.show();
}

void StatusLed::update()
{
    if (!isBlinking)
    {
        return;
    }

    unsigned long now = millis();
    if (now - lastBlink >= blinkInterval)
    {
        blinkState = !blinkState;
        led[0] = blinkState ? currentColor : CRGB::Black;
        FastLED.show();
        lastBlink = now;
    }
}

#endif // BOARD_ESP32S3_CUSTOM_PCB && ENABLE_LEDS
