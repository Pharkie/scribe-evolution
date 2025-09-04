/**
 * @file PulseWave.cpp
 * @brief Implementation of sine wave pulse effect
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
    // Create a sine wave pulse across the entire strip
    float phaseRadians = effectPhase * 3.14159f / 180.0f;
    float sineValue = sin(phaseRadians);

    // Convert sine wave (-1 to 1) to brightness (0 to 1)
    float brightness = (sineValue + 1.0f) / 2.0f;

    // Apply brightness to all LEDs with the same color
    for (int i = 0; i < ledCount; i++)
    {
        CRGB color = color1;
        color.fadeToBlackBy(255 - (int)(brightness * 255));
        leds[i] = color;
    }

    // Use frame counter for speed control.
    // config.speed represents a frame delay (smaller = faster)
    frameCounter++;
    if (frameCounter >= config.speed)
    {
        frameCounter = 0;
        effectPhase += 8.0f; // Increment phase per update

        // Check if we completed a full cycle (0 -> 360 degrees)
        if (effectPhase >= 360.0f)
        {
            effectPhase = 0.0f;
            completedCycles++;
        }
    }

    return true; // Continue running (cycle tracking handled by LedEffects manager)
}

void PulseWave::reset()
{
    frameCounter = 0;
    // Reset phase will be handled by the effect manager
}

#endif // ENABLE_LEDS
