/**
 * @file led_config_loader.cpp
 * @brief Implementation of LED configuration loading and management
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "led_config_loader.h"

#ifdef ENABLE_LEDS

#include "logging.h"

LedEffectsConfig getDefaultLedEffectsConfig()
{
    LedEffectsConfig config;

    // Use standardized defaults that map from DEFAULT_LED_EFFECT_SPEED/INTENSITY
    // These will be overridden by the API handlers with proper 10-100 mapping
    
    // Chase Single defaults (steps-per-frame x100; 80 = 0.80 steps/frame)
    config.chaseSingle.speed = 80;  // Smooth, reasonably quick default
    config.chaseSingle.trailLength = 15;  // Reasonable trail for 50 intensity  
    config.chaseSingle.trailFade = 15;  // Fixed fade amount
    config.chaseSingle.defaultColor = String(DEFAULT_CHASE_SINGLE_COLOR);

    // Chase Multi defaults (steps-per-frame x100)
    config.chaseMulti.speed = 70;  // Slightly slower than single chase by default
    config.chaseMulti.trailLength = 15;  // Reasonable trail for 50 intensity
    config.chaseMulti.trailFade = 20;  // Fixed fade amount
    config.chaseMulti.colorSpacing = DEFAULT_CHASE_MULTI_COLOR_SPACING;
    config.chaseMulti.color1 = String(DEFAULT_CHASE_MULTI_COLOR1);
    config.chaseMulti.color2 = String(DEFAULT_CHASE_MULTI_COLOR2);
    config.chaseMulti.color3 = String(DEFAULT_CHASE_MULTI_COLOR3);

    // Matrix defaults (mapped from standard 50 speed/intensity)
    config.matrix.speed = 4;  // Reasonable frame delay for 50 speed
    config.matrix.drops = 10;  // Reasonable drops for 50 intensity
    config.matrix.backgroundFade = 64;  // Fixed background fade
    config.matrix.trailFade = 32;  // Fixed trail fade
    config.matrix.brightnessFade = 40;  // Fixed brightness fade
    config.matrix.defaultColor = String(DEFAULT_MATRIX_COLOR);

    // Twinkle defaults (mapped from standard 50 speed/intensity)
    config.twinkle.density = 10;  // Reasonable twinkles for 50 intensity
    config.twinkle.fadeSpeed = 3;  // Reasonable fade speed for 50 speed
    config.twinkle.minBrightness = 50;  // Fixed min brightness
    config.twinkle.maxBrightness = 255;  // Fixed max brightness
    config.twinkle.defaultColor = String(DEFAULT_TWINKLE_COLOR);

    // Pulse defaults (mapped from standard 50 speed/intensity)
    config.pulse.speed = 5;  // Reasonable frame delay for 50 speed
    config.pulse.minBrightness = 127;  // Reasonable variation for 50 intensity
    config.pulse.maxBrightness = 255;  // Fixed max brightness
    config.pulse.waveFrequency = 0.05f;  // Fixed wave frequency
    config.pulse.defaultColor = String(DEFAULT_PULSE_COLOR);

    // Rainbow defaults (mapped from standard 50 speed/intensity)
    config.rainbow.speed = 2.5f;  // Reasonable wave speed for 50 speed
    config.rainbow.saturation = 255;  // Fixed saturation
    config.rainbow.brightness = 255;  // Fixed brightness
    config.rainbow.hueStep = 2.0f;  // Reasonable wave length for 50 intensity

    return config;
}

void loadLedEffectsFromJson(JsonObject leds, LedEffectsConfig &effectsConfig)
{
    // Start with defaults
    effectsConfig = getDefaultLedEffectsConfig();

    // Load effects configuration if present
    JsonObject effects = leds["effects"];
    if (effects.isNull())
    {
        LOG_VERBOSE("LED_CONFIG", "No per-effect configuration found, using defaults");
        return;
    }

    // Load Chase Single configuration
    JsonObject chaseSingle = effects["chaseSingle"];
    if (!chaseSingle.isNull())
    {
        effectsConfig.chaseSingle.speed = chaseSingle["speed"] | effectsConfig.chaseSingle.speed;
        effectsConfig.chaseSingle.trailLength = chaseSingle["trailLength"] | effectsConfig.chaseSingle.trailLength;
        effectsConfig.chaseSingle.trailFade = chaseSingle["trailFade"] | effectsConfig.chaseSingle.trailFade;
        effectsConfig.chaseSingle.defaultColor = chaseSingle["defaultColor"] | effectsConfig.chaseSingle.defaultColor;
    }

    // Load Chase Multi configuration
    JsonObject chaseMulti = effects["chaseMulti"];
    if (!chaseMulti.isNull())
    {
        effectsConfig.chaseMulti.speed = chaseMulti["speed"] | effectsConfig.chaseMulti.speed;
        effectsConfig.chaseMulti.trailLength = chaseMulti["trailLength"] | effectsConfig.chaseMulti.trailLength;
        effectsConfig.chaseMulti.trailFade = chaseMulti["trailFade"] | effectsConfig.chaseMulti.trailFade;
        effectsConfig.chaseMulti.colorSpacing = chaseMulti["colorSpacing"] | effectsConfig.chaseMulti.colorSpacing;
        effectsConfig.chaseMulti.color1 = chaseMulti["color1"] | effectsConfig.chaseMulti.color1;
        effectsConfig.chaseMulti.color2 = chaseMulti["color2"] | effectsConfig.chaseMulti.color2;
        effectsConfig.chaseMulti.color3 = chaseMulti["color3"] | effectsConfig.chaseMulti.color3;
    }

    // Load Matrix configuration
    JsonObject matrix = effects["matrix"];
    if (!matrix.isNull())
    {
        effectsConfig.matrix.speed = matrix["speed"] | effectsConfig.matrix.speed;
        effectsConfig.matrix.drops = matrix["drops"] | effectsConfig.matrix.drops;
        effectsConfig.matrix.backgroundFade = matrix["backgroundFade"] | effectsConfig.matrix.backgroundFade;
        effectsConfig.matrix.trailFade = matrix["trailFade"] | effectsConfig.matrix.trailFade;
        effectsConfig.matrix.brightnessFade = matrix["brightnessFade"] | effectsConfig.matrix.brightnessFade;
        effectsConfig.matrix.defaultColor = matrix["defaultColor"] | effectsConfig.matrix.defaultColor;
    }

    // Load Twinkle configuration
    JsonObject twinkle = effects["twinkle"];
    if (!twinkle.isNull())
    {
        effectsConfig.twinkle.density = twinkle["density"] | effectsConfig.twinkle.density;
        effectsConfig.twinkle.fadeSpeed = twinkle["fadeSpeed"] | effectsConfig.twinkle.fadeSpeed;
        effectsConfig.twinkle.minBrightness = twinkle["minBrightness"] | effectsConfig.twinkle.minBrightness;
        effectsConfig.twinkle.maxBrightness = twinkle["maxBrightness"] | effectsConfig.twinkle.maxBrightness;
        effectsConfig.twinkle.defaultColor = twinkle["defaultColor"] | effectsConfig.twinkle.defaultColor;
    }

    // Load Pulse configuration
    JsonObject pulse = effects["pulse"];
    if (!pulse.isNull())
    {
        effectsConfig.pulse.speed = pulse["speed"] | effectsConfig.pulse.speed;
        effectsConfig.pulse.minBrightness = pulse["minBrightness"] | effectsConfig.pulse.minBrightness;
        effectsConfig.pulse.maxBrightness = pulse["maxBrightness"] | effectsConfig.pulse.maxBrightness;
        effectsConfig.pulse.waveFrequency = pulse["waveFrequency"] | effectsConfig.pulse.waveFrequency;
        effectsConfig.pulse.defaultColor = pulse["defaultColor"] | effectsConfig.pulse.defaultColor;
    }

    // Load Rainbow configuration
    JsonObject rainbow = effects["rainbow"];
    if (!rainbow.isNull())
    {
        effectsConfig.rainbow.speed = rainbow["speed"] | effectsConfig.rainbow.speed;
        effectsConfig.rainbow.saturation = rainbow["saturation"] | effectsConfig.rainbow.saturation;
        effectsConfig.rainbow.brightness = rainbow["brightness"] | effectsConfig.rainbow.brightness;
        effectsConfig.rainbow.hueStep = rainbow["hueStep"] | effectsConfig.rainbow.hueStep;
    }

    LOG_VERBOSE("LED_CONFIG", "Per-effect LED configuration loaded successfully");
}

void saveLedEffectsToJson(JsonObject leds, const LedEffectsConfig &effectsConfig)
{
    // Save per-effect configurations in nested structure
    JsonObject effects = leds.createNestedObject("effects");

    // Chase Single
    JsonObject chaseSingle = effects.createNestedObject("chaseSingle");
    chaseSingle["speed"] = effectsConfig.chaseSingle.speed;
    chaseSingle["trailLength"] = effectsConfig.chaseSingle.trailLength;
    chaseSingle["trailFade"] = effectsConfig.chaseSingle.trailFade;
    chaseSingle["defaultColor"] = effectsConfig.chaseSingle.defaultColor;

    // Chase Multi
    JsonObject chaseMulti = effects.createNestedObject("chaseMulti");
    chaseMulti["speed"] = effectsConfig.chaseMulti.speed;
    chaseMulti["trailLength"] = effectsConfig.chaseMulti.trailLength;
    chaseMulti["trailFade"] = effectsConfig.chaseMulti.trailFade;
    chaseMulti["colorSpacing"] = effectsConfig.chaseMulti.colorSpacing;
    chaseMulti["color1"] = effectsConfig.chaseMulti.color1;
    chaseMulti["color2"] = effectsConfig.chaseMulti.color2;
    chaseMulti["color3"] = effectsConfig.chaseMulti.color3;

    // Matrix
    JsonObject matrix = effects.createNestedObject("matrix");
    matrix["speed"] = effectsConfig.matrix.speed;
    matrix["drops"] = effectsConfig.matrix.drops;
    matrix["backgroundFade"] = effectsConfig.matrix.backgroundFade;
    matrix["trailFade"] = effectsConfig.matrix.trailFade;
    matrix["brightnessFade"] = effectsConfig.matrix.brightnessFade;
    matrix["defaultColor"] = effectsConfig.matrix.defaultColor;

    // Twinkle
    JsonObject twinkle = effects.createNestedObject("twinkle");
    twinkle["density"] = effectsConfig.twinkle.density;
    twinkle["fadeSpeed"] = effectsConfig.twinkle.fadeSpeed;
    twinkle["minBrightness"] = effectsConfig.twinkle.minBrightness;
    twinkle["maxBrightness"] = effectsConfig.twinkle.maxBrightness;
    twinkle["defaultColor"] = effectsConfig.twinkle.defaultColor;

    // Pulse
    JsonObject pulse = effects.createNestedObject("pulse");
    pulse["speed"] = effectsConfig.pulse.speed;
    pulse["minBrightness"] = effectsConfig.pulse.minBrightness;
    pulse["maxBrightness"] = effectsConfig.pulse.maxBrightness;
    pulse["waveFrequency"] = effectsConfig.pulse.waveFrequency;
    pulse["defaultColor"] = effectsConfig.pulse.defaultColor;

    // Rainbow
    JsonObject rainbow = effects.createNestedObject("rainbow");
    rainbow["speed"] = effectsConfig.rainbow.speed;
    rainbow["saturation"] = effectsConfig.rainbow.saturation;
    rainbow["brightness"] = effectsConfig.rainbow.brightness;
    rainbow["hueStep"] = effectsConfig.rainbow.hueStep;

    LOG_VERBOSE("LED_CONFIG", "Per-effect LED configuration saved successfully");
}

#endif // ENABLE_LEDS
