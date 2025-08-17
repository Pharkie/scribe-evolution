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
    : chaseSpeed(chaseSpeed), isCycleBasedMode(true), targetCycles(1), frameCounter(0)
{
}

bool ChaseMulti::update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                        float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                        int &completedCycles)
{
    clearAllLEDs(leds, ledCount);

    if (isCycleBasedMode)
    {
        // Cycle-based: run from start to end, then wait for trail to completely exit
        // Add spacing between colors so they follow each other
        int colorSpacing = CHASE_TRAIL_LENGTH + 2;                         // Space colors apart by trail length + gap
        int totalSteps = ledCount + colorSpacing * 2 + CHASE_TRAIL_LENGTH; // Include space for all colors and trails
        int currentPosition = effectStep;

        // Draw color1 (main/first color)
        if (currentPosition >= 0 && currentPosition < ledCount)
        {
            leds[currentPosition] = color1;
        }

        // Add color1 trail
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            int trailPos = currentPosition - i;
            if (trailPos >= 0 && trailPos < ledCount)
            {
                CRGB trailColor = color1;
                trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP);
                leds[trailPos] = trailColor;
            }
        }

        // Draw color2 (second color, following behind)
        int color2Position = currentPosition - colorSpacing;
        if (color2Position >= 0 && color2Position < ledCount)
        {
            leds[color2Position] = color2;
        }

        // Add color2 trail
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            int trailPos = color2Position - i;
            if (trailPos >= 0 && trailPos < ledCount)
            {
                CRGB trailColor = color2;
                trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP);
                leds[trailPos] = trailColor;
            }
        }

        // Draw color3 (third color, following behind color2)
        int color3Position = currentPosition - (colorSpacing * 2);
        if (color3Position >= 0 && color3Position < ledCount)
        {
            leds[color3Position] = color3;
        }

        // Add color3 trail
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            int trailPos = color3Position - i;
            if (trailPos >= 0 && trailPos < ledCount)
            {
                CRGB trailColor = color3;
                trailColor.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP);
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

        // Check if we've completed a cycle (all colors + trails have exited the strip)
        if (effectStep >= totalSteps)
        {
            completedCycles++;
            effectStep = 0;   // Reset for next cycle
            frameCounter = 0; // Reset frame counter
            LOG_VERBOSE("LEDS", "Chase multi completed cycle %d/%d", completedCycles, targetCycles);

            // Return false if we've completed all requested cycles
            return completedCycles < targetCycles;
        }
    }
    else
    {
        // Duration-based: continuous multi-color chase with spacing
        int colorSpacing = CHASE_TRAIL_LENGTH + 2; // Space colors apart
        int position1 = effectStep % ledCount;
        int position2 = (effectStep - colorSpacing + ledCount) % ledCount;
        int position3 = (effectStep - (colorSpacing * 2) + ledCount) % ledCount;

        // Draw all three colors with their trails
        leds[position1] = color1;
        leds[position2] = color2;
        leds[position3] = color3;

        // Add trails for each color
        for (int i = 1; i <= CHASE_TRAIL_LENGTH; i++)
        {
            // Color1 trail
            int trail1Pos = (position1 - i + ledCount) % ledCount;
            CRGB trail1Color = color1;
            trail1Color.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP);
            leds[trail1Pos] = trail1Color;

            // Color2 trail
            int trail2Pos = (position2 - i + ledCount) % ledCount;
            CRGB trail2Color = color2;
            trail2Color.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP);
            leds[trail2Pos] = trail2Color;

            // Color3 trail
            int trail3Pos = (position3 - i + ledCount) % ledCount;
            CRGB trail3Color = color3;
            trail3Color.fadeToBlackBy(i * CHASE_TRAIL_FADE_STEP);
            leds[trail3Pos] = trail3Color;
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

void ChaseMulti::reset()
{
    // Reset state variables - will be set by the effect manager
    frameCounter = 0;
}

void ChaseMulti::clearAllLEDs(CRGB *leds, int ledCount)
{
    for (int i = 0; i < ledCount; i++)
    {
        leds[i] = CRGB::Black;
    }
}

#endif // ENABLE_LEDS
