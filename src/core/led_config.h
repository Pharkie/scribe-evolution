/**
 * @file led_config.h
 * @brief LED configuration settings and structures
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include "config.h"

#ifdef ENABLE_LEDS

#include <Arduino.h>

// ============================================================================
// SHARED SYSTEM-WIDE LED HARDWARE SETTINGS
// ============================================================================

// LED Hardware Configuration - Runtime Configurable Defaults
static const int DEFAULT_LED_PIN = 20;                                                    // GPIO pin for LED strip data (safe, digital output)
static const int DEFAULT_LED_COUNT = 30;                                                  // Number of LEDs in the strip
static const int DEFAULT_LED_BRIGHTNESS = 64;                                             // Default brightness (0-255, 64 = 25% to save power)
static const int DEFAULT_LED_REFRESH_RATE = 60;                                           // Refresh rate in Hz
static const unsigned long DEFAULT_LED_UPDATE_INTERVAL = 1000 / DEFAULT_LED_REFRESH_RATE; // Update interval in milliseconds

// ============================================================================
// PER-EFFECT AUTONOMOUS SETTINGS
// ============================================================================

// ============================================================================
// UNIFIED LED EFFECT DEFAULTS (1-100 Scale, 50 = Ideal)
// ============================================================================
// Speed and Intensity use 1-100 scale where:
// - 50 = ideal setting most people want
// - 25 = 50% slower/weaker than ideal  
// - 100 = 200% faster/stronger than ideal (double)

static const int DEFAULT_LED_EFFECT_SPEED = 50;        // Universal speed default (1-100 scale)
static const int DEFAULT_LED_EFFECT_INTENSITY = 50;    // Universal intensity default (1-100 scale)  
static const int DEFAULT_LED_EFFECT_CYCLES = 3;        // Default number of cycles to run

// Chase Single Effect Settings
static const int DEFAULT_CHASE_SINGLE_SPEED = DEFAULT_LED_EFFECT_SPEED;      // Speed (1-100, 50=ideal)
static const int DEFAULT_CHASE_SINGLE_INTENSITY = DEFAULT_LED_EFFECT_INTENSITY; // Trail length intensity (1-100, 50=ideal)
static const int DEFAULT_CHASE_SINGLE_CYCLES = DEFAULT_LED_EFFECT_CYCLES;    // Number of cycles
static const char *DEFAULT_CHASE_SINGLE_COLOR = "#0062ff"; // Blue

// Chase Multi Effect Settings  
static const int DEFAULT_CHASE_MULTI_SPEED = DEFAULT_LED_EFFECT_SPEED;       // Speed (1-100, 50=ideal)
static const int DEFAULT_CHASE_MULTI_INTENSITY = DEFAULT_LED_EFFECT_INTENSITY; // Trail length intensity (1-100, 50=ideal)
static const int DEFAULT_CHASE_MULTI_CYCLES = DEFAULT_LED_EFFECT_CYCLES;     // Number of cycles
static const int DEFAULT_CHASE_MULTI_COLOR_SPACING = 3;                      // Space between colors
static const char *DEFAULT_CHASE_MULTI_COLOR1 = "#ff0000"; // Red
static const char *DEFAULT_CHASE_MULTI_COLOR2 = "#00ff00"; // Green  
static const char *DEFAULT_CHASE_MULTI_COLOR3 = "#0000ff"; // Blue

// Matrix Effect Settings
static const int DEFAULT_MATRIX_SPEED = DEFAULT_LED_EFFECT_SPEED;            // Speed (1-100, 50=ideal)
static const int DEFAULT_MATRIX_INTENSITY = DEFAULT_LED_EFFECT_INTENSITY;    // Number of drops (1-100, 50=ideal)  
static const int DEFAULT_MATRIX_CYCLES = DEFAULT_LED_EFFECT_CYCLES;          // Number of cycles
static const char *DEFAULT_MATRIX_COLOR = "#00ff00"; // Green

// Twinkle Effect Settings
static const int DEFAULT_TWINKLE_SPEED = DEFAULT_LED_EFFECT_SPEED;           // Twinkle rate & fade speed (1-100, 50=ideal)
static const int DEFAULT_TWINKLE_INTENSITY = DEFAULT_LED_EFFECT_INTENSITY;   // Number of active twinkles (1-100, 50=ideal)
static const int DEFAULT_TWINKLE_CYCLES = DEFAULT_LED_EFFECT_CYCLES;         // Number of cycles
static const char *DEFAULT_TWINKLE_COLOR = "#ffff00"; // Yellow

// Pulse Effect Settings
static const int DEFAULT_PULSE_SPEED = DEFAULT_LED_EFFECT_SPEED;             // Pulse rate (1-100, 50=ideal)
static const int DEFAULT_PULSE_INTENSITY = DEFAULT_LED_EFFECT_INTENSITY;     // Brightness variation (1-100, 50=ideal)
static const int DEFAULT_PULSE_CYCLES = DEFAULT_LED_EFFECT_CYCLES;           // Number of cycles  
static const char *DEFAULT_PULSE_COLOR = "#800080"; // Purple

// Rainbow Effect Settings
static const int DEFAULT_RAINBOW_SPEED = DEFAULT_LED_EFFECT_SPEED;           // Wave movement speed (1-100, 50=ideal)
static const int DEFAULT_RAINBOW_INTENSITY = DEFAULT_LED_EFFECT_INTENSITY;   // Wave length/density (1-100, 50=ideal)
static const int DEFAULT_RAINBOW_CYCLES = DEFAULT_LED_EFFECT_CYCLES;         // Number of cycles

// ============================================================================
// PER-EFFECT CONFIGURATION STRUCTURES
// ============================================================================

struct ChaseSingleConfig
{
    int speed;
    int trailLength;
    int trailFade;
    String defaultColor;
};

struct ChaseMultiConfig
{
    int speed;
    int trailLength;
    int trailFade;
    int colorSpacing;
    String color1;
    String color2;
    String color3;
};

struct MatrixConfig
{
    int speed;
    int drops;
    int backgroundFade;
    int trailFade;
    int brightnessFade;
    String defaultColor;
};

struct TwinkleConfig
{
    int density;
    int fadeSpeed;
    int minBrightness;
    int maxBrightness;
    String defaultColor;
};

struct PulseConfig
{
    int speed;
    int minBrightness;
    int maxBrightness;
    float waveFrequency;
    String defaultColor;
};

struct RainbowConfig
{
    float speed;
    int saturation;
    int brightness;
    float hueStep;
};

// Master LED Effects Configuration
struct LedEffectsConfig
{
    ChaseSingleConfig chaseSingle;
    ChaseMultiConfig chaseMulti;
    MatrixConfig matrix;
    TwinkleConfig twinkle;
    PulseConfig pulse;
    RainbowConfig rainbow;
};

#endif // ENABLE_LEDS
#endif // LED_CONFIG_H
