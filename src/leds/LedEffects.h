/**
 * @file LedEffectsNew.h
 * @brief Refactored LED effects manager using modular effect system
 * @author Adam Knowles
 * @date 2025
     // Effect state
    bool effectActive;
    String currentEffectName;
    unsigned long effectStartTime;
    unsigned long lastUpdate;

    // Cycle tracking
    int targetCycles;    // Number of cycles to complete (0 = infinite)
    int completedCycles; // Number of cycles completed so fart Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef LED_EFFECTS_NEW_H
#define LED_EFFECTS_NEW_H

#include <config/config.h>

#if ENABLE_LEDS

#include <Arduino.h>
#include <FastLED.h>

// Forward declarations
class EffectBase;
class EffectRegistry;

/**
 * @brief Non-blocking LED effects manager using modular effect system
 *
 * This refactored class provides a cleaner architecture where individual effects
 * are implemented as separate classes, making the code more maintainable and extensible.
 */
class LedEffects
{
public:
    /**
     * @brief Constructor - initializes the LED effects manager
     */
    LedEffects();

    /**
     * @brief Destructor - cleans up dynamically allocated memory
     */
    ~LedEffects();

    /**
     * @brief Initialize the LED strip and effects manager
     * Must be called in setup() before using any effects
     * @return true if initialization successful, false otherwise
     */
    bool begin();

    /**
     * @brief Reinitialize LED strip with new configuration
     * Used when LED settings are changed at runtime
     * @param pin GPIO pin for LED strip data
     * @param count Number of LEDs in the strip
     * @param brightness LED brightness (0-255)
     * @param refreshRate Refresh rate in Hz (used to calculate update interval)
     * @param effectsConfig Autonomous configuration for all effects
     * @return true if reinitialization successful, false otherwise
     */
    bool reinitialize(int pin, int count, int brightness, int refreshRate,
                      const LedEffectsConfig &effectsConfig);

    /**
     * @brief Update the current effect - call this in the main loop
     * This method is non-blocking and handles timing internally
     */
    void update();

    /**
     * @brief Start a new LED effect (cycle-based)
     * @param effectName Name of the effect (case-insensitive)
     * @param cycles Number of cycles/sequences to run (0 = infinite)
     * @param color1 Primary color for the effect (default: blue)
     * @param color2 Secondary color for the effect (default: black/off)
     * @param color3 Tertiary color for the effect (default: black/off)
     * @return true if effect started successfully, false if unknown effect
     */
    bool startEffectCycles(const String &effectName, int cycles = 0,
                           CRGB color1 = CRGB::Blue, CRGB color2 = CRGB::Black, CRGB color3 = CRGB::Black);

    /**
     * @brief Start a new LED effect using autonomous default colors from configuration
     * @param effectName Name of the effect (case-insensitive)
     * @param cycles Number of cycles/sequences to run (0 = infinite)
     * @return true if effect started successfully, false if unknown effect
     */
    bool startEffectCyclesAuto(const String &effectName, int cycles);

    /**
     * @brief Stop the current effect and turn off all LEDs
     */
    void stopEffect();

    /**
     * @brief Check if an effect is currently running
     * @return true if an effect is active, false otherwise
     */
    bool isEffectRunning() const;

    /**
     * @brief Get the name of the currently running effect
     * @return String containing the effect name, or empty string if none
     */
    String getCurrentEffectName() const;

    /**
     * @brief Get remaining time for the current effect
     * @return Remaining time in milliseconds, 0 if infinite or no effect
     */
    unsigned long getRemainingTime() const;

    /**
     * @brief Temporarily update effect configuration for playground use
     * @param newConfig New effects configuration to use
     */
    void updateEffectConfig(const LedEffectsConfig &newConfig);

private:
    // LED strip array
    CRGB *leds;

    // Runtime LED configuration
    int ledCount;
    int ledPin;
    int ledBrightness;
    int ledRefreshRate;
    unsigned long ledUpdateInterval;
    int ledEffectFadeSpeed;
    int ledTwinkleDensity;
    int ledChaseSingleSpeed;
    int ledChaseMultiSpeed;
    int ledMatrixDrops;

    // Effect state variables
    bool effectActive;
    String currentEffectName;
    unsigned long effectStartTime;
    unsigned long effectDuration; // 0 = infinite (for duration-based effects)
    unsigned long lastUpdate;

    // Effect mode and cycle tracking
    bool isCycleBased;   // true for cycle-based, false for duration-based
    int targetCycles;    // Number of cycles to complete
    int completedCycles; // Number of cycles completed so far

    // Effect parameters
    CRGB effectColor1;
    CRGB effectColor2;
    CRGB effectColor3;

    // Effect-specific state variables (passed to effects)
    int effectStep;
    int effectDirection;
    float effectPhase;

    // Modular effect system
    EffectBase *currentEffect;
    EffectRegistry *effectRegistry;

    // Final fade management (manager-driven)
    bool finalFadeActive;
    unsigned long finalFadeStart;
    static constexpr unsigned long finalFadeDurationMs = 3000; // 3s fade-out
    CRGB *finalFadeBase = nullptr; // Snapshot of LEDs at fade start
};

extern LedEffects ledEffects;

#endif // ENABLE_LEDS

#endif // LED_EFFECTS_NEW_H
