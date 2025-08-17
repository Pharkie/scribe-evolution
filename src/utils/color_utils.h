/**
 * @file color_utils.h
 * @brief Color utilities for LED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include "../core/config.h"

#ifdef ENABLE_LEDS

#include <Arduino.h>
#include <FastLED.h>

/**
 * @brief Convert hex color string to CRGB
 * @param hexColor Hex color string (e.g., "#FF0000" or "FF0000")
 * @return CRGB color object
 */
CRGB hexToRgb(const String &hexColor);

/**
 * @brief Convert RGB array to CRGB
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return CRGB color object
 */
CRGB rgbToRgb(int r, int g, int b);

/**
 * @brief Validate hex color string format
 * @param hexColor Hex color string to validate
 * @return true if valid format, false otherwise
 */
bool isValidHexColor(const String &hexColor);

/**
 * @brief Convert CRGB to hex string
 * @param color CRGB color object
 * @return Hex color string (e.g., "#FF0000")
 */
String rgbToHex(const CRGB &color);

#endif // ENABLE_LEDS
#endif // COLOR_UTILS_H
