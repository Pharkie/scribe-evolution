/**
 * @file ChaseMultiEffect.h
 * @brief Multi-color chase effect cycling through three colors
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef CHASE_MULTI_H
#define CHASE_MULTI_H

#include "EffectBase.h"

#ifdef ENABLE_LEDS

/**
 * @brief Multi-color chase effect with trailing fade
 * A LED travels along the strip cycling through three colors with a fading trail.
 * Supports both cycle-based (color change per cycle) and duration-based modes.
 */
class ChaseMulti : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param chaseSpeed Speed of the chase effect (pixels per update)
     */
    ChaseMulti(int chaseSpeed);

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
     * @brief Check if this effect supports cycle-based operation
     */
    bool isCycleBased() const override { return true; }

    /**
     * @brief Set chase speed
     * @param speed New chase speed (pixels per update)
     */
    void setChaseSpeed(int speed) { chaseSpeed = speed; }

private:
    int chaseSpeed;
    bool isCycleBasedMode;
    int targetCycles;

    void clearAllLEDs(CRGB *leds, int ledCount);
};

#endif // ENABLE_LEDS
#endif // CHASE_MULTI_H
