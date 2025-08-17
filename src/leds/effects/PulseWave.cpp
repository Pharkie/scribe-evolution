/**
 * @file PulseWave.cpp
 * @brief Implementation of sine wave pulse effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "PulseWave.h"

#ifdef ENABLE_LEDS

#include <math.h>

PulseWave::PulseWave()
    : frameCounter(0)
{
}

bool PulseWave::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                       float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                       int &completedCycles)
{
    // Create a sine wave pulse across the strip
    for (int i = 0; i < ledCount; i++)
    {
        float brightness = sin((effectPhase + i * 0.3f) * 3.14159f / 180.0f);
        brightness = (brightness + 1.0f) / 2.0f; // Normalize to 0-1

        // Use color1 parameter (should be hot pink from web interface)
        CRGB color = color1;
        color.fadeToBlackBy(255 - (int)(brightness * 255));
        leds[i] = color;
    }

    // Use frame counter for speed control - update phase only every few frames
    frameCounter++;
    if (frameCounter >= 2) // Update every 2 frames for smoother but controlled speed
    {
        frameCounter = 0;
        effectPhase += 8.0f; // Speed of pulse wave
        if (effectPhase >= 360.0f)
        {
            effectPhase = 0.0f;
        }
    }

    return true; // Continue running (duration-based effect)
}

void PulseWave::reset()
{
    frameCounter = 0;
    // Reset phase will be handled by the effect manager
}

#endif // ENABLE_LEDS
