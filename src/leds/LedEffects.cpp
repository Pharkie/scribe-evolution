/**
 * @file LedEffectsNew.cpp
 * @brief Refactored LED effects manager using modular effect system
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "LedEffects.h"

#if ENABLE_LEDS

#include <core/logging.h>
#include <config/config.h>
#include <core/config_loader.h>
#include "effects/EffectRegistry.h"
#include <utils/color_utils.h>

// Global instance
LedEffects ledEffects;

LedEffects::LedEffects() : leds(nullptr),
                           ledCount(0),
                           ledPin(0),
                           ledBrightness(0),
                           ledRefreshRate(0),
                           ledUpdateInterval(0),
                           ledEffectFadeSpeed(0),
                           ledTwinkleDensity(0),
                           ledChaseSingleSpeed(0),
                           ledChaseMultiSpeed(0),
                           ledMatrixDrops(0),
                           effectActive(false),
                           currentEffectName(""),
                           effectStartTime(0),
                           lastUpdate(0),
                           targetCycles(1),
                           completedCycles(0),
                           effectColor1(CRGB::Blue),
                           effectColor2(CRGB::Black),
                           effectColor3(CRGB::Black),
                           effectStep(0),
                           effectDirection(1),
                           effectPhase(0.0f),
                           currentEffect(nullptr),
                           effectRegistry(nullptr),
                           finalFadeActive(false),
                           finalFadeStart(0),
                           finalFadeBase(nullptr)
{
}

LedEffects::~LedEffects()
{
    if (currentEffect)
    {
        delete currentEffect;
        currentEffect = nullptr;
    }

    if (effectRegistry)
    {
        delete effectRegistry;
        effectRegistry = nullptr;
    }

    if (leds)
    {
        delete[] leds;
        leds = nullptr;
    }
}

bool LedEffects::begin()
{
    // Load configuration
    const RuntimeConfig &config = getRuntimeConfig();

    return reinitialize(config.ledPin, config.ledCount, config.ledBrightness,
                        config.ledRefreshRate, config.ledEffects);
}

bool LedEffects::reinitialize(int pin, int count, int brightness, int refreshRate,
                              const LedEffectsConfig &effectsConfig)
{
    // Stop current effect
    stopEffect();

    // Store configuration
    ledPin = pin;
    ledCount = count;
    ledBrightness = brightness;
    ledRefreshRate = refreshRate;
    ledUpdateInterval = 1000 / refreshRate; // Convert Hz to milliseconds

    // Validate parameters
    if (ledCount <= 0 || ledPin < 0)
    {
        LOG_ERROR("LEDS", "Invalid LED configuration: count=%d, pin=%d", ledCount, ledPin);
        return false;
    }

    // Clean up existing LED array
    if (leds)
    {
        delete[] leds;
        leds = nullptr;
    }

    // Allocate LED array
    leds = new CRGB[ledCount];
    if (!leds)
    {
        LOG_ERROR("LEDS", "Failed to allocate memory for %d LEDs", ledCount);
        return false;
    }

    // Enable LED eFuse if present (custom PCB only)
    #if BOARD_HAS_LED_EFUSE
    const BoardPinDefaults &boardDefaults = getBoardDefaults();
    if (boardDefaults.efuse.ledStrip != -1)
    {
        LOG_VERBOSE("LEDS", "Enabling LED strip eFuse on GPIO %d", boardDefaults.efuse.ledStrip);
        pinMode(boardDefaults.efuse.ledStrip, OUTPUT);
        digitalWrite(boardDefaults.efuse.ledStrip, HIGH); // Enable LED power
        delay(10); // Give eFuse time to stabilize
    }
    #endif

    // Initialize FastLED
    // Validate GPIO pin using configuration system
    if (!isSafeGPIO(ledPin))
    {
        LOG_ERROR("LEDS", "GPIO %d cannot be used for LEDs: %s", ledPin, getGPIODescription(ledPin));
        return false;
    }

    LOG_VERBOSE("LEDS", "Initializing FastLED on GPIO %d (Board: %s)", ledPin, BOARD_NAME);

    // FastLED requires GPIO pin as compile-time template parameter
    // Use macro to reduce repetition
    #define FASTLED_INIT_CASE(pin) \
        case pin: FastLED.addLeds<WS2812B, pin, GRB>(leds, ledCount); break;

    bool initSuccess = false;
    switch (ledPin)
    {
    // Common GPIO pins (0-10, 20-21) - available on all ESP32 boards
    FASTLED_INIT_CASE(0)
    FASTLED_INIT_CASE(1)
    FASTLED_INIT_CASE(2)
    FASTLED_INIT_CASE(3)
    FASTLED_INIT_CASE(4)
    FASTLED_INIT_CASE(5)
    FASTLED_INIT_CASE(6)
    FASTLED_INIT_CASE(7)
    FASTLED_INIT_CASE(8)
    FASTLED_INIT_CASE(9)
    FASTLED_INIT_CASE(10)
    FASTLED_INIT_CASE(20)
    FASTLED_INIT_CASE(21)

    // ESP32-S3 additional GPIO pins - only compile for boards with GPIO > 21
    #if BOARD_MAX_GPIO > 21
    FASTLED_INIT_CASE(11)
    FASTLED_INIT_CASE(12)
    FASTLED_INIT_CASE(13)
    FASTLED_INIT_CASE(14)
    FASTLED_INIT_CASE(15)
    FASTLED_INIT_CASE(16)
    FASTLED_INIT_CASE(17)
    FASTLED_INIT_CASE(18)
    FASTLED_INIT_CASE(33)
    FASTLED_INIT_CASE(34)
    FASTLED_INIT_CASE(35)
    FASTLED_INIT_CASE(36)
    FASTLED_INIT_CASE(37)
    FASTLED_INIT_CASE(38)
    FASTLED_INIT_CASE(39)
    FASTLED_INIT_CASE(40)
    FASTLED_INIT_CASE(41)
    FASTLED_INIT_CASE(42)
    FASTLED_INIT_CASE(43)
    FASTLED_INIT_CASE(44)
    FASTLED_INIT_CASE(47)
    FASTLED_INIT_CASE(48)
    #endif

    default:
        LOG_ERROR("LEDS", "GPIO %d not implemented in FastLED switch (this is a code bug)", ledPin);
        return false;
    }

    initSuccess = true;
    #undef FASTLED_INIT_CASE

    if (!initSuccess)
    {
        LOG_ERROR("LEDS", "FastLED initialization failed for GPIO %d", ledPin);
        return false;
    }

    FastLED.setBrightness(ledBrightness);
    FastLED.clear();
    FastLED.show();

    // Create or update effect registry
    if (effectRegistry)
    {
        effectRegistry->updateConfig(effectsConfig);
    }
    else
    {
        effectRegistry = new EffectRegistry(effectsConfig);
    }

    LOG_VERBOSE("LEDS", "LED system initialized: pin=%d, count=%d, brightness=%d, refresh=%dHz",
                ledPin, ledCount, ledBrightness, ledRefreshRate);

    return true;
}

void LedEffects::update()
{
    if (!effectActive || !currentEffect || !leds)
    {
        return;
    }

    unsigned long now = millis();

    // Check if it's time to update
    if (now - lastUpdate < ledUpdateInterval)
    {
        return;
    }

    lastUpdate = now;

    // Handle manager-driven final fade-out (e.g., rainbow end-of-all-cycles)
    if (finalFadeActive)
    {
        unsigned long elapsed = now - finalFadeStart;
        if (elapsed >= finalFadeDurationMs)
        {
            // Cleanup fade resources then stop
            if (finalFadeBase)
            {
                delete[] finalFadeBase;
                finalFadeBase = nullptr;
            }
            stopEffect();
            return;
        }

        // Time-based linear fade for smoother perception over the duration
        float t = (float)elapsed / (float)finalFadeDurationMs; // 0..1
        t = constrain(t, 0.0f, 1.0f);
        // Perceptual (gamma) fade: closer to human brightness perception
        float gamma = 2.2f;
        uint8_t scale = (uint8_t)(powf(1.0f - t, gamma) * 255.0f);
        if (finalFadeBase)
        {
            for (int i = 0; i < ledCount; i++)
            {
                CRGB c = finalFadeBase[i];
                nscale8x3_video(c.r, c.g, c.b, scale);
                leds[i] = c;
            }
        }
        FastLED.show();
        return;
    }

    // Update the current effect
    bool shouldContinue = currentEffect->update(leds, ledCount, effectStep, effectDirection,
                                                effectPhase, effectColor1, effectColor2, effectColor3,
                                                completedCycles);

    // Check if cycle-based effect is complete (when target cycles > 0)
    if (targetCycles > 0 && completedCycles >= targetCycles)
    {
        // For rainbow: initiate a manager-driven final fade-out instead of abrupt stop
        if (currentEffectName.equalsIgnoreCase("rainbow"))
        {
            finalFadeActive = true;
            finalFadeStart = now;
            // Snapshot current LED state as base for linear fade
            if (finalFadeBase)
            {
                delete[] finalFadeBase;
                finalFadeBase = nullptr;
            }
            finalFadeBase = new CRGB[ledCount];
            if (finalFadeBase)
            {
                for (int i = 0; i < ledCount; i++)
                {
                    finalFadeBase[i] = leds[i];
                }
            }
            FastLED.show();
            return;
        }
        else
        {
            stopEffect();
            return;
        }
    }

    // Show the updated LEDs
    FastLED.show();
}

bool LedEffects::startEffectCycles(const String &effectName, int cycles,
                                   CRGB color1, CRGB color2, CRGB color3)
{
    if (!effectRegistry || !effectRegistry->isValidEffect(effectName))
    {
        LOG_WARNING("LEDS", "Unknown effect name: %s", effectName.c_str());
        return false;
    }

    // Stop current effect
    stopEffect();

    // Create new effect
    currentEffect = effectRegistry->createEffect(effectName);
    if (!currentEffect)
    {
        return false;
    }

    // Initialize effect if needed
    currentEffect->initialize(ledCount);

    // Set effect parameters
    currentEffectName = effectName;
    effectColor1 = color1;
    effectColor2 = color2;
    effectColor3 = color3;
    effectStartTime = millis();
    effectActive = true;
    targetCycles = cycles;
    completedCycles = 0;
    finalFadeActive = false;
    finalFadeStart = 0;
    if (finalFadeBase)
    {
        delete[] finalFadeBase;
        finalFadeBase = nullptr;
    }

    // Reset effect state
    effectStep = 0;
    effectDirection = 1;
    effectPhase = 0.0f;

    LOG_VERBOSE("LEDS", "Started LED effect: %s (cycles: %d)",
                effectName.c_str(), cycles);

    return true;
}

bool LedEffects::startEffectCyclesAuto(const String &effectName, int cycles)
{
    // Use autonomous per-effect default colors from registry
    if (!effectRegistry)
    {
        LOG_WARNING("LEDS", "Effect registry not initialized");
        return false;
    }

    String h1, h2, h3;
    effectRegistry->getDefaultColorsHex(effectName, h1, h2, h3);

    // Convert to CRGB (fallbacks if strings are empty)
    CRGB c1 = CRGB::Blue;
    CRGB c2 = CRGB::Black;
    CRGB c3 = CRGB::Black;

    // Convert only when hex provided; otherwise keep sensible defaults
    if (h1.length() > 0)
    {
        c1 = hexToRgb(h1);
    }
    if (h2.length() > 0)
    {
        c2 = hexToRgb(h2);
    }
    if (h3.length() > 0)
    {
        c3 = hexToRgb(h3);
    }

    return startEffectCycles(effectName, cycles, c1, c2, c3);
}

void LedEffects::stopEffect()
{
    if (currentEffect)
    {
        delete currentEffect;
        currentEffect = nullptr;
    }

    effectActive = false;
    currentEffectName = "";

    // Clear all LEDs - be extra thorough
    if (leds && ledCount > 0)
    {
        // First clear our LED array
        for (int i = 0; i < ledCount; i++)
        {
            leds[i] = CRGB::Black;
        }

        // Show the cleared state
        FastLED.show();

        // Additional safety: clear again and show again to ensure it takes effect
        fill_solid(leds, ledCount, CRGB::Black);
        FastLED.show();
    }
    finalFadeActive = false;
    finalFadeStart = 0;
    if (finalFadeBase)
    {
        delete[] finalFadeBase;
        finalFadeBase = nullptr;
    }
}

bool LedEffects::isEffectRunning() const
{
    return effectActive;
}

String LedEffects::getCurrentEffectName() const
{
    return currentEffectName;
}

unsigned long LedEffects::getRemainingTime() const
{
    if (!effectActive || effectDuration == 0)
    {
        return 0;
    }

    unsigned long elapsed = millis() - effectStartTime;
    if (elapsed >= effectDuration)
    {
        return 0;
    }

    return effectDuration - elapsed;
}

void LedEffects::updateEffectConfig(const LedEffectsConfig &newConfig)
{
    if (effectRegistry)
    {
        effectRegistry->updateConfig(newConfig);
        LOG_VERBOSE("LEDS", "Updated effect configuration for playground use");
    }
}

#endif // ENABLE_LEDS
