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

        LOG_ERROR("LEDS", "No settings object found - only new format supported");
        sendErrorResponse(request, 400, "Settings object required");
        return;
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_led_handlers.h"

#if ENABLE_LEDS

#include "api_handlers.h" // For sendErrorResponse
#include "web_server.h"   // For getRequestBody function
#include "../core/config.h"
#include "../core/config_loader.h"
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
    int cycles = 1; // Default value
    if (doc.containsKey("cycles"))
    {
        if (doc["cycles"].is<int>())
        {
            cycles = doc["cycles"].as<int>();
        }
        else if (doc["cycles"].is<const char *>() || doc["cycles"].is<String>())
        {
            // Handle string values from frontend
            String cyclesStr = doc["cycles"].as<String>();
            cycles = cyclesStr.toInt();
            if (cycles <= 0)
                cycles = 1; // Ensure valid value
        }
    }

    // Note: No duration concept - effects run for cycles or continuously

    // Colors: optional; effects will use defaults when not supplied.
    const bool hasColors = doc.containsKey("colors") && doc["colors"].is<JsonArray>();

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

    // Add colors array to settings if provided
    if (hasColors)
    {
        settings["colors"] = doc["colors"];
    }

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

    // Parse colors into CRGB values (effect-specific rules, defaults when needed)
    CRGB c1 = CRGB::Blue, c2 = CRGB::Black, c3 = CRGB::Black;
    bool colorsAdjusted = false;

    auto setDefaultColorsForEffect = [&](const String &name) {
        if (name.equalsIgnoreCase("chase_single")) { c1 = CRGB(0x00, 0x62, 0xff); }
        else if (name.equalsIgnoreCase("chase_multi")) { c1 = CRGB(0xff, 0x00, 0x00); c2 = CRGB(0x00, 0xff, 0x00); c3 = CRGB(0x00, 0x00, 0xff); }
        else if (name.equalsIgnoreCase("matrix")) { c1 = CRGB(0x00, 0xff, 0x00); }
        else if (name.equalsIgnoreCase("twinkle")) { c1 = CRGB(0xff, 0xff, 0x00); }
        else if (name.equalsIgnoreCase("pulse")) { c1 = CRGB(0x80, 0x00, 0x80); }
        else /* rainbow or unknown */ { /* rainbow ignores explicit colors */ }
    };

    if (effectName.equalsIgnoreCase("rainbow"))
    {
        // Rainbow ignores colors; use internal color generation
        LOG_VERBOSE("LEDS", "Rainbow effect: ignoring explicit colors");
    }
    else if (effectName.equalsIgnoreCase("chase_multi"))
    {
        // Expect 3 colors; adjust if missing/excess
        if (hasColors)
        {
            JsonArray colorsArray = doc["colors"].as<JsonArray>();
            int n = colorsArray.size();
            if (n >= 1) c1 = parseHexColor(colorsArray[0].as<String>());
            if (n >= 2) c2 = parseHexColor(colorsArray[1].as<String>());
            if (n >= 3) c3 = parseHexColor(colorsArray[2].as<String>());
            if (n != 3)
            {
                // Fill missing with defaults
                CRGB d1 = CRGB(0xff,0x00,0x00), d2 = CRGB(0x00,0xff,0x00), d3 = CRGB(0x00,0x00,0xff);
                if (n < 1) c1 = d1;
                if (n < 2) c2 = d2;
                if (n < 3) c3 = d3;
                colorsAdjusted = true;
            }
            if (n > 3) colorsAdjusted = true; // extras ignored
        }
        else
        {
            setDefaultColorsForEffect(effectName);
            colorsAdjusted = true;
        }
    }
    else
    {
        // Single-color effects: chase_single, matrix, twinkle, pulse
        if (hasColors)
        {
            JsonArray colorsArray = doc["colors"].as<JsonArray>();
            if (colorsArray.size() >= 1)
            {
                c1 = parseHexColor(colorsArray[0].as<String>());
                if (colorsArray.size() > 1) colorsAdjusted = true; // extras ignored
            }
            else
            {
                setDefaultColorsForEffect(effectName);
                colorsAdjusted = true;
            }
        }
        else
        {
            setDefaultColorsForEffect(effectName);
            colorsAdjusted = true;
        }
    }

    // Parse ALL settings based on effect type and apply them

    // Parse ALL settings based on effect type and apply them
    // Create a temporary effects configuration with settings
    // This will be used for the playground without saving to permanent config
#ifdef ENABLE_LEDS
    {
        LedEffectsConfig playgroundConfig = {}; // Start with empty config

        // Get LED configuration for calculations
        const RuntimeConfig &config = getRuntimeConfig();
        int ledCount = config.ledCount;
        
        // Map 10-100 speed/intensity to reasonable effect parameters (50 = ideal)
        int speed = settings["speed"] | 50;        // Default to 50 if missing
        int intensity = settings["intensity"] | 50; // Default to 50 if missing
        
        // Clamp to expected range
        speed = max(10, min(100, speed));
        intensity = max(10, min(100, intensity));
        
        if (effectName.equalsIgnoreCase("chase_single"))
        {
            // IMPROVED SPEED MAPPING: Much more dramatic differences between speed values
            // Speed 10 = 50 frames (very slow, ~1.7 seconds per pixel at 30fps)
            // Speed 50 = 6 frames (moderately fast) 
            // Speed 100 = 1 frame (very fast)
            // Uses inverse exponential for dramatic visual differences
            int frameDelay;
            if (speed <= 10) frameDelay = 50;
            else if (speed >= 100) frameDelay = 1; 
            else {
                // Exponential decay: speed 10->50 frames, 50->6 frames, 100->1 frame
                float t = (speed - 10) / 90.0f; // 0 to 1
                frameDelay = (int)(50 * pow(0.02f, t)); // Exponential decay from 50 to 1
            }
            playgroundConfig.chaseSingle.speed = max(1, frameDelay);
            
            // Intensity: 10-100 -> trail length (50 = 15 pixels, good balance)
            playgroundConfig.chaseSingle.trailLength = max(3, min(30, (int)(intensity * 0.3)));
            playgroundConfig.chaseSingle.trailFade = 15; // Fixed fade amount
            playgroundConfig.chaseSingle.defaultColor = "#0062ff";
        }
        else if (effectName.equalsIgnoreCase("chase_multi"))
        {
            // IMPROVED SPEED MAPPING: Dramatic differences for chase_multi
            // Speed 10 = 40 frames (very slow), Speed 50 = 5 frames (moderate), Speed 100 = 1 frame (very fast)
            int frameDelay;
            if (speed <= 10) frameDelay = 40;
            else if (speed >= 100) frameDelay = 1; 
            else {
                float t = (speed - 10) / 90.0f; // 0 to 1
                frameDelay = (int)(40 * pow(0.025f, t)); // Exponential decay from 40 to 1
            }
            playgroundConfig.chaseMulti.speed = max(1, frameDelay);
            
            // Intensity: 10-100 -> trail length (50 = 15 pixels) 
            playgroundConfig.chaseMulti.trailLength = max(3, min(25, (int)(intensity * 0.3)));
            playgroundConfig.chaseMulti.trailFade = 20; // Fixed fade amount
            
            // Color spacing based on LED count for optimal visual effect
            playgroundConfig.chaseMulti.colorSpacing = max(2, ledCount / 10); // Auto-scale to strip length
            
            // Colors from frontend
            if (settings.containsKey("colors") && settings["colors"].as<JsonArray>().size() > 0)
            {
                JsonArray settingsColors = settings["colors"];
                playgroundConfig.chaseMulti.color1 = settingsColors.size() > 0 ? settingsColors[0].as<String>() : "#ff0000";
                playgroundConfig.chaseMulti.color2 = settingsColors.size() > 1 ? settingsColors[1].as<String>() : "#00ff00";
                playgroundConfig.chaseMulti.color3 = settingsColors.size() > 2 ? settingsColors[2].as<String>() : "#0000ff";
            }
        }
        else if (effectName.equalsIgnoreCase("matrix"))
        {
            // IMPROVED SPEED MAPPING: Dramatic differences for matrix effect
            // Speed 10 = 30 frames (very slow drops), Speed 50 = 4 frames (moderate), Speed 100 = 1 frame (very fast)
            int frameDelay;
            if (speed <= 10) frameDelay = 30;
            else if (speed >= 100) frameDelay = 1; 
            else {
                float t = (speed - 10) / 90.0f; // 0 to 1
                frameDelay = (int)(30 * pow(0.033f, t)); // Exponential decay from 30 to 1
            }
            playgroundConfig.matrix.speed = max(1, frameDelay);
            
            // Intensity: 10-100 -> number of drops (50 = good density, scales with LED count)
            // For 30 LEDs: 50 intensity = ~6 drops, 10 = 1-2 drops, 100 = 10-12 drops
            int baseDensity = max(1, ledCount / 15); // Scale to strip length
            playgroundConfig.matrix.drops = max(1, (int)(baseDensity * intensity / 50.0f));
            
            // Fixed internal parameters for smooth matrix effect
            playgroundConfig.matrix.backgroundFade = 64;
            playgroundConfig.matrix.trailFade = 32;
            playgroundConfig.matrix.brightnessFade = 40;
            playgroundConfig.matrix.defaultColor = "#00ff00";
        }
        else if (effectName.equalsIgnoreCase("twinkle"))
        {
            // IMPROVED SPEED MAPPING: Dramatic differences for twinkle effect
            // Speed 10 = 25 frames (very slow twinkle), Speed 50 = 3 frames (moderate), Speed 100 = 1 frame (very fast)
            int twinkleRate;
            if (speed <= 10) twinkleRate = 25;
            else if (speed >= 100) twinkleRate = 1; 
            else {
                float t = (speed - 10) / 90.0f; // 0 to 1
                twinkleRate = (int)(25 * pow(0.04f, t)); // Exponential decay from 25 to 1
            }
            twinkleRate = max(1, twinkleRate);
            
            // Intensity: 1-100 -> number of active twinkles (50->10 twinkles ideal)
            int numTwinkles = max(1, (int)(intensity * 0.2)); // 50->10, 25->5, 100->20
            
            playgroundConfig.twinkle.density = numTwinkles;
            playgroundConfig.twinkle.fadeSpeed = max(1, twinkleRate); // Use speed for fade too
            playgroundConfig.twinkle.minBrightness = 50;  // Fixed
            playgroundConfig.twinkle.maxBrightness = 255; // Fixed 
            playgroundConfig.twinkle.defaultColor = "#ffff00";
        }
        else if (effectName.equalsIgnoreCase("pulse"))
        {
            // IMPROVED SPEED MAPPING: Dramatic differences for pulse effect
            // Speed 10 = 20 frames (very slow pulse), Speed 50 = 3 frames (moderate), Speed 100 = 1 frame (very fast)
            int pulseSpeed;
            if (speed <= 10) pulseSpeed = 20;
            else if (speed >= 100) pulseSpeed = 1; 
            else {
                float t = (speed - 10) / 90.0f; // 0 to 1
                pulseSpeed = (int)(20 * pow(0.05f, t)); // Exponential decay from 20 to 1
            }
            playgroundConfig.pulse.speed = max(1, pulseSpeed);
            
            // Intensity: 1-100 -> brightness variation (50 = moderate variation)
            // Map intensity to min brightness: 50->64 (moderate), 25->128 (subtle), 100->0 (full range)  
            int minBrightness = max(0, 255 - (int)(intensity * 2.55));
            playgroundConfig.pulse.minBrightness = minBrightness;
            playgroundConfig.pulse.maxBrightness = 255; // Always full bright at peak
            playgroundConfig.pulse.waveFrequency = 0.05f; // Fixed
            playgroundConfig.pulse.defaultColor = "#800080";
        }
        else if (effectName.equalsIgnoreCase("rainbow"))
        {
            // IMPROVED SPEED MAPPING: Dramatic differences for rainbow effect
            // Speed 10 = 0.5 (very slow movement), Speed 50 = 4.0 (moderate), Speed 100 = 15.0 (very fast)
            float rainbowSpeed;
            if (speed <= 10) rainbowSpeed = 0.5f;
            else if (speed >= 100) rainbowSpeed = 15.0f; 
            else {
                float t = (speed - 10) / 90.0f; // 0 to 1
                rainbowSpeed = 0.5f + (t * t * 14.5f); // Quadratic growth from 0.5 to 15.0
            }
            playgroundConfig.rainbow.speed = rainbowSpeed;
            
            // Intensity: 1-100 -> wave length/density (50->2.5 hue step ideal)
            // Lower intensity = longer waves (higher hue step), higher intensity = shorter waves
            float hueStep = max(0.5f, (float)(6.0 - (intensity * 0.08))); // 50->2.0, 25->4.0, 100->1.2
            playgroundConfig.rainbow.hueStep = hueStep;
            
            // Fixed parameters
            playgroundConfig.rainbow.saturation = 255;
            playgroundConfig.rainbow.brightness = 255;
        }

        // Apply the playground configuration temporarily
        ledEffects.updateEffectConfig(playgroundConfig);
    }
#endif

    LOG_VERBOSE("LEDS", "Applied settings for LED effect: %s", effectName.c_str());
    int colorsCount = 0;
    if (hasColors)
    {
        JsonArray ca = doc["colors"].as<JsonArray>();
        colorsCount = ca.size();
    }
    LOG_VERBOSE("LEDS", "Parsed %d colors from settings", colorsCount);

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
        response["message"] = "LED effect started";
        response["effect"] = effectName;
        response["cycles"] = cycles;

        if (colorsAdjusted)
        {
            response["note"] = "Colors adjusted for effect; defaults applied where necessary";
        }

        // Include the resolved colors used
        JsonArray used = response.createNestedArray("colors_used");
        used.add(String("#") + String(c1.r, HEX) + String(c1.g, HEX) + String(c1.b, HEX));
        used.add(String("#") + String(c2.r, HEX) + String(c2.g, HEX) + String(c2.b, HEX));
        used.add(String("#") + String(c3.r, HEX) + String(c3.g, HEX) + String(c3.b, HEX));

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
