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
#include "../../core/led_config.h"

PulseWave::PulseWave(const PulseConfig &config)
    : config(config), frameCounter(0)
{
}

bool PulseWave::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                       float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                       int &completedCycles)
{
    // Update phase first so last frame of a cycle renders at phase 0 (off)
    frameCounter++;
    if (frameCounter >= config.speed)
    {
        frameCounter = 0;
        effectPhase += 8.0f;
        if (effectPhase >= 360.0f)
        {
            effectPhase = 0.0f; // Ensure OFF at cycle boundary
            completedCycles++;
        }
    }

    // Cosine-based pulse for OFF → ON → OFF over 0..360 degrees
    // cos(0)=1 -> 0 brightness; cos(180)=-1 -> 1 brightness; cos(360)=1 -> 0 brightness
    float phaseRadians = effectPhase * 3.14159f / 180.0f;
    float brightness01 = 0.5f * (1.0f - cosf(phaseRadians)); // 0..1

    // Map to configured min/max brightness range (0..255)
    int minB = max(0, min(255, config.minBrightness));
    int maxB = max(0, min(255, config.maxBrightness));
    if (maxB < minB)
        std::swap(maxB, minB);
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
    frameCounter = 0;
    // Reset phase will be handled by the effect manager
}

#endif // ENABLE_LEDS
