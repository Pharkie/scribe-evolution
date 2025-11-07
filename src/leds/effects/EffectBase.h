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

#include <config/config.h>

#if ENABLE_LEDS

#include <Arduino.h>
#include <FastLED.h>

/**
 * @brief Base interface for all LED effects with frame-rate independent timing helpers
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
    virtual void reset() { lastUpdateMs = 0; }

    /**
     * @brief Get effect name
     * @return String containing the effect name
     */
    virtual String getName() const = 0;

protected:
    /**
     * @brief Calculate delta time since last update (frame-rate independent timing)
     * @return Delta time in seconds
     * @note First call after reset() returns assumed 16.67ms (60 FPS)
     */
    float calculateDeltaTime()
    {
        unsigned long now = millis();
        float deltaTimeMs = (lastUpdateMs == 0) ? 16.67f : (float)(now - lastUpdateMs);
        lastUpdateMs = now;
        return deltaTimeMs / 1000.0f; // Return seconds
    }

    /**
     * @brief Convert speed (1-100) to cycle duration in seconds
     * @param speed Speed value (1=slowest, 100=fastest)
     * @return Cycle duration in seconds (speed=100→1sec, speed=1→5sec)
     * @note Uses formula: cycleSeconds = (499 - 4×speed) / 99
     */
    static float speedToCycleSeconds(int speed)
    {
        return (499.0f - 4.0f * constrain(speed, 1, 100)) / 99.0f;
    }

    /**
     * @brief Initialize timing state (call in derived class constructor)
     */
    void initTiming() { lastUpdateMs = 0; }

private:
    unsigned long lastUpdateMs = 0; ///< Last update timestamp for delta time calculation
};

#endif // ENABLE_LEDS
#endif // EFFECT_BASE_H
