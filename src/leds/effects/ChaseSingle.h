/**
 * @file ChaseSingle.h
 * @brief Single color chase LED effect with trail
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef CHASE_SINGLE_H
#define CHASE_SINGLE_H

#include <config/config.h>

#if ENABLE_LEDS

#include "EffectBase.h"

/**
 * @brief Single color chase effect with fading trail
 *
 * Timing behavior (LED-count independent at 60 Hz):
 * - speed=100: 1 second per cycle (fast chase)
 * - speed=1: 5 seconds per cycle (slow chase)
 * - Linear interpolation between endpoints
 * - Chase moves faster on longer strips to maintain consistent timing
 *
 * Parameters semantics:
 * - speed (1..100): chase rate (higher = faster, 100→1sec, 1→5sec per cycle)
 * - intensity (1..100): trail length from 2..20 LEDs, linearly mapped
 * - cycles: number of complete traversals (one cycle = head + trail fully exits strip)
 *
 * Note: Time-based - 30 LED and 160 LED strips complete in same duration
 */
class ChaseSingle : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param config Configuration for the chase single effect
     */
    ChaseSingle(const ChaseSingleConfig &config);

    /**
     * @brief Update the chase single effect
     */
    bool update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                int &completedCycles) override;

    /**
     * @brief Reset effect state
     */
    void reset() override;

    /**
     * @brief Get effect name
     */
    String getName() const override { return "chase_single"; }

    /**
     * @brief Get effect name
     */

    /**
     * @brief Set chase speed
     * @param speed New chase speed delay (higher = slower)
     */
    void setChaseSpeed(int speed) { config.speed = speed; }

private:
    ChaseSingleConfig config;

    int targetCycles;
    float stepAccumulator = 0.0f; // fractional step accumulator for smooth speed

    void clearAllLEDs(CRGB *leds, int ledCount);
};

#endif // ENABLE_LEDS
#endif // CHASE_SINGLE_H
