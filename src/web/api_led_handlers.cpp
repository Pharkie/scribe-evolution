/**
 * @file api_led_handlers.cpp
 * @brief LED API endpoint handlers for FastLED effects
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_led_handlers.h"

#if ENABLE_LEDS

#include "api_handlers.h" // For sendErrorResponse
#include "web_server.h"   // For getRequestBody function
#include <config/config.h>
#include <core/config_loader.h>
#include <core/led_config.h>
#include <core/logging.h>
#include <leds/LedEffects.h>
#include <ArduinoJson.h>
#include <FastLED.h>

// External LED effects instance

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
    JsonDocument doc;
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
    int cycles = DEFAULT_LED_EFFECT_CYCLES; // Default cycles from led_config.h
    if (doc["cycles"].is<JsonVariant>())
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
                cycles = DEFAULT_LED_EFFECT_CYCLES; // Ensure valid value from defaults
        }
    }

    // Note: No duration concept - effects run for cycles or continuously

    // Colors: optional; effects will use defaults when not supplied.
    const bool hasColors = doc["colors"].is<JsonVariant>() && doc["colors"].is<JsonArray>();

    // Create settings object from unified parameters
    JsonDocument settingsDoc;
    JsonObject settings = settingsDoc.to<JsonObject>();

    // Map unified parameters to effect-specific settings
    if (doc["speed"].is<JsonVariant>())
        settings["speed"] = doc["speed"];
    if (doc["intensity"].is<JsonVariant>())
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

    auto setDefaultColorsForEffect = [&](const String &name)
    {
        if (name.equalsIgnoreCase("chase_single"))
        {
            c1 = CRGB(0x00, 0x62, 0xff);
        }
        else if (name.equalsIgnoreCase("chase_multi"))
        {
            c1 = CRGB(0xff, 0x00, 0x00);
            c2 = CRGB(0x00, 0xff, 0x00);
            c3 = CRGB(0x00, 0x00, 0xff);
        }
        else if (name.equalsIgnoreCase("matrix"))
        {
            c1 = CRGB(0x00, 0xff, 0x00);
        }
        else if (name.equalsIgnoreCase("twinkle"))
        {
            c1 = CRGB(0xff, 0xff, 0x00);
        }
        else if (name.equalsIgnoreCase("pulse"))
        {
            c1 = CRGB(0x80, 0x00, 0x80);
        }
        else /* rainbow or unknown */
        {    /* rainbow ignores explicit colors */
        }
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
            if (n >= 1)
                c1 = parseHexColor(colorsArray[0].as<String>());
            if (n >= 2)
                c2 = parseHexColor(colorsArray[1].as<String>());
            if (n >= 3)
                c3 = parseHexColor(colorsArray[2].as<String>());
            if (n != 3)
            {
                // Fill missing with defaults
                CRGB d1 = CRGB(0xff, 0x00, 0x00), d2 = CRGB(0x00, 0xff, 0x00), d3 = CRGB(0x00, 0x00, 0xff);
                if (n < 1)
                    c1 = d1;
                if (n < 2)
                    c2 = d2;
                if (n < 3)
                    c3 = d3;
                colorsAdjusted = true;
            }
            if (n > 3)
                colorsAdjusted = true; // extras ignored
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
                if (colorsArray.size() > 1)
                    colorsAdjusted = true; // extras ignored
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
    // Create a temporary effects configuration with settings
    // This will be used for the playground without saving to permanent config
#ifdef ENABLE_LEDS
    {
        // Start from current runtime defaults so missing parameters keep defaults from led_config.h
        LedEffectsConfig playgroundConfig = getRuntimeConfig().ledEffects;

        // Get LED configuration for calculations
        const RuntimeConfig &config = getRuntimeConfig();
        int ledCount = config.ledCount;

        // Map 1–100 speed/intensity to reasonable effect parameters (50 = ideal)
        bool speedProvided = doc["speed"].is<JsonVariant>();
        bool intensityProvided = doc["intensity"].is<JsonVariant>();
        int speed = speedProvided ? (int)doc["speed"] : 50;         // Only used if provided
        int intensity = intensityProvided ? (int)doc["intensity"] : 50; // Only used if provided
        // Clamp to expected range (1..100)
        speed = max(1, min(100, speed));
        intensity = max(10, min(100, intensity));

        // Helper to map slider (1..100) to frame delay (smaller delay = faster)
        auto mapFrameDelayExp = [](int s, int minDelay, int maxDelay, float expK = 2.0f)
        {
            // s=1 (slow) -> maxDelay, s=100 (fast) -> minDelay
            float t = (float)(s - 1) / 99.0f; // 0..1
            int d = (int)(maxDelay - (maxDelay - minDelay) * powf(t, expK));
            return max(minDelay, min(maxDelay, d));
        };
        // Helper to map slider (1..100) to a direct range where higher = larger value
        auto mapRangeExp = [](int s, int minVal, int maxVal, float expK = 2.0f)
        {
            // s=1 -> minVal, s=100 -> maxVal
            float t = (float)(s - 1) / 99.0f; // 0..1
            int v = (int)(minVal + (maxVal - minVal) * powf(t, expK));
            return max(minVal, min(maxVal, v));
        };

        if (effectName.equalsIgnoreCase("chase_single"))
        {
            // Map slider to steps-per-frame (x100 fixed-point) for smooth speed
            // 1 => 40% slower than previous minimum (0.30 spf), 100 keeps current max (1.20 spf)
            auto mapStepsPerFrameX100 = [&](int s)
            {
                float t = (float)(s - 1) / 99.0f; // 0..1
                float minStep = 0.30f;            // ~40% slower than previous 0.50
                float maxStep = 1.20f;            // keep current max
                float spf = minStep + (maxStep - minStep) * powf(t, 1.7f);
                return (int)(spf * 100.0f + 0.5f);
            };

            if (speedProvided)
                playgroundConfig.chaseSingle.speed = mapStepsPerFrameX100(speed);

            // Intensity 1..100 -> trail length 2..20 (linear)
            if (intensityProvided)
            {
                int trailLen = (int)(2 + ((intensity - 1) * 18.0f / 99.0f) + 0.5f);
                playgroundConfig.chaseSingle.trailLength = max(2, min(20, trailLen));
            }
            playgroundConfig.chaseSingle.trailFade = 15; // Fixed fade amount
            playgroundConfig.chaseSingle.defaultColor = "#0062ff";
        }
        else if (effectName.equalsIgnoreCase("chase_multi"))
        {
            // Same steps-per-frame mapping as chase_single
            auto mapStepsPerFrameX100 = [&](int s)
            {
                float t = (float)(s - 1) / 99.0f; // 0..1
                float minStep = 0.30f;
                float maxStep = 1.20f;
                float spf = minStep + (maxStep - minStep) * powf(t, 1.7f);
                return (int)(spf * 100.0f + 0.5f);
            };
            if (speedProvided)
                playgroundConfig.chaseMulti.speed = mapStepsPerFrameX100(speed);

            // Intensity 1..100 -> trail length 2..20 (linear)
            if (intensityProvided)
            {
                int trailLen = (int)(2 + ((intensity - 1) * 18.0f / 99.0f) + 0.5f);
                playgroundConfig.chaseMulti.trailLength = max(2, min(20, trailLen));
            }
            playgroundConfig.chaseMulti.trailFade = 20; // Fixed fade amount

            // Color spacing based on LED count for optimal visual effect
            playgroundConfig.chaseMulti.colorSpacing = max(2, ledCount / 10); // Auto-scale to strip length

            // Colors from frontend
            if (settings["colors"].is<JsonVariant>() && settings["colors"].as<JsonArray>().size() > 0)
            {
                JsonArray settingsColors = settings["colors"];
                playgroundConfig.chaseMulti.color1 = settingsColors.size() > 0 ? settingsColors[0].as<String>() : "#ff0000";
                playgroundConfig.chaseMulti.color2 = settingsColors.size() > 1 ? settingsColors[1].as<String>() : "#00ff00";
                playgroundConfig.chaseMulti.color3 = settingsColors.size() > 2 ? settingsColors[2].as<String>() : "#0000ff";
            }
        }
        else if (effectName.equalsIgnoreCase("matrix"))
        {
            // Speed controls droplet spawn target per cycle: 1->3, 100->10
            if (speedProvided)
            {
                int targetDrops = (int)roundf(3.0f + (float)(speed - 1) * (10.0f - 3.0f) / 99.0f);
                playgroundConfig.matrix.drops = max(1, targetDrops);
            }

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
            // Higher slider -> faster twinkling; allow min fade step = 1 for smoother low-end
            int fadeStep = mapRangeExp(speed, /*min*/ 1, /*max*/ 64, /*exp*/ 1.6f);
            
            // Intensity: 1-100 -> number of active twinkles (50->10 twinkles ideal)
            int numTwinkles = max(1, (int)(intensity * 0.2)); // 50->10, 25->5, 100->20

            playgroundConfig.twinkle.density = numTwinkles;
            playgroundConfig.twinkle.fadeSpeed = fadeStep; // Larger = faster fade per frame
            playgroundConfig.twinkle.maxBrightness = 255;  // Fixed
            playgroundConfig.twinkle.defaultColor = "#ffff00";
        }
        else if (effectName.equalsIgnoreCase("pulse"))
        {
            // Higher = faster; compress slow end so 1 isn't glacial
            // Speed: 1-100 maps to time-based cycle time (100→1sec, 1→5sec)
            if (speedProvided)
            {
                playgroundConfig.pulse.speed = speed; // Direct mapping (no frame delay conversion)
            }

            // Pulse always goes OFF→MAX→OFF (intensity parameter unused)
            playgroundConfig.pulse.maxBrightness = 255;    // Always peak at full brightness
            playgroundConfig.pulse.waveFrequency = 0.05f;  // Fixed
            playgroundConfig.pulse.defaultColor = "#800080";
        }
        else if (effectName.equalsIgnoreCase("rainbow"))
        {
            // Rainbow: higher slider -> faster movement; map 1..100 to 0.5..15.0
            float t = (float)(speed - 1) / 99.0f; // 0..1
            float minSpeed = 0.5f, maxSpeed = 15.0f;
            // Quadratic growth toward maxSpeed as slider increases
            float rainbowSpeed = minSpeed + (maxSpeed - minSpeed) * (t * t);
            if (speedProvided)
                playgroundConfig.rainbow.speed = rainbowSpeed;

            // Intensity: 1-100 -> wave length/density (50->2.5 hue step ideal)
            // Lower intensity = longer waves (higher hue step), higher intensity = shorter waves
            float hueStep = max(0.5f, (float)(6.0 - (intensity * 0.08))); // 50->2.0, 25->4.0, 100->1.2
            if (intensityProvided)
                playgroundConfig.rainbow.hueStep = hueStep;

            // Fixed parameters
            playgroundConfig.rainbow.saturation = 255;
            playgroundConfig.rainbow.brightness = 255;
        }

        // Apply the playground configuration temporarily
        ledEffects().updateEffectConfig(playgroundConfig);
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
        success = ledEffects().startEffectCycles(effectName, cycles, c1, c2, c3);
        LOG_VERBOSE("LEDS", "Started LED effect: %s for %d cycles", effectName.c_str(), cycles);
    }
    else
    {
        // Run effect continuously (cycles=0 means indefinite)
        success = ledEffects().startEffectCycles(effectName, 0, c1, c2, c3);
        LOG_VERBOSE("LEDS", "Started LED effect: %s continuously", effectName.c_str());
    }

    if (success)
    {
        JsonDocument response;
        response["message"] = "LED effect started";
        response["effect"] = effectName;
        response["cycles"] = cycles;

        if (colorsAdjusted)
        {
            response["note"] = "Colors adjusted for effect; defaults applied where necessary";
        }

        // Include the resolved colors used
        JsonArray used = response["colorsUsed"].to<JsonArray>();
        used.add(String("#") + String(c1.r, HEX) + String(c1.g, HEX) + String(c1.b, HEX));
        used.add(String("#") + String(c2.r, HEX) + String(c2.g, HEX) + String(c2.b, HEX));
        used.add(String("#") + String(c3.r, HEX) + String(c3.g, HEX) + String(c3.b, HEX));

        // Response includes only new-format fields; no legacy echo

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
    ledEffects().stopEffect();

    LOG_NOTICE("LEDS", "LEDs turned off");

    JsonDocument response;
    response["success"] = true;
    response["message"] = "LEDs turned off";

    String responseStr;
    serializeJson(response, responseStr);
    request->send(200, "application/json", responseStr);
}

#endif // ENABLE_LEDS
