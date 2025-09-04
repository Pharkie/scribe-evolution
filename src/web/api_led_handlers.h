/**
 * @file api_led_handlers.h
 * @brief LED API endpoint handlers for FastLED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_LED_HANDLERS_H
#define API_LED_HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

#ifdef ENABLE_LEDS

/**
 * @brief Handle LED effect trigger requests
 * @param request The HTTP request containing effect parameters
 *
 * Endpoint: POST /api/leds/test
 * Body JSON: { effect, speed, intensity, cycles, colors }
 * Example: { "effect": "chase_single", "speed": 50, "intensity": 50, "cycles": 3, "colors": ["#0062ff"] }
 *
 * Supported effects: chase_single, chase_multi, rainbow, twinkle, pulse, matrix
 * Supported colors: red, green, blue, yellow, purple, cyan, white, orange, pink, black
 */
void handleLedEffect(AsyncWebServerRequest *request);

/**
 * @brief Handle LED off request (stops all effects)
 * @param request The HTTP request
 *
 * Endpoint: POST /api/leds/off
 */
void handleLedOff(AsyncWebServerRequest *request);

#endif // ENABLE_LEDS

#endif // API_LED_HANDLERS_H
