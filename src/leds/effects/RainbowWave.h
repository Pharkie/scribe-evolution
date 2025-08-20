/**
 * @file RainbowWave.h
 * @brief Rainbow wave effect that cycles through hues
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef RAINBOW_WAVE_H
#define RAINBOW_WAVE_H

#include "EffectBase.h"

#ifdef ENABLE_LEDS

/**
 * @brief Rainbow wave effect that cycles through the color spectrum
 * Creates a moving rainbow pattern across the LED strip
 */
class RainbowWave : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param config Configuration for the rainbow effect
     */
    RainbowWave(const RainbowConfig &config);

    /**
     * @brief Update the rainbow wave effect
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
    String getName() const override { return "rainbow"; }

    /**
     * @brief Get effect name
     */

private:
    RainbowConfig config; // Store the autonomous configuration
    int frameCounter;     // Frame counter for speed control

    /**
     * @brief Convert hue to RGB color (color wheel function)
     * @param wheelPos Position on the color wheel (0-255)
     * @return CRGB color
     */
    CRGB wheel(byte wheelPos);
};

#endif // ENABLE_LEDS
#endif // RAINBOW_WAVE_H
