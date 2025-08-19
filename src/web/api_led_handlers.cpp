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
    
    // Map frontend effect names to backend effect names for compatibility
    String backendEffectName = effectName;
    if (effectName.equalsIgnoreCase("simple_chase")) {
        backendEffectName = "chase_single";
    } else if (effectName.equalsIgnoreCase("chase")) {
        backendEffectName = "chase_multi";  
    }
    // Other effects (rainbow, twinkle, pulse, matrix) use same names

    // Parse optional parameters from JSON body
    int duration = doc["duration"] | 5; // Default 5 seconds (for duration-based effects)
    int cycles = doc["cycles"] | 1;     // Default 1 cycle (for cycle-based effects)

    // For duration-based effects from web UI, convert milliseconds to seconds
    if (duration > 100)
    { // Likely milliseconds from web UI
        duration = duration / 1000;
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

    // Function to parse hex color strings (#RRGGBB or #RRGGBBAA)
    auto parseHexColor = [](const String &hexColor) -> CRGB
    {
        if (hexColor.length() < 7 || hexColor[0] != '#')
        {
            return CRGB::Black; // Invalid hex format
        }

        // Parse RGB values from hex string
        unsigned long colorValue = strtoul(hexColor.substring(1, 7).c_str(), NULL, 16);
        uint8_t r = (colorValue >> 16) & 0xFF;
        uint8_t g = (colorValue >> 8) & 0xFF;
        uint8_t b = colorValue & 0xFF;

        return CRGB(r, g, b);
    };

    // Parse colors - handle both old settings object format and new unified format
    CRGB c1 = CRGB::Blue, c2 = CRGB::Black, c3 = CRGB::Black;
    std::vector<CRGB> colors;
    
    JsonObject settings;
    bool hasSettingsObject = doc.containsKey("settings") && doc["settings"].is<JsonObject>();
    
    if (hasSettingsObject) {
        // Old format with settings object
        settings = doc["settings"];
        LOG_VERBOSE("LEDS", "Processing old settings object format");
    } else {
        // New unified format - create settings object from root level parameters
        LOG_VERBOSE("LEDS", "Processing new unified parameter format");
        DynamicJsonDocument settingsDoc(256);
        JsonObject newSettings = settingsDoc.to<JsonObject>();
        
        // Map unified parameters to effect-specific settings
        if (doc.containsKey("speed")) newSettings["speed"] = doc["speed"];
        if (doc.containsKey("intensity")) {
            // Map intensity to effect-specific parameter based on effect type
            int intensity = doc["intensity"];
            if (effectName.equalsIgnoreCase("twinkle") || effectName.equalsIgnoreCase("rainbow")) {
                newSettings["density"] = intensity;
            } else if (effectName.equalsIgnoreCase("chase") || effectName.equalsIgnoreCase("chase_single")) {
                newSettings["trailLength"] = intensity / 8; // Map 0-255 to reasonable trail length
            }
            // For other effects, intensity might be unused or mapped differently
        }
        if (doc.containsKey("color")) newSettings["color"] = doc["color"];
        if (doc.containsKey("palette")) newSettings["palette"] = doc["palette"];
        
        settings = newSettings;
    }

    if (settings.isNull()) {
        LOG_ERROR("LEDS", "No settings found - neither old settings object nor new unified parameters");
        sendErrorResponse(request, 400, "Settings object or unified parameters required");
        return;
    }

    // Check for hex color array or single color in settings
    if (settings.containsKey("colors") && settings["colors"].is<JsonArray>())
    {
        JsonArray colorArray = settings["colors"];
        for (JsonVariant colorVar : colorArray)
        {
            if (colorVar.is<const char *>())
            {
                String colorStr = colorVar.as<String>();
                CRGB color = parseHexColor(colorStr);
                colors.push_back(color);
                LOG_VERBOSE("LEDS", "Parsed hex color: %s -> RGB(%d,%d,%d)",
                            colorStr.c_str(), color.r, color.g, color.b);
            }
        }
    }
    else if (settings.containsKey("color"))
    {
        // Single color for effects like pulse, twinkle, matrix
        String colorStr = settings["color"].as<String>();
        CRGB color = parseHexColor(colorStr);
        colors.push_back(color);
        LOG_VERBOSE("LEDS", "Parsed single color: %s -> RGB(%d,%d,%d)",
                    colorStr.c_str(), color.r, color.g, color.b);
    }

    // Set primary colors
    c1 = colors.size() > 0 ? colors[0] : CRGB::Blue;
    c2 = colors.size() > 1 ? colors[1] : CRGB::Black;
    c3 = colors.size() > 2 ? colors[2] : CRGB::Black;

// Parse ALL settings based on effect type and apply them
// Create a temporary effects configuration with frontend settings
// This will be used for the playground without saving to permanent config
#ifdef ENABLE_LEDS
        LedEffectsConfig playgroundConfig = {}; // Start with empty config

        if (backendEffectName.equalsIgnoreCase("chase_single"))
        {
            playgroundConfig.chaseSingle.speed = settings["speed"] | 5;
            playgroundConfig.chaseSingle.trailLength = settings["trailLength"] | 15;
            playgroundConfig.chaseSingle.trailFade = settings["trailFade"] | 15;
            playgroundConfig.chaseSingle.defaultColor = settings["color"] | "#0062ffff";
        }
        else if (backendEffectName.equalsIgnoreCase("chase_multi"))
        {
            playgroundConfig.chaseMulti.speed = settings["speed"] | 2;
            playgroundConfig.chaseMulti.trailLength = settings["trailLength"] | 20;
            playgroundConfig.chaseMulti.trailFade = settings["trailFade"] | 20;
            playgroundConfig.chaseMulti.colorSpacing = settings["colorSpacing"] | 12;
            if (settings.containsKey("colors") && settings["colors"].is<JsonArray>())
            {
                JsonArray colorsArray = settings["colors"];
                playgroundConfig.chaseMulti.color1 = colorsArray.size() > 0 ? colorsArray[0].as<String>() : "#ff9900ff";
                playgroundConfig.chaseMulti.color2 = colorsArray.size() > 1 ? colorsArray[1].as<String>() : "#008f00ff";
                playgroundConfig.chaseMulti.color3 = colorsArray.size() > 2 ? colorsArray[2].as<String>() : "#78cffeff";
            }
        }
        else if (backendEffectName.equalsIgnoreCase("matrix"))
        {
            playgroundConfig.matrix.speed = settings["speed"] | 3;
            playgroundConfig.matrix.drops = settings["drops"] | 5;
            playgroundConfig.matrix.backgroundFade = settings["backgroundFade"] | 64;
            playgroundConfig.matrix.trailFade = settings["trailFade"] | 32;
            playgroundConfig.matrix.brightnessFade = settings["brightnessFade"] | 40;
            playgroundConfig.matrix.defaultColor = settings["color"] | "#009100ff";
        }
        else if (backendEffectName.equalsIgnoreCase("twinkle"))
        {
            playgroundConfig.twinkle.density = settings["density"] | 8;
            playgroundConfig.twinkle.fadeSpeed = settings["fadeSpeed"] | 5;
            playgroundConfig.twinkle.minBrightness = settings["minBrightness"] | 50;
            playgroundConfig.twinkle.maxBrightness = settings["maxBrightness"] | 255;
            playgroundConfig.twinkle.defaultColor = settings["color"] | "#ffffffff";
        }
        else if (backendEffectName.equalsIgnoreCase("pulse"))
        {
            playgroundConfig.pulse.speed = settings["speed"] | 4;
            playgroundConfig.pulse.minBrightness = settings["minBrightness"] | 0;
            playgroundConfig.pulse.maxBrightness = settings["maxBrightness"] | 255;
            playgroundConfig.pulse.waveFrequency = settings["waveFrequency"] | 0.05f;
            playgroundConfig.pulse.defaultColor = settings["color"] | "#ff00f2ff";
        }
        else if (backendEffectName.equalsIgnoreCase("rainbow"))
        {
            playgroundConfig.rainbow.speed = settings["speed"] | 3.0f;
            playgroundConfig.rainbow.saturation = settings["saturation"] | 255;
            playgroundConfig.rainbow.brightness = settings["brightness"] | 255;
            playgroundConfig.rainbow.hueStep = settings["hueStep"] | 2.0f;
        }

        // Apply the playground configuration temporarily
        ledEffects.updateEffectConfig(playgroundConfig);
#endif

        LOG_VERBOSE("LEDS", "Applied all frontend settings for effect: %s (backend: %s)", effectName.c_str(), backendEffectName.c_str());
        LOG_VERBOSE("LEDS", "Using colors from frontend settings: %d colors parsed", (int)colors.size());
    }
    else
    {
        LOG_ERROR("LEDS", "No settings object found - only new format supported");
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Settings object required\"}");
        return;
    }

    // Determine if this effect should be cycle-based or duration-based
    bool useCycles = backendEffectName.equalsIgnoreCase("chase_single") || backendEffectName.equalsIgnoreCase("chase_multi");
    bool success;

    if (useCycles)
    {
        // Cycle-based effects (chase_single, chase_multi)
        success = ledEffects.startEffectCycles(backendEffectName, cycles, c1, c2, c3);
        LOG_VERBOSE("LEDS", "Started LED effect: %s for %d cycles", backendEffectName.c_str(), cycles);
    }
    else
    {
        // Duration-based effects (rainbow, twinkle, pulse, matrix)
        success = ledEffects.startEffectDuration(backendEffectName, duration, c1, c2, c3);
        LOG_VERBOSE("LEDS", "Started LED effect: %s for %d seconds", backendEffectName.c_str(), duration);
    }

    if (success)
    {
        DynamicJsonDocument response(1024); // Increased size for full settings object
        response["success"] = true;
        response["message"] = "LED effect started";
        response["effect"] = effectName;
        if (useCycles)
        {
            response["cycles"] = cycles;
        }
        else
        {
            response["duration"] = duration;
        }

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
