/**
 * @file RainbowWave.cpp
 * @brief Implementation of rainbow wave effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "RainbowWave.h"

#if ENABLE_LEDS

#include <core/led_config.h>
#include <core/config_loader.h>

RainbowWave::RainbowWave(const RainbowConfig &config)
    : config(config), frameCounter(0)
{
}

bool RainbowWave::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                         float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                         int &completedCycles)
{
    if (startMillis == 0)
    {
        startMillis = millis();
    }

    // Compute fade-in (first 1s). Final fade-out is handled by the manager.
    float brightnessScale = 1.0f;
    unsigned long now = millis();
    unsigned long elapsed = now - startMillis;
    if (elapsed < 1000UL)
    {
        brightnessScale = (float)elapsed / 1000.0f; // 0..1 over first second
    }

    for (int i = 0; i < ledCount; i++)
    {
        // Create rainbow wave with moving phase
        int hue = (i * 255 / ledCount + (int)effectPhase) % 256;
        CRGB c = wheel(hue);
        // Apply brightness scaling
        nscale8x3_video(c.r, c.g, c.b, (uint8_t)(brightnessScale * 255.0f));
        leds[i] = c;
    }

    // Advance the phase every update. config.speed is the phase increment per frame.
    // Higher slider -> larger increment -> faster movement
    effectPhase += max(0.1f, config.speed);

    // Check if we completed a full rainbow cycle (0 -> 256 hue values)
    if (effectPhase >= 256.0f)
    {
        effectPhase = 0.0f;
        completedCycles++;
        // Reset fade-in for consecutive effects only once at start; keep startMillis
    }

    return true; // Continue running (cycle tracking handled by LedEffects manager)
}

void RainbowWave::reset()
{
    frameCounter = 0;
    startMillis = millis();
    // Reset phase will be handled by the effect manager
}

CRGB RainbowWave::wheel(byte wheelPos)
{
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85)
    {
        return CRGB(255 - wheelPos * 3, 0, wheelPos * 3);
    }
    else if (wheelPos < 170)
    {
        wheelPos -= 85;
        return CRGB(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    else
    {
        wheelPos -= 170;
        return CRGB(wheelPos * 3, 255 - wheelPos * 3, 0);
    }
}

#endif // ENABLE_LEDS
