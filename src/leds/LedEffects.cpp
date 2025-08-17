/**
 * @file LedEffectsNew.cpp
 * @brief Refactored LED effects manager using modular effect system
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "LedEffects.h"

#ifdef ENABLE_LEDS

#include "../core/logging.h"
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
                           effectDuration(0),
                           lastUpdate(0),
                           isCycleBased(false),
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
    switch (ledPin)
    {
    case 0:
        FastLED.addLeds<WS2812B, 0, GRB>(leds, ledCount);
        break;
    case 1:
        FastLED.addLeds<WS2812B, 1, GRB>(leds, ledCount);
        break;
    case 2:
        FastLED.addLeds<WS2812B, 2, GRB>(leds, ledCount);
        break;
    case 3:
        FastLED.addLeds<WS2812B, 3, GRB>(leds, ledCount);
        break;
    case 4:
        FastLED.addLeds<WS2812B, 4, GRB>(leds, ledCount);
        break;
    case 5:
        FastLED.addLeds<WS2812B, 5, GRB>(leds, ledCount);
        break;
    case 6:
        FastLED.addLeds<WS2812B, 6, GRB>(leds, ledCount);
        break;
    case 7:
        FastLED.addLeds<WS2812B, 7, GRB>(leds, ledCount);
        break;
    case 8:
        FastLED.addLeds<WS2812B, 8, GRB>(leds, ledCount);
        break;
    case 9:
        FastLED.addLeds<WS2812B, 9, GRB>(leds, ledCount);
        break;
    case 10:
        FastLED.addLeds<WS2812B, 10, GRB>(leds, ledCount);
        break;
    default:
        LOG_ERROR("LEDS", "Unsupported LED pin: %d", ledPin);
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

    // Check if effect should stop (duration-based)
    if (!isCycleBased && effectDuration > 0)
    {
        if (now - effectStartTime >= effectDuration)
        {
            stopEffect();
            return;
        }
    }

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

    // Check if cycle-based effect is complete
    if (isCycleBased && !shouldContinue)
    {
        stopEffect();
        return;
    }

    // Show the updated LEDs
    FastLED.show();
}

bool LedEffects::startEffectDuration(const String &effectName, unsigned long durationSeconds,
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
    effectDuration = durationSeconds * 1000; // Convert to milliseconds
    effectActive = true;
    isCycleBased = false;
    targetCycles = 0;
    completedCycles = 0;

    // Reset effect state
    effectStep = 0;
    effectDirection = 1;
    effectPhase = 0.0f;

    LOG_VERBOSE("LEDS", "Started duration-based effect: %s (duration: %lu seconds)",
                effectName.c_str(), durationSeconds);

    return true;
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

    // Check if effect supports cycle-based operation
    if (!currentEffect->isCycleBased())
    {
        LOG_WARNING("LEDS", "Effect %s does not support cycle-based operation", effectName.c_str());
        delete currentEffect;
        currentEffect = nullptr;
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
    effectDuration = 0; // Not used in cycle mode
    effectActive = true;
    isCycleBased = true;
    targetCycles = cycles;
    completedCycles = 0;

    // Reset effect state
    effectStep = 0;
    effectDirection = 1;
    effectPhase = 0.0f;

    LOG_VERBOSE("LEDS", "Started cycle-based effect: %s (cycles: %d)",
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

#endif // ENABLE_LEDS
