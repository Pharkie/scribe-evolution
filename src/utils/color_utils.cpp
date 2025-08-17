/**
 * @file color_utils.cpp
 * @brief Implementation of color utilities for LED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "color_utils.h"

#if ENABLE_LEDS

#include "../core/logging.h"

CRGB hexToRgb(const String &hexColor)
{
    String cleanHex = hexColor;

    // Remove # if present
    if (cleanHex.startsWith("#"))
    {
        cleanHex = cleanHex.substring(1);
    }

    // Validate length
    if (cleanHex.length() != 6)
    {
        LOG_WARNING("COLOR", "Invalid hex color length: %s, using white", hexColor.c_str());
        return CRGB::White;
    }

    // Parse hex components
    long hexValue = strtol(cleanHex.c_str(), NULL, 16);

    int r = (hexValue >> 16) & 0xFF;
    int g = (hexValue >> 8) & 0xFF;
    int b = hexValue & 0xFF;

    LOG_VERBOSE("COLOR", "Parsed hex %s -> RGB(%d,%d,%d)", hexColor.c_str(), r, g, b);

    return CRGB(r, g, b);
}

CRGB rgbToRgb(int r, int g, int b)
{
    // Clamp values to valid range
    r = constrain(r, 0, 255);
    g = constrain(g, 0, 255);
    b = constrain(b, 0, 255);

    return CRGB(r, g, b);
}

bool isValidHexColor(const String &hexColor)
{
    String cleanHex = hexColor;

    // Remove # if present
    if (cleanHex.startsWith("#"))
    {
        cleanHex = cleanHex.substring(1);
    }

    // Check length
    if (cleanHex.length() != 6)
    {
        return false;
    }

    // Check if all characters are valid hex
    for (int i = 0; i < cleanHex.length(); i++)
    {
        char c = cleanHex.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
        {
            return false;
        }
    }

    return true;
}

String rgbToHex(const CRGB &color)
{
    char hexBuffer[8];
    sprintf(hexBuffer, "#%02X%02X%02X", color.r, color.g, color.b);
    return String(hexBuffer);
}

#endif // ENABLE_LEDS
