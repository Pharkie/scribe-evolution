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

#include "../../config/config.h"

#if ENABLE_LEDS

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
     * @brief Constructor with autonomous per-effect configuration
     * @param effectsConfig Complete effects configuration structure
     */
    EffectRegistry(const LedEffectsConfig &effectsConfig);

    /**
     * @brief Destructor
     */
    ~EffectRegistry();

    /**
     * @brief Create an effect by name using autonomous configuration
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
     * @brief Update autonomous configuration for all effects
     * @param newEffectsConfig New complete effects configuration
     */
    void updateConfig(const LedEffectsConfig &newEffectsConfig);

    /**
     * @brief Get default color hex strings for an effect from autonomous config
     * @param effectName Effect name (case-insensitive)
     * @param h1 Out: primary color hex (e.g., #FF0000)
     * @param h2 Out: secondary color hex (may be empty if unused)
     * @param h3 Out: tertiary color hex (may be empty if unused)
     */
    void getDefaultColorsHex(const String &effectName, String &h1, String &h2, String &h3) const;

private:
    LedEffectsConfig effectsConfig; // Autonomous per-effect configurations

    static const String effectNames[];
    static const int numEffects;

    String toLowerCase(const String &str) const;
};

#endif // ENABLE_LEDS
#endif // EFFECT_REGISTRY_H
