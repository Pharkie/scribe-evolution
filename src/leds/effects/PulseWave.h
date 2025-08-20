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

#ifdef ENABLE_LEDS

/**
 * @brief Sine wave pulse effect
 * Creates a sine wave pattern that pulses across the LED strip
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
     * @brief Check if this effect supports cycle-based operation
     */
    bool isCycleBased() const override { return true; } // Now cycle-based: 1 cycle = 1 complete pulse (0 -> max -> 0)

private:
    PulseConfig config; // Store the autonomous configuration
    int frameCounter;   // Frame counter for speed control
};

#endif // ENABLE_LEDS
#endif // PULSE_WAVE_H
