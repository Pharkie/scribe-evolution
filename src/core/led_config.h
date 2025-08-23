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

// Chase Single Effect Settings
static const int DEFAULT_CHASE_SINGLE_SPEED = 5;             // Frame delay (higher = slower)
static const int DEFAULT_CHASE_SINGLE_TRAIL_LENGTH = 15;     // Length of trailing fade
static const int DEFAULT_CHASE_SINGLE_TRAIL_FADE = 15;       // Fade amount per trail step (0-255)
static const char *DEFAULT_CHASE_SINGLE_COLOR = "#0062ffff"; // Blue

// Chase Multi Effect Settings
static const int DEFAULT_CHASE_MULTI_SPEED = 2;              // Frame delay (higher = slower)
static const int DEFAULT_CHASE_MULTI_TRAIL_LENGTH = 20;      // Length of trailing fade
static const int DEFAULT_CHASE_MULTI_TRAIL_FADE = 20;        // Fade amount per trail step (0-255)
static const int DEFAULT_CHASE_MULTI_COLOR_SPACING = 12;     // Space between colors
static const char *DEFAULT_CHASE_MULTI_COLOR1 = "#ff9900ff"; // Orange
static const char *DEFAULT_CHASE_MULTI_COLOR2 = "#008f00ff"; // Nice green
static const char *DEFAULT_CHASE_MULTI_COLOR3 = "#78cffeff"; // Cyan

// Matrix Effect Settings
static const int DEFAULT_MATRIX_SPEED = 3;             // Frame delay (higher = slower)
static const int DEFAULT_MATRIX_DROPS = 5;             // Number of simultaneous drops
static const int DEFAULT_MATRIX_BACKGROUND_FADE = 64;  // Background fade amount
static const int DEFAULT_MATRIX_TRAIL_FADE = 32;       // Trail fade amount
static const int DEFAULT_MATRIX_BRIGHTNESS_FADE = 40;  // Brightness fade per position
static const char *DEFAULT_MATRIX_COLOR = "#009100ff"; // Default green

// Twinkle Effect Settings
static const int DEFAULT_TWINKLE_DENSITY = 8;          // Number of simultaneous stars
static const int DEFAULT_TWINKLE_FADE_SPEED = 5;       // Fade transition speed
static const int DEFAULT_TWINKLE_MIN_BRIGHTNESS = 50;  // Minimum star brightness
static const int DEFAULT_TWINKLE_MAX_BRIGHTNESS = 255; // Maximum star brightness
static const char *DEFAULT_TWINKLE_COLOR = "#FFFFFF";  // Default white

// Pulse Effect Settings
static const int DEFAULT_PULSE_SPEED = 4;                // Frame delay (higher = slower)
static const int DEFAULT_PULSE_MIN_BRIGHTNESS = 0;       // Minimum brightness in pulse cycle
static const int DEFAULT_PULSE_MAX_BRIGHTNESS = 255;     // Maximum brightness in pulse cycle
static const float DEFAULT_PULSE_WAVE_FREQUENCY = 0.05f; // Wave frequency multiplier
static const char *DEFAULT_PULSE_COLOR = "#ff00f2ff";    // Hot pink

// Rainbow Effect Settings
static const float DEFAULT_RAINBOW_SPEED = 2.0f;    // Animation speed multiplier
static const int DEFAULT_RAINBOW_SATURATION = 255;  // Color saturation (0-255)
static const int DEFAULT_RAINBOW_BRIGHTNESS = 255;  // Overall brightness (0-255)
static const float DEFAULT_RAINBOW_HUE_STEP = 2.5f; // Hue step between LEDs

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
