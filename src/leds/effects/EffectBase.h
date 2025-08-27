/**
 * @file EffectBase.h
 * @brief Base interface for LED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef EFFECT_BASE_H
#define EFFECT_BASE_H

#include "../../core/config.h"

#if ENABLE_LEDS

#include <Arduino.h>
#include <FastLED.h>

/**
 * @brief Base interface for all LED effects
 */
class EffectBase
{
public:
    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~EffectBase() = default;

    /**
     * @brief Update the effect - called every frame
     * @param leds Pointer to the LED array
     * @param ledCount Number of LEDs in the strip
     * @param effectStep Current step/position in the effect
     * @param effectDirection Current direction (1 or -1)
     * @param effectPhase Current phase (0.0-1.0)
     * @param color1 Primary effect color
     * @param color2 Secondary effect color
     * @param color3 Tertiary effect color
     * @param completedCycles Number of completed cycles (for cycle tracking)
     * @return true if effect should continue, false if complete
     */
    virtual bool update(CRGB *leds, int ledCount, int &effectStep, int &effectDirection,
                        float &effectPhase, CRGB color1, CRGB color2, CRGB color3,
                        int &completedCycles) = 0;

    /**
     * @brief Initialize effect-specific state
     * @param ledCount Number of LEDs in the strip
     */
    virtual void initialize(int ledCount) {}

    /**
     * @brief Reset effect state
     */
    virtual void reset() {}

    /**
     * @brief Get effect name
     * @return String containing the effect name
     */
    virtual String getName() const = 0;
};

#endif // ENABLE_LEDS
#endif // EFFECT_BASE_H
