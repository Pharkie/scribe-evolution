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
    : chaseSpeed(chaseSpeed), isCycleBasedMode(true), targetCycles(1), frameCounter(0)
{
}

bool ChaseSingle::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                         float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                         int &completedCycles)
{
    clearAllLEDs(leds, ledCount);

    if (isCycleBasedMode)
    {
        // Cycle-based: run from start to end, then wait for trail to completely exit
        int totalSteps = ledCount + CHASE_TRAIL_LENGTH; // Include trail length for complete exit
        int currentPosition = effectStep;

        if (currentPosition < ledCount)
        {
            // Main LED (only show if within strip bounds)
            leds[currentPosition] = color1;
        }

        // Add trailing dots with fading (only show trail positions within strip bounds)
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            int trailPos = currentPosition - i;
            if (trailPos >= 0 && trailPos < ledCount) // Only show trail if within strip bounds (no wrapping)
            {
                CRGB trailColor = color1;
                trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP); // Fade each trailing dot
                leds[trailPos] = trailColor;
            }
        }

        // Use frame counter for speed control (higher chaseSpeed = slower movement)
        frameCounter++;
        if (frameCounter >= chaseSpeed)
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
    }
    else
    {
        // Duration-based: continuous chase with trail
        int position = effectStep % ledCount;

        leds[position] = color1;

        // Add trailing dots with fading - always show full trail
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            int trailPos = (position - i + ledCount) % ledCount;
            CRGB trailColor = color1;
            trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP); // Fade each trailing dot
            leds[trailPos] = trailColor;
        }

        // Use frame counter for speed control (higher chaseSpeed = slower movement)
        frameCounter++;
        if (frameCounter >= chaseSpeed)
        {
            frameCounter = 0;
            effectStep++; // Only advance position when frame counter reaches speed threshold
        }
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
