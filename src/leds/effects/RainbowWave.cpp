/**
 * @file RainbowWave.cpp
 * @brief Implementation of rainbow wave effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "RainbowWave.h"

#ifdef ENABLE_LEDS

RainbowWave::RainbowWave()
{
}

bool RainbowWave::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                               float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                               int &completedCycles)
{
    for (int i = 0; i < ledCount; i++)
    {
        // Create rainbow wave with moving phase
        int hue = (i * 255 / ledCount + (int)effectPhase) % 256;
        leds[i] = wheel(hue);
    }

    effectPhase += RAINBOW_WAVE_SPEED; // Speed of wave movement
    if (effectPhase >= 256.0f)
    {
        effectPhase = 0.0f;
    }

    return true; // Continue running (duration-based effect)
}

void RainbowWave::reset()
{
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
