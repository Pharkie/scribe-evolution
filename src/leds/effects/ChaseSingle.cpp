/**
 * @file ChaseSingleEffect.cpp
 * @brief Implementation of single color chase effect with trailing fade
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "ChaseSingle.h"

#if ENABLE_LEDS

#include "../../core/logging.h"
#include "../../core/config.h"
#include "../../core/led_config.h"

ChaseSingle::ChaseSingle(const ChaseSingleConfig &config)
    : config(config), targetCycles(1), frameCounter(0)
{
}

bool ChaseSingle::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                         float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                         int &completedCycles)
{
    clearAllLEDs(leds, ledCount);

    // Cycle-based: run from start to end, then wait for trail to completely exit
    int totalSteps = ledCount + config.trailLength; // Include trail length for complete exit
    int currentPosition = effectStep;

    if (currentPosition < ledCount)
    {
        // Main LED (only show if within strip bounds)
        leds[currentPosition] = color1;
    }

    // Add trailing dots with fading (only show trail positions within strip bounds)
    for (int i = 1; i <= config.trailLength; i++)
    {
        int trailPos = currentPosition - i;
        if (trailPos >= 0 && trailPos < ledCount) // Only show trail if within strip bounds (no wrapping)
        {
            CRGB trailColor = color1;
            trailColor.fadeToBlackBy(i * config.trailFade); // Fade each trailing dot
            leds[trailPos] = trailColor;
        }
    }

    // Use frame counter for speed control (higher speed = slower movement)
    frameCounter++;
    if (frameCounter >= config.speed)
    {
        frameCounter = 0;
        effectStep++; // Only advance position when frame counter reaches speed threshold
    }

    // Check if we've completed a cycle (head + entire trail has exited the strip)
    if (effectStep >= totalSteps)
    {
        completedCycles++;
        effectStep = 0;   // Reset for next cycle
        frameCounter = 0; // Reset frame counter
        LOG_VERBOSE("LEDS", "Chase single completed cycle %d/%d", completedCycles, targetCycles);

        // Return false if we've completed all requested cycles
        return completedCycles < targetCycles;
    }

    return true; // Continue running
}

void ChaseSingle::reset()
{
    // Reset state variables - will be set by the effect manager
    frameCounter = 0;
}

void ChaseSingle::clearAllLEDs(CRGB *leds, int ledCount)
{
    for (int i = 0; i < ledCount; i++)
    {
        leds[i] = CRGB::Black;
    }
}

#endif // ENABLE_LEDS
