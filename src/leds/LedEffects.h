/**
 * @file LedEffects.h
 * @brief LED effects manager for WS2812B LED strips using FastLED library
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 */

#ifndef LED_EFFECTS_H
#define LED_EFFECTS_H

#include "../core/config.h"

#ifdef ENABLE_LEDS

#include <Arduino.h>
#include <FastLED.h>

/**
 * @brief Non-blocking LED effects manager for WS2812B strips
 * 
 * This class provides a framework for creating time-based LED effects that
 * integrate seamlessly with the main application loop without blocking.
 * All effects are parameterized and can be started, stopped, and replaced.
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
     * @param fadeSpeed Fade speed for transitions (1-255)
     * @param twinkleDensity Number of twinkle stars simultaneously
     * @param chaseSpeed Chase effect speed (pixels per update)
     * @param matrixDrops Number of matrix drops simultaneously
     * @return true if reinitialization successful, false otherwise
     */
    bool reinitialize(int pin, int count, int brightness, int refreshRate,
                     int fadeSpeed, int twinkleDensity, int chaseSpeed, int matrixDrops);

    /**
     * @brief Update the current effect - call this in the main loop
     * This method is non-blocking and handles timing internally
     */
    void update();

    /**
     * @brief Start a new LED effect (duration-based)
     * @param effectName Name of the effect (case-insensitive)
     * @param durationSeconds Duration to run the effect (0 = infinite)
     * @param color1 Primary color for the effect (default: blue)
     * @param color2 Secondary color for the effect (default: black/off)
     * @param color3 Tertiary color for the effect (default: black/off)
     * @return true if effect started successfully, false if unknown effect
     */
    bool startEffectDuration(const String& effectName, unsigned long durationSeconds = 0, 
                            CRGB color1 = CRGB::Blue, CRGB color2 = CRGB::Black, CRGB color3 = CRGB::Black);

    /**
     * @brief Start a new LED effect (cycle-based)
     * @param effectName Name of the effect (case-insensitive)
     * @param cycles Number of cycles/sequences to run (1 = single sequence)
     * @param color1 Primary color for the effect (default: blue)
     * @param color2 Secondary color for the effect (default: black/off)
     * @param color3 Tertiary color for the effect (default: black/off)
     * @return true if effect started successfully, false if unknown effect
     */
    bool startEffectCycles(const String& effectName, int cycles = 1, 
                          CRGB color1 = CRGB::Blue, CRGB color2 = CRGB::Black, CRGB color3 = CRGB::Black);

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

private:
    // LED strip array (dynamically allocated)
    CRGB* leds;
    
    // Runtime LED configuration
    int ledCount;
    int ledPin;
    int ledBrightness;
    int ledRefreshRate;
    unsigned long ledUpdateInterval;
    int ledEffectFadeSpeed;
    int ledTwinkleDensity;
    int ledChaseSpeed;
    int ledMatrixDrops;
    
    // Effect state variables
    bool effectActive;
    String currentEffectName;
    unsigned long effectStartTime;
    unsigned long effectDuration;  // 0 = infinite (for duration-based effects)
    unsigned long lastUpdate;
    
    // Effect mode and cycle tracking
    bool isCycleBased;             // true for cycle-based, false for duration-based
    int targetCycles;              // Number of cycles to complete
    int completedCycles;           // Number of cycles completed so far
    
    // Effect parameters
    CRGB effectColor1;
    CRGB effectColor2;
    CRGB effectColor3;
    
    // Effect-specific state variables
    int effectStep;
    int effectDirection;
    float effectPhase;
    
    // Twinkle effect state (dynamically allocated)
    struct TwinkleState {
        int position;
        int brightness;
        int fadeDirection;
        bool active;
    };
    TwinkleState* twinkleStars;
    
    // Matrix effect state (dynamically allocated)
    struct MatrixDrop {
        int position;
        int length;
        int speed;
        bool active;
    };
    MatrixDrop* matrixDrops;

    // Internal effect methods
    void clearAllLEDs();
    void allocateArrays();
    void deallocateArrays();
    void updateSimpleChase();
    void updateRainbowWave();
    void updateTwinkleStars();
    void updateColorChase();
    void updatePulseWave();
    void updateMatrixMovie();
    
    // Utility methods
    CRGB wheel(byte wheelPos);
    void fadeToBlackBy(int ledIndex, int fadeValue);
    int random16(int max);
};

extern LedEffects ledEffects;

#endif // ENABLE_LEDS

#endif // LED_EFFECTS_H