/**
 * @file PulseWave.h
 * @brief Cosine wave pulse effect (off→on→off)
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
 * @brief Cosine wave pulse effect
 * Creates a smooth cosine-shaped brightness pulse across the LED strip
 *
 * Timing behavior (at 60 Hz refresh rate):
 * - speed=100: 1 second per cycle (fast pulse, 60 smooth steps)
 * - speed=1: 5 seconds per cycle (slow breathing, 300 smooth steps)
 * - Linear interpolation between these endpoints
 *
 * Parameters semantics:
 * - speed (1..100): pulse rate (higher = faster, 100→1sec, 1→5sec per cycle)
 * - intensity (1..100): brightness range (min brightness, 100=off→full, 1=bright→full)
 * - cycles: number of complete pulses (one cycle = OFF → peak → OFF, 360 degrees)
 *
 * Note: Updates every frame for buttery-smooth transitions (no frame skipping)
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
};

#endif // ENABLE_LEDS
#endif // PULSE_WAVE_H
