/**
 * @file ChaseSingleEffect.cpp
 * @brief Implementation of single color chase effect with trailing fade
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "ChaseSingle.h"

#ifdef ENABLE_LEDS

#include "../../core/logging.h"
#include "../../core/config.h"

ChaseSingle::ChaseSingle(int chaseSpeed)
    : chaseSpeed(chaseSpeed), isCycleBasedMode(true), targetCycles(1)
{
}

bool ChaseSingle::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                         float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                         int &completedCycles)
{
    clearAllLEDs(leds, ledCount);

    if (isCycleBasedMode)
    {
        // Cycle-based: run from start to end once per cycle with trail
        int totalSteps = ledCount;
        int currentPosition = effectStep % totalSteps;

        if (currentPosition < ledCount)
        {
            // Main LED
            leds[currentPosition] = color1;

            // Add trailing dots with fading
            for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
            {
                int trailPos = (currentPosition - i + ledCount) % ledCount;
                if (currentPosition >= i)
                { // Only show trail if we're far enough along
                    CRGB trailColor = color1;
                    trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP); // Fade each trailing dot
                    leds[trailPos] = trailColor;
                }
            }
        }

        effectStep += chaseSpeed;

        // Check if we've completed a cycle (reached the end)
        if (effectStep >= totalSteps)
        {
            completedCycles++;
            effectStep = 0; // Reset for next cycle
            LOG_VERBOSE("LEDS", "Chase single completed cycle %d/%d", completedCycles, targetCycles);

            // Return false if we've completed all requested cycles
            return completedCycles < targetCycles;
        }
    }
    else
    {
        // Duration-based: continuous chase with trail
        int position = effectStep % ledCount;
        leds[position] = color1;

        // Add trailing dots with fading
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            int trailPos = (position - i + ledCount) % ledCount;
            CRGB trailColor = color1;
            trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP); // Fade each trailing dot
            leds[trailPos] = trailColor;
        }

        effectStep += chaseSpeed;
    }

    return true; // Continue running
}

void ChaseSingle::reset()
{
    // Reset state variables - will be set by the effect manager
}

void ChaseSingle::clearAllLEDs(CRGB *leds, int ledCount)
{
    for (int i = 0; i < ledCount; i++)
    {
        leds[i] = CRGB::Black;
    }
}

#endif // ENABLE_LEDS
