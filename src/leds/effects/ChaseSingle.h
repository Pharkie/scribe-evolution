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

#include "../../core/config.h"

#ifdef ENABLE_LEDS

#include "EffectBase.h"

/**
 * @brief Single color chase effect with fading trail
 */
class ChaseSingle : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param chaseSpeed Speed of the chase effect (pixels per update)
     */
    ChaseSingle(int chaseSpeed);

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
#endif // CHASE_SINGLE_H
