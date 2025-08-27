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

#include "../core/logging.h"
#include "../core/config.h"
#include "../core/config_loader.h"
#include "effects/EffectRegistry.h"

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
                           effectRegistry(nullptr)
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

    // Initialize FastLED
    // Validate GPIO pin using configuration system
    if (!isSafeGPIO(ledPin))
    {
        LOG_ERROR("LEDS", "GPIO %d cannot be used for LEDs: %s", ledPin, getGPIODescription(ledPin));
        return false;
    }

    // Dynamic FastLED initialization using template magic
    // This avoids the massive switch statement and supports all valid GPIO pins
    bool initSuccess = false;
    switch (ledPin)
    {
    case 0:
        initSuccess = (FastLED.addLeds<WS2812B, 0, GRB>(leds, ledCount), true);
        break;
    case 1:
        initSuccess = (FastLED.addLeds<WS2812B, 1, GRB>(leds, ledCount), true);
        break;
    case 2:
        initSuccess = (FastLED.addLeds<WS2812B, 2, GRB>(leds, ledCount), true);
        break;
    case 3:
        initSuccess = (FastLED.addLeds<WS2812B, 3, GRB>(leds, ledCount), true);
        break;
    case 4:
        initSuccess = (FastLED.addLeds<WS2812B, 4, GRB>(leds, ledCount), true);
        break;
    case 5:
        initSuccess = (FastLED.addLeds<WS2812B, 5, GRB>(leds, ledCount), true);
        break;
    case 6:
        initSuccess = (FastLED.addLeds<WS2812B, 6, GRB>(leds, ledCount), true);
        break;
    case 7:
        initSuccess = (FastLED.addLeds<WS2812B, 7, GRB>(leds, ledCount), true);
        break;
    case 8:
        initSuccess = (FastLED.addLeds<WS2812B, 8, GRB>(leds, ledCount), true);
        break;
    case 9:
        initSuccess = (FastLED.addLeds<WS2812B, 9, GRB>(leds, ledCount), true);
        break;
    case 10:
        initSuccess = (FastLED.addLeds<WS2812B, 10, GRB>(leds, ledCount), true);
        break;
    case 20:
        initSuccess = (FastLED.addLeds<WS2812B, 20, GRB>(leds, ledCount), true);
        break;
    case 21:
        initSuccess = (FastLED.addLeds<WS2812B, 21, GRB>(leds, ledCount), true);
        break;
    default:
        LOG_ERROR("LEDS", "GPIO %d not implemented in FastLED switch (this is a code bug)", ledPin);
        return false;
    }

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

    // Update the current effect
    bool shouldContinue = currentEffect->update(leds, ledCount, effectStep, effectDirection,
                                                effectPhase, effectColor1, effectColor2, effectColor3,
                                                completedCycles);

    // Check if cycle-based effect is complete (when target cycles > 0)
    if (targetCycles > 0 && completedCycles >= targetCycles)
    {
        stopEffect();
        return;
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

    // Reset effect state
    effectStep = 0;
    effectDirection = 1;
    effectPhase = 0.0f;

    LOG_VERBOSE("LEDS", "Started LED effect: %s (cycles: %d)",
                effectName.c_str(), cycles);

    return true;
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
