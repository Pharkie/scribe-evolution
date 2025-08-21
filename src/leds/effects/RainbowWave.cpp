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

#include "../../core/led_config.h"

RainbowWave::RainbowWave(const RainbowConfig &config)
    : config(config), frameCounter(0)
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

    // Use frame counter for speed control - update phase only every few frames
    frameCounter++;
    if (frameCounter >= (8 - (int)config.speed)) // Faster speed = fewer frames to wait
    {
        frameCounter = 0;
        effectPhase += 4.0f; // Speed of wave movement

        // Check if we completed a full rainbow cycle (0 -> 256 hue values)
        if (effectPhase >= 256.0f)
        {
            effectPhase = 0.0f;
            completedCycles++;
        }
    }

    return true; // Continue running (cycle tracking handled by LedEffects manager)
}

void RainbowWave::reset()
{
    frameCounter = 0;
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
