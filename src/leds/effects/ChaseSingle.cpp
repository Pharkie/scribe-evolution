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
    : config(config), targetCycles(1), frameCounter(0), stepAccumulator(0.0f)
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

    // Add trailing dots with linearly fading brightness (tip -> tail)
    int N = max(1, config.trailLength);
    for (int i = 1; i <= config.trailLength; i++)
    {
        int trailPos = currentPosition - i;
        if (trailPos >= 0 && trailPos < ledCount)
        {
            // Linear fade: near the tip is brightest, tail fades to 0
            uint8_t scale = (uint8_t)(((long)(N - i + 1) * 255) / (N + 1));
            CRGB trailColor = color1;
            nscale8x3_video(trailColor.r, trailColor.g, trailColor.b, scale);
            leds[trailPos] = trailColor;
        }
    }

    // Smooth speed control using fractional steps-per-frame (x100 fixed-point)
    // config.speed encodes steps-per-frame * 100 (e.g., 120 = 1.20 steps/frame)
    // This allows speeds faster than 1 step per frame while staying smooth.
    float stepsPerFrame = max(1, config.speed) / 100.0f;
    stepAccumulator += stepsPerFrame;
    while (stepAccumulator >= 1.0f)
    {
        effectStep++;
        stepAccumulator -= 1.0f;
    }

    // Check if we've completed a cycle (head + entire trail has exited the strip)
    if (effectStep >= totalSteps)
    {
        completedCycles++;
        effectStep = 0;   // Reset for next cycle
        frameCounter = 0; // Reset frame counter
        stepAccumulator = 0.0f; // Reset fractional speed accumulator
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
    stepAccumulator = 0.0f;
}

void ChaseSingle::clearAllLEDs(CRGB *leds, int ledCount)
{
    for (int i = 0; i < ledCount; i++)
    {
        leds[i] = CRGB::Black;
    }
}

#endif // ENABLE_LEDS
