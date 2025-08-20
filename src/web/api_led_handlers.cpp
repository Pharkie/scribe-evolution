/**
 * @file api_led_handlers.cpp
 * @brief LED API endpoint handlers for FastLED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyr        LOG_ERROR("LEDS", "No         if (effectHasCycles(effect))
        {
            response["cycles"] = cycles;
        }
        else
        {
            response["duration"] = duration;
        }

        // Add color information to response
        JsonArray responseColors = response.createNestedArray("colors");
        responseColors.add(String("#") + String(c1.r, HEX) + String(c1.g, HEX) + String(c1.b, HEX));
        if (colors.size() > 1) {
            responseColors.add(String("#") + String(c2.r, HEX) + String(c2.g, HEX) + String(c2.b, HEX));
        }
        if (colors.size() > 2) {
            responseColors.add(String("#") + String(c3.r, HEX) + String(c3.g, HEX) + String(c3.b, HEX));
        }

        String responseStr;ct found - only new format supported");
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Settings object required\"}");
        return;t (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_led_handlers.h"

#if ENABLE_LEDS

#include "api_handlers.h" // For sendErrorResponse
#include "web_server.h"   // For getRequestBody function
#include "../core/config.h"
#include "../core/led_config.h"
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

    // Get request body using the proper method
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        sendErrorResponse(request, 400, "Missing JSON body with effect configuration");
        return;
    }

    // Parse JSON body
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);

    if (error)
    {
        LOG_ERROR("API", "Failed to parse LED effect JSON: %s", error.c_str());
        sendErrorResponse(request, 400, "Invalid JSON in request body");
        return;
    }

    // Get effect name from JSON
    String effectName = doc["effect"] | "";
    if (effectName.length() == 0)
    {
        sendErrorResponse(request, 400, "No effect name provided in JSON");
        return;
    }

    LOG_VERBOSE("LEDS", "LED effect request: %s", effectName.c_str());
    LOG_VERBOSE("LEDS", "Full request body: %s", body.c_str());

    // Parse required parameters
    int cycles = doc["cycles"] | 1; // Default 1 cycle (0 = continuous)

    // Note: No duration concept - effects run for cycles or continuously

    // Parse colors array (required)
    if (!doc.containsKey("colors") || !doc["colors"].is<JsonArray>())
    {
        sendErrorResponse(request, 400, "Colors array is required");
        return;
    }

    JsonArray colorsArray = doc["colors"];
    if (colorsArray.size() == 0)
    {
        sendErrorResponse(request, 400, "Colors array cannot be empty");
        return;
    }

    // Create settings object from unified parameters
    DynamicJsonDocument settingsDoc(512);
    JsonObject settings = settingsDoc.to<JsonObject>();

    // Map unified parameters to effect-specific settings
    if (doc.containsKey("speed"))
        settings["speed"] = doc["speed"];
    if (doc.containsKey("intensity"))
    {
        // Map intensity to effect-specific parameter based on effect type
        int intensity = doc["intensity"];
        if (effectName.equalsIgnoreCase("twinkle") || effectName.equalsIgnoreCase("rainbow"))
        {
            settings["density"] = intensity;
        }
        else if (effectName.equalsIgnoreCase("chase_multi") || effectName.equalsIgnoreCase("chase_single"))
        {
            settings["trailLength"] = intensity / 8; // Map 0-255 to reasonable trail length
        }
    }

    // WLED-like unified interface: copy all frontend parameters to settings
    // This ensures consistent interface for all LED effects
    for (JsonPair kv : doc.as<JsonObject>())
    {
        const char *key = kv.key().c_str();
        // Skip core parameters that are handled separately
        if (strcmp(key, "effect") != 0 &&
            strcmp(key, "cycles") != 0 &&
            strcmp(key, "colors") != 0)
        {
            settings[key] = kv.value();
        }
    }

    // Add colors array to settings
    settings["colors"] = doc["colors"];

    // Parse colors from the colors array into CRGB values
    auto parseHexColor = [](const String &hexColor) -> CRGB
    {
        if (hexColor.length() < 7 || hexColor[0] != '#')
        {
            return CRGB::Black; // Invalid hex format
        }
        unsigned long colorValue = strtoul(hexColor.substring(1, 7).c_str(), NULL, 16);
        uint8_t r = (colorValue >> 16) & 0xFF;
        uint8_t g = (colorValue >> 8) & 0xFF;
        uint8_t b = colorValue & 0xFF;
        return CRGB(r, g, b);
    };

    // Parse colors into CRGB values
    CRGB c1 = CRGB::Blue, c2 = CRGB::Black, c3 = CRGB::Black;
    if (colorsArray.size() > 0)
        c1 = parseHexColor(colorsArray[0].as<String>());
    if (colorsArray.size() > 1)
        c2 = parseHexColor(colorsArray[1].as<String>());
    if (colorsArray.size() > 2)
        c3 = parseHexColor(colorsArray[2].as<String>());
    LOG_VERBOSE("LEDS", "Parsed %d colors from array", colorsArray.size());

    // Parse ALL settings based on effect type and apply them

    // Parse ALL settings based on effect type and apply them
    // Create a temporary effects configuration with settings
    // This will be used for the playground without saving to permanent config
#ifdef ENABLE_LEDS
    {
        LedEffectsConfig playgroundConfig = {}; // Start with empty config

        if (effectName.equalsIgnoreCase("chase_single"))
        {
            playgroundConfig.chaseSingle.speed = settings["speed"] | 5;
            playgroundConfig.chaseSingle.trailLength = settings["trailLength"] | 15;
            playgroundConfig.chaseSingle.trailFade = settings["trailFade"] | 15;
            playgroundConfig.chaseSingle.defaultColor = settings["color"] | "#0062ffff";
        }
        else if (effectName.equalsIgnoreCase("chase_multi"))
        {
            playgroundConfig.chaseMulti.speed = settings["speed"] | 2;
            playgroundConfig.chaseMulti.trailLength = settings["trailLength"] | 20;
            playgroundConfig.chaseMulti.trailFade = settings["trailFade"] | 20;
            playgroundConfig.chaseMulti.colorSpacing = settings["colorSpacing"] | 12;
            if (settings.containsKey("colors") && settings["colors"].as<JsonArray>().size() > 0)
            {
                JsonArray settingsColors = settings["colors"];
                playgroundConfig.chaseMulti.color1 = settingsColors.size() > 0 ? settingsColors[0].as<String>() : "#ff9900ff";
                playgroundConfig.chaseMulti.color2 = settingsColors.size() > 1 ? settingsColors[1].as<String>() : "#008f00ff";
                playgroundConfig.chaseMulti.color3 = settingsColors.size() > 2 ? settingsColors[2].as<String>() : "#78cffeff";
            }
        }
        else if (effectName.equalsIgnoreCase("matrix"))
        {
            playgroundConfig.matrix.speed = settings["speed"] | 3;
            playgroundConfig.matrix.drops = settings["drops"] | 5;
            playgroundConfig.matrix.backgroundFade = settings["backgroundFade"] | 64;
            playgroundConfig.matrix.trailFade = settings["trailFade"] | 32;
            playgroundConfig.matrix.brightnessFade = settings["brightnessFade"] | 40;
            playgroundConfig.matrix.defaultColor = settings["color"] | "#009100ff";
        }
        else if (effectName.equalsIgnoreCase("twinkle"))
        {
            playgroundConfig.twinkle.density = settings["density"] | 8;
            playgroundConfig.twinkle.fadeSpeed = settings["fadeSpeed"] | 5;
            playgroundConfig.twinkle.minBrightness = settings["minBrightness"] | 50;
            playgroundConfig.twinkle.maxBrightness = settings["maxBrightness"] | 255;
            playgroundConfig.twinkle.defaultColor = settings["color"] | "#ffffffff";
        }
        else if (effectName.equalsIgnoreCase("pulse"))
        {
            playgroundConfig.pulse.speed = settings["speed"] | 4;
            playgroundConfig.pulse.minBrightness = settings["minBrightness"] | 0;
            playgroundConfig.pulse.maxBrightness = settings["maxBrightness"] | 255;
            playgroundConfig.pulse.waveFrequency = settings["waveFrequency"] | 0.05f;
            playgroundConfig.pulse.defaultColor = settings["color"] | "#ff00f2ff";
        }
        else if (effectName.equalsIgnoreCase("rainbow"))
        {
            playgroundConfig.rainbow.speed = settings["speed"] | 3.0f;
            playgroundConfig.rainbow.saturation = settings["saturation"] | 255;
            playgroundConfig.rainbow.brightness = settings["brightness"] | 255;
            playgroundConfig.rainbow.hueStep = settings["hueStep"] | 2.0f;
        }

        // Apply the playground configuration temporarily
        ledEffects.updateEffectConfig(playgroundConfig);
    }
#endif

    LOG_VERBOSE("LEDS", "Applied settings for LED effect: %s", effectName.c_str());
    LOG_VERBOSE("LEDS", "Parsed %d colors from settings", (int)colorsArray.size());

    // Unified cycle-based system: all effects run for cycles (0 = continuous)
    bool success;
    if (cycles > 0)
    {
        // Run effect for specific number of cycles
        success = ledEffects.startEffectCycles(effectName, cycles, c1, c2, c3);
        LOG_VERBOSE("LEDS", "Started LED effect: %s for %d cycles", effectName.c_str(), cycles);
    }
    else
    {
        // Run effect continuously (cycles=0 means indefinite)
        success = ledEffects.startEffectCycles(effectName, 0, c1, c2, c3);
        LOG_VERBOSE("LEDS", "Started LED effect: %s continuously", effectName.c_str());
    }

    if (success)
    {
        DynamicJsonDocument response(1024); // Increased size for full settings object
        response["success"] = true;
        response["message"] = "LED effect started";
        response["effect"] = effectName;
        response["cycles"] = cycles;

        // Include the original settings object in the response
        if (doc.containsKey("settings"))
        {
            response["settings"] = doc["settings"];
        }

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
