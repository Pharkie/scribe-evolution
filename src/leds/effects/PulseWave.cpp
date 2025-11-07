/**
 * @file PulseWave.cpp
 * @brief Implementation of cosine wave pulse effect (off→on→off)
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "PulseWave.h"

#if ENABLE_LEDS

#include <math.h>
#include <core/led_config.h>

PulseWave::PulseWave(const PulseConfig &config)
    : config(config)
{
    initTiming();
}

bool PulseWave::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                       float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                       int &completedCycles)
{
    float deltaTime = calculateDeltaTime();

    // Smooth pulse timing: speed=100→1sec, speed=1→5sec per cycle (frame-rate independent!)
    float cycleSeconds = speedToCycleSeconds(config.speed);
    float degreesPerSecond = 360.0f / cycleSeconds;
    float phaseIncrementThisFrame = degreesPerSecond * deltaTime;

    effectPhase += phaseIncrementThisFrame;

    if (effectPhase >= 360.0f)
    {
        effectPhase = fmodf(effectPhase, 360.0f); // Wrap to 0..360 range
        completedCycles++;
    }

    // Cosine-based pulse for OFF → ON → OFF over 0..360 degrees
    // cos(0)=1 -> 0 brightness; cos(180)=-1 -> 1 brightness; cos(360)=1 -> 0 brightness
    float phaseRadians = effectPhase * 3.14159f / 180.0f;
    float brightness01 = 0.5f * (1.0f - cosf(phaseRadians)); // 0..1

    // Map to configured max brightness range (always starts from 0)
    int minB = 0;  // Pulse always starts from completely OFF
    int maxB = max(0, min(255, config.maxBrightness));
    int targetBrightness = minB + (int)((maxB - minB) * brightness01 + 0.5f); // 0..255
    uint8_t fadeAmount = (uint8_t)(255 - targetBrightness);

    // Apply brightness to all LEDs with the same color
    for (int i = 0; i < ledCount; i++)
    {
        CRGB color = color1;
        color.fadeToBlackBy(fadeAmount);
        leds[i] = color;
    }

    return true; // Continue running (cycle tracking handled by LedEffects manager)
}

void PulseWave::reset()
{
    EffectBase::reset();
    // Reset phase will be handled by the effect manager
}

#endif // ENABLE_LEDS
