/**
 * @file ChaseMulti.h
 * @brief Multi-color chase effect with autonomous configuration
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef CHASE_MULTI_H
#define CHASE_MULTI_H

#include "../../config/config.h"

#if ENABLE_LEDS

#include "EffectBase.h"

/**
 * @brief Multi-color chase effect with autonomous per-effect configuration
 *
 * Parameters semantics:
 * - speed (1..100): steps-per-frame ×100. 1 ≈ 0.30 spf (slow), 100 ≈ 1.20 spf (fast).
 * - intensity (1..100): trail length from 2..20 LEDs, linearly mapped.
 * - cycles: one cycle = all colors traverse strip and their trails exit.
 */
class ChaseMulti : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param config Chase multi configuration parameters
     */
    ChaseMulti(const ChaseMultiConfig &config);

    /**
     * @brief Update the chase multi effect
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
    String getName() const override { return "chase_multi"; }

    /**
     * @brief Update configuration for this effect
     * @param newConfig New configuration parameters
     */
    void updateConfig(const ChaseMultiConfig &newConfig) { config = newConfig; }

private:
    ChaseMultiConfig config;

    int targetCycles;
    int frameCounter;
    float stepAccumulator = 0.0f; // fractional step accumulator for smooth speed

    void clearAllLEDs(CRGB *leds, int ledCount);
};

#endif // ENABLE_LEDS
#endif // CHASE_MULTI_H
