/**
 * @file ChaseMultiEffect.cpp
 * @brief Implementation of multi-color chase effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "ChaseMulti.h"

#ifdef ENABLE_LEDS

#include "../../core/logging.h"
#include "../../core/config.h"

ChaseMulti::ChaseMulti(int chaseSpeed)
    : chaseSpeed(chaseSpeed), isCycleBasedMode(true), targetCycles(3)
{
}

bool ChaseMulti::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                        float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                        int &completedCycles)
{
    clearAllLEDs(leds, ledCount);

    if (isCycleBasedMode)
    {
        // Cycle-based: cycle through colors (color1, color2, color3, repeat)
        int totalSteps = ledCount;
        int currentPosition = effectStep % totalSteps;

        // Determine which color to use based on completed cycles
        CRGB currentColor;
        int colorIndex = completedCycles % 3;
        switch (colorIndex)
        {
        case 0:
            currentColor = color1;
            break;
        case 1:
            currentColor = color2;
            break;
        case 2:
            currentColor = color3;
            break;
        default:
            currentColor = color1;
            break;
        }

        if (currentPosition < ledCount)
        {
            // Main LED with current color
            leds[currentPosition] = currentColor;

            // Add trailing dots with fading
            for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
            {
                int trailPos = (currentPosition - i + ledCount) % ledCount;
                if (currentPosition >= i)
                { // Only show trail if we're far enough along
                    CRGB trailColor = currentColor;
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
            LOG_VERBOSE("LEDS", "Chase multi completed cycle %d/%d (color %d)", completedCycles, targetCycles, colorIndex);

            // Return false if we've completed all requested cycles
            return completedCycles < targetCycles;
        }
    }
    else
    {
        // Duration-based: continuous multi-color chase
        int position = effectStep % ledCount;

        // Cycle through colors based on position
        CRGB currentColor;
        int colorIndex = (effectStep / ledCount) % 3;
        switch (colorIndex)
        {
        case 0:
            currentColor = color1;
            break;
        case 1:
            currentColor = color2;
            break;
        case 2:
            currentColor = color3;
            break;
        default:
            currentColor = color1;
            break;
        }

        leds[position] = currentColor;

        // Add trailing dots with fading
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            int trailPos = (position - i + ledCount) % ledCount;
            CRGB trailColor = currentColor;
            trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP); // Fade each trailing dot
            leds[trailPos] = trailColor;
        }

        effectStep += chaseSpeed;
    }

    return true; // Continue running
}

void ChaseMulti::reset()
{
    // Reset state variables - will be set by the effect manager
}

void ChaseMulti::clearAllLEDs(CRGB *leds, int ledCount)
{
    for (int i = 0; i < ledCount; i++)
    {
        leds[i] = CRGB::Black;
    }
}

#endif // ENABLE_LEDS
