/**
 * @file api_led_handlers.cpp
 * @brief LED API endpoint handlers for FastLED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_led_handlers.h"

#ifdef ENABLE_LEDS

#include "api_handlers.h" // For sendErrorResponse
#include "../core/config.h"
#include "../core/logging.h"
#include "../leds/LedEffects.h"
#include <ArduinoJson.h>
#include <FastLED.h>

// External LED effects instance
extern LedEffects ledEffects;

// ========================================
// LED API HANDLERS
// ========================================

void handleLedEffect(AsyncWebServerRequest *request)
{
    // Validate request method
    if (request->method() != HTTP_POST)
    {
        sendErrorResponse(request, 405, "Method not allowed");
        return;
    }

    // Get effect name from URL path
    String path = request->url();
    String effectName = "";

    if (path.startsWith("/api/led/"))
    {
        effectName = path.substring(9); // Remove "/api/led/" prefix
    }
    else
    {
        sendErrorResponse(request, 400, "Invalid LED effect endpoint");
        return;
    }

    // Validate effect name
    if (effectName.length() == 0)
    {
        sendErrorResponse(request, 400, "No effect name provided");
        return;
    }

    LOG_VERBOSE("LEDS", "LED effect request: %s", effectName.c_str());

    // Parse optional parameters from JSON body
    int duration = 5;       // Default 5 seconds
    String color1 = "blue"; // Default color
    String color2 = "black";
    String color3 = "black";

    if (request->hasParam("body", true))
    {
        String body = request->getParam("body", true)->value();
        if (body.length() > 0)
        {
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, body);

            if (!error)
            {
                duration = doc["duration"] | duration;
                color1 = doc["color1"] | color1;
                color2 = doc["color2"] | color2;
                color3 = doc["color3"] | color3;
            }
        }
    }

    // Convert color names to CRGB values
    auto parseColor = [](const String &colorName) -> CRGB
    {
        String lower = colorName;
        lower.toLowerCase();

        if (lower == "red")
            return CRGB::Red;
        if (lower == "green")
            return CRGB::Green;
        if (lower == "blue")
            return CRGB::Blue;
        if (lower == "yellow")
            return CRGB::Yellow;
        if (lower == "purple")
            return CRGB::Purple;
        if (lower == "cyan")
            return CRGB::Cyan;
        if (lower == "white")
            return CRGB::White;
        if (lower == "orange")
            return CRGB::Orange;
        if (lower == "pink")
            return CRGB::Pink;
        return CRGB::Black; // Default to off
    };

    CRGB c1 = parseColor(color1);
    CRGB c2 = parseColor(color2);
    CRGB c3 = parseColor(color3);

    // Start the LED effect
    bool success = ledEffects.startEffect(effectName, duration, c1, c2, c3);

    if (success)
    {
        LOG_NOTICE("LEDS", "Started LED effect: %s for %d seconds", effectName.c_str(), duration);

        DynamicJsonDocument response(256);
        response["success"] = true;
        response["message"] = "LED effect started";
        response["effect"] = effectName;
        response["duration"] = duration;
        response["color1"] = color1;
        response["color2"] = color2;
        response["color3"] = color3;

        String responseStr;
        serializeJson(response, responseStr);
        request->send(200, "application/json", responseStr);
    }
    else
    {
        LOG_ERROR("LEDS", "Failed to start LED effect: %s", effectName.c_str());
        sendErrorResponse(request, 400, "Unknown LED effect: " + effectName);
    }
}

void handleLedOff(AsyncWebServerRequest *request)
{
    // Validate request method
    if (request->method() != HTTP_POST)
    {
        sendErrorResponse(request, 405, "Method not allowed");
        return;
    }

    LOG_VERBOSE("LEDS", "LED off request");

    // Stop current effect and turn off LEDs
    ledEffects.stopEffect();

    LOG_NOTICE("LEDS", "LEDs turned off");

    DynamicJsonDocument response(256);
    response["success"] = true;
    response["message"] = "LEDs turned off";

    String responseStr;
    serializeJson(response, responseStr);
    request->send(200, "application/json", responseStr);
}

#endif // ENABLE_LEDS
