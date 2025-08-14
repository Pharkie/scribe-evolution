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
     * @brief Initialize the LED strip and effects manager
     * Must be called in setup() before using any effects
     * @return true if initialization successful, false otherwise
     */
    bool begin();

    /**
     * @brief Update the current effect - call this in the main loop
     * This method is non-blocking and handles timing internally
     */
    void update();

    /**
     * @brief Start a new LED effect
     * @param effectName Name of the effect (case-insensitive)
     * @param durationSeconds Duration to run the effect (0 = infinite)
     * @param color1 Primary color for the effect (default: blue)
     * @param color2 Secondary color for the effect (default: black/off)
     * @param color3 Tertiary color for the effect (default: black/off)
     * @return true if effect started successfully, false if unknown effect
     */
    bool startEffect(const String& effectName, unsigned long durationSeconds = 0, 
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
    // LED strip array
    CRGB leds[LED_COUNT];
    
    // Effect state variables
    bool effectActive;
    String currentEffectName;
    unsigned long effectStartTime;
    unsigned long effectDuration;  // 0 = infinite
    unsigned long lastUpdate;
    
    // Effect parameters
    CRGB effectColor1;
    CRGB effectColor2;
    CRGB effectColor3;
    
    // Effect-specific state variables
    int effectStep;
    int effectDirection;
    float effectPhase;
    
    // Twinkle effect state
    struct TwinkleState {
        int position;
        int brightness;
        int fadeDirection;
        bool active;
    };
    TwinkleState twinkleStars[LED_TWINKLE_DENSITY];
    
    // Matrix effect state  
    struct MatrixDrop {
        int position;
        int length;
        int speed;
        bool active;
    };
    MatrixDrop matrixDrops[LED_MATRIX_DROPS];

    // Internal effect methods
    void clearAllLEDs();
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