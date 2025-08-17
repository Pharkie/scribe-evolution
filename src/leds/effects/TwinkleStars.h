/**
 * @file TwinkleStars.h
 * @brief Twinkling stars effect with random sparkles
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef TWINKLE_STARS_H
#define TWINKLE_STARS_H

#include "EffectBase.h"

#ifdef ENABLE_LEDS

/**
 * @brief Twinkling stars effect with random sparkles
 * Creates random twinkling points of light across the LED strip
 */
class TwinkleStars : public EffectBase
{
public:
    /**
     * @brief Constructor
     * @param config Configuration for the twinkle effect
     */
    TwinkleStars(const TwinkleConfig &config);

    /**
     * @brief Destructor - cleanup allocated memory
     */
    ~TwinkleStars();

    /**
     * @brief Initialize effect with LED count
     */
    void initialize(int ledCount) override;

    /**
     * @brief Update the twinkle stars effect
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
    String getName() const override { return "twinkle"; }

    /**
     * @brief Check if this effect supports cycle-based operation
     */
    bool isCycleBased() const override { return false; } // Duration-based

    /**
     * @brief Set twinkle density
     * @param density Number of simultaneous twinkle stars
     */
    void setDensity(int density);

    /**
     * @brief Set fade speed
     * @param fadeSpeed Background fade speed
     */
    void setFadeSpeed(int fadeSpeed) { config.fadeSpeed = fadeSpeed; }

private:
    TwinkleConfig config; // Store the autonomous configuration
    struct TwinkleState
    {
        int position;
        int brightness;
        int fadeDirection;
        bool active;
    };

    TwinkleState *twinkleStars;
    bool initialized;
    int frameCounter; // Frame counter for speed control

    void deallocateStars();
    void fadeToBlackBy(CRGB *leds, int ledIndex, int fadeValue);
    int random16(int max);
};

#endif // ENABLE_LEDS
#endif // TWINKLE_STARS_H
