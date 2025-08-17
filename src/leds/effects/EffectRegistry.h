/**
 * @file EffectRegistry.h
 * @brief Registry for managing LED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef EFFECT_REGISTRY_H
#define EFFECT_REGISTRY_H

#include "../../core/config.h"

#ifdef ENABLE_LEDS

#include <Arduino.h>
#include <memory>
#include "EffectBase.h"
#include "ChaseSingle.h"
#include "RainbowWave.h"
#include "TwinkleStars.h"
#include "ChaseMulti.h"
#include "PulseWave.h"
#include "Matrix.h"

/**
 * @brief Registry for managing and creating LED effects
 */
class EffectRegistry
{
public:
    /**
     * @brief Constructor
     * @param chaseSpeed Speed for chase effects
     * @param twinkleDensity Density for twinkle effect
     * @param fadeSpeed Fade speed for twinkle effect
     * @param matrixDrops Number of drops for matrix effect
     */
    EffectRegistry(int chaseSpeed, int twinkleDensity, int fadeSpeed, int matrixDrops);

    /**
     * @brief Destructor
     */
    ~EffectRegistry();

    /**
     * @brief Create an effect by name
     * @param effectName Name of the effect to create
     * @return Pointer to the effect, or nullptr if not found
     */
    EffectBase *createEffect(const String &effectName);

    /**
     * @brief Check if an effect name is valid
     * @param effectName Name to check
     * @return true if valid, false otherwise
     */
    bool isValidEffect(const String &effectName) const;

    /**
     * @brief Get list of all available effect names
     * @return String containing comma-separated effect names
     */
    String getAvailableEffects() const;

    /**
     * @brief Update configuration parameters
     * @param chaseSpeed New chase speed
     * @param twinkleDensity New twinkle density
     * @param fadeSpeed New fade speed
     * @param matrixDrops New matrix drops count
     */
    void updateConfig(int chaseSpeed, int twinkleDensity, int fadeSpeed, int matrixDrops);

private:
    int chaseSpeed;
    int twinkleDensity;
    int fadeSpeed;
    int matrixDrops;

    static const String effectNames[];
    static const int numEffects;

    String toLowerCase(const String &str) const;
};

#endif // ENABLE_LEDS
#endif // EFFECT_REGISTRY_H
