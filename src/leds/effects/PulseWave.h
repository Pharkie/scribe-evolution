/**
 * @file PulseWave.h
 * @brief Sine wave pulse effect
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef PULSE_WAVE_H
#define PULSE_WAVE_H

#include "EffectBase.h"

#if ENABLE_LEDS

/**
 * @brief Sine wave pulse effect
 * Creates a sine wave pattern that pulses across the LED strip
 *
 * Parameters semantics:
 * - speed (1..100): frame delay for phase steps (smaller = faster).
 * - intensity (1..100): brightness range (min brightness).
 * - cycles: one cycle = OFF → peak → OFF (full sine wave 0..360 degrees).
 */
class PulseWave : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param config Configuration for the pulse effect
     */
    PulseWave(const PulseConfig &config);

    /**
     * @brief Update the pulse wave effect
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
    String getName() const override { return "pulse"; }

    /**
     * @brief Get effect name
     */

private:
    PulseConfig config; // Store the autonomous configuration
    int frameCounter;   // Frame counter for speed control
};

#endif // ENABLE_LEDS
#endif // PULSE_WAVE_H
