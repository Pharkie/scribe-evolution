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

#include "../../core/config.h"

#ifdef ENABLE_LEDS

#include "EffectBase.h"

/**
 * @brief Multi-color chase effect with autonomous per-effect configuration
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

    void clearAllLEDs(CRGB *leds, int ledCount);
};

#endif // ENABLE_LEDS
#endif // CHASE_MULTI_H
