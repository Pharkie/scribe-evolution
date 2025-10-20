/**
 * @file LedEffects.h
 * @brief Thread-safe LED effects manager using modular effect system
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef LED_EFFECTS_NEW_H
#define LED_EFFECTS_NEW_H

#include <config/config.h>

#if ENABLE_LEDS

#include <Arduino.h>
#include <FastLED.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Forward declarations
class EffectBase;
class EffectRegistry;

/**
 * @brief RAII lock guard for LED mutex
 * Automatically releases mutex when it goes out of scope
 * Prevents mutex leaks and ensures thread-safety on multi-core ESP32-S3
 */
class LedLock
{
private:
    SemaphoreHandle_t mutex;
    bool locked;

public:
    LedLock(SemaphoreHandle_t m, uint32_t timeoutMs = 1000);
    ~LedLock();
    bool isLocked() const { return locked; }

    // Prevent copying
    LedLock(const LedLock&) = delete;
    LedLock& operator=(const LedLock&) = delete;
};

/**
 * @brief Thread-safe LED effects manager using modular effect system
 *
 * Thread-safe for multi-core ESP32-S3 operation using RAII locking:
 * - Public methods acquire mutex using LedLock (RAII)
 * - Internal methods assume mutex is already held by caller
 * - Prevents concurrent access from Core 0 (main loop) and Core 1 (web handlers)
 * - FastLED is NOT thread-safe - this wrapper protects against race conditions
 */
class LedEffects
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the singleton LedEffects instance
     */
    static LedEffects& getInstance();

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
    // Private constructor for singleton pattern
    LedEffects();

    // Private destructor
    ~LedEffects();

    // Prevent copying
    LedEffects(const LedEffects&) = delete;
    LedEffects& operator=(const LedEffects&) = delete;

    // Thread synchronization
    SemaphoreHandle_t mutex;
    bool initialized;

    // LED strip array pointer
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

    // Internal methods - MUST be called with mutex already held
    bool reinitializeInternal(int pin, int count, int brightness, int refreshRate,
                              const LedEffectsConfig &effectsConfig);
    void stopEffectInternal();
};

// Helper inline function for backward compatibility
// Allows existing code to use ledEffects without conflicts
inline LedEffects& ledEffects() {
    return LedEffects::getInstance();
}

#endif // ENABLE_LEDS

#endif // LED_EFFECTS_NEW_H
