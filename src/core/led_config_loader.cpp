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

    // Chase Single defaults
    config.chaseSingle.speed = DEFAULT_CHASE_SINGLE_SPEED;
    config.chaseSingle.trailLength = DEFAULT_CHASE_SINGLE_TRAIL_LENGTH;
    config.chaseSingle.trailFade = DEFAULT_CHASE_SINGLE_TRAIL_FADE;
    config.chaseSingle.defaultColor = String(DEFAULT_CHASE_SINGLE_COLOR);

    // Chase Multi defaults
    config.chaseMulti.speed = DEFAULT_CHASE_MULTI_SPEED;
    config.chaseMulti.trailLength = DEFAULT_CHASE_MULTI_TRAIL_LENGTH;
    config.chaseMulti.trailFade = DEFAULT_CHASE_MULTI_TRAIL_FADE;
    config.chaseMulti.colorSpacing = DEFAULT_CHASE_MULTI_COLOR_SPACING;
    config.chaseMulti.color1 = String(DEFAULT_CHASE_MULTI_COLOR1);
    config.chaseMulti.color2 = String(DEFAULT_CHASE_MULTI_COLOR2);
    config.chaseMulti.color3 = String(DEFAULT_CHASE_MULTI_COLOR3);

    // Matrix defaults
    config.matrix.speed = DEFAULT_MATRIX_SPEED;
    config.matrix.drops = DEFAULT_MATRIX_DROPS;
    config.matrix.backgroundFade = DEFAULT_MATRIX_BACKGROUND_FADE;
    config.matrix.trailFade = DEFAULT_MATRIX_TRAIL_FADE;
    config.matrix.brightnessFade = DEFAULT_MATRIX_BRIGHTNESS_FADE;
    config.matrix.defaultColor = String(DEFAULT_MATRIX_COLOR);

    // Twinkle defaults
    config.twinkle.density = DEFAULT_TWINKLE_DENSITY;
    config.twinkle.fadeSpeed = DEFAULT_TWINKLE_FADE_SPEED;
    config.twinkle.minBrightness = DEFAULT_TWINKLE_MIN_BRIGHTNESS;
    config.twinkle.maxBrightness = DEFAULT_TWINKLE_MAX_BRIGHTNESS;
    config.twinkle.defaultColor = String(DEFAULT_TWINKLE_COLOR);

    // Pulse defaults
    config.pulse.speed = DEFAULT_PULSE_SPEED;
    config.pulse.minBrightness = DEFAULT_PULSE_MIN_BRIGHTNESS;
    config.pulse.maxBrightness = DEFAULT_PULSE_MAX_BRIGHTNESS;
    config.pulse.waveFrequency = DEFAULT_PULSE_WAVE_FREQUENCY;
    config.pulse.defaultColor = String(DEFAULT_PULSE_COLOR);

    // Rainbow defaults
    config.rainbow.speed = DEFAULT_RAINBOW_SPEED;
    config.rainbow.saturation = DEFAULT_RAINBOW_SATURATION;
    config.rainbow.brightness = DEFAULT_RAINBOW_BRIGHTNESS;
    config.rainbow.hueStep = DEFAULT_RAINBOW_HUE_STEP;

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
        effectsConfig.chaseSingle.speed = chaseSingle["speed"] | DEFAULT_CHASE_SINGLE_SPEED;
        effectsConfig.chaseSingle.trailLength = chaseSingle["trailLength"] | DEFAULT_CHASE_SINGLE_TRAIL_LENGTH;
        effectsConfig.chaseSingle.trailFade = chaseSingle["trailFade"] | DEFAULT_CHASE_SINGLE_TRAIL_FADE;
        effectsConfig.chaseSingle.defaultColor = chaseSingle["defaultColor"] | String(DEFAULT_CHASE_SINGLE_COLOR);
    }

    // Load Chase Multi configuration
    JsonObject chaseMulti = effects["chaseMulti"];
    if (!chaseMulti.isNull())
    {
        effectsConfig.chaseMulti.speed = chaseMulti["speed"] | DEFAULT_CHASE_MULTI_SPEED;
        effectsConfig.chaseMulti.trailLength = chaseMulti["trailLength"] | DEFAULT_CHASE_MULTI_TRAIL_LENGTH;
        effectsConfig.chaseMulti.trailFade = chaseMulti["trailFade"] | DEFAULT_CHASE_MULTI_TRAIL_FADE;
        effectsConfig.chaseMulti.colorSpacing = chaseMulti["colorSpacing"] | DEFAULT_CHASE_MULTI_COLOR_SPACING;
        effectsConfig.chaseMulti.color1 = chaseMulti["color1"] | String(DEFAULT_CHASE_MULTI_COLOR1);
        effectsConfig.chaseMulti.color2 = chaseMulti["color2"] | String(DEFAULT_CHASE_MULTI_COLOR2);
        effectsConfig.chaseMulti.color3 = chaseMulti["color3"] | String(DEFAULT_CHASE_MULTI_COLOR3);
    }

    // Load Matrix configuration
    JsonObject matrix = effects["matrix"];
    if (!matrix.isNull())
    {
        effectsConfig.matrix.speed = matrix["speed"] | DEFAULT_MATRIX_SPEED;
        effectsConfig.matrix.drops = matrix["drops"] | DEFAULT_MATRIX_DROPS;
        effectsConfig.matrix.backgroundFade = matrix["backgroundFade"] | DEFAULT_MATRIX_BACKGROUND_FADE;
        effectsConfig.matrix.trailFade = matrix["trailFade"] | DEFAULT_MATRIX_TRAIL_FADE;
        effectsConfig.matrix.brightnessFade = matrix["brightnessFade"] | DEFAULT_MATRIX_BRIGHTNESS_FADE;
        effectsConfig.matrix.defaultColor = matrix["defaultColor"] | String(DEFAULT_MATRIX_COLOR);
    }

    // Load Twinkle configuration
    JsonObject twinkle = effects["twinkle"];
    if (!twinkle.isNull())
    {
        effectsConfig.twinkle.density = twinkle["density"] | DEFAULT_TWINKLE_DENSITY;
        effectsConfig.twinkle.fadeSpeed = twinkle["fadeSpeed"] | DEFAULT_TWINKLE_FADE_SPEED;
        effectsConfig.twinkle.minBrightness = twinkle["minBrightness"] | DEFAULT_TWINKLE_MIN_BRIGHTNESS;
        effectsConfig.twinkle.maxBrightness = twinkle["maxBrightness"] | DEFAULT_TWINKLE_MAX_BRIGHTNESS;
        effectsConfig.twinkle.defaultColor = twinkle["defaultColor"] | String(DEFAULT_TWINKLE_COLOR);
    }

    // Load Pulse configuration
    JsonObject pulse = effects["pulse"];
    if (!pulse.isNull())
    {
        effectsConfig.pulse.speed = pulse["speed"] | DEFAULT_PULSE_SPEED;
        effectsConfig.pulse.minBrightness = pulse["minBrightness"] | DEFAULT_PULSE_MIN_BRIGHTNESS;
        effectsConfig.pulse.maxBrightness = pulse["maxBrightness"] | DEFAULT_PULSE_MAX_BRIGHTNESS;
        effectsConfig.pulse.waveFrequency = pulse["waveFrequency"] | DEFAULT_PULSE_WAVE_FREQUENCY;
        effectsConfig.pulse.defaultColor = pulse["defaultColor"] | String(DEFAULT_PULSE_COLOR);
    }

    // Load Rainbow configuration
    JsonObject rainbow = effects["rainbow"];
    if (!rainbow.isNull())
    {
        effectsConfig.rainbow.speed = rainbow["speed"] | DEFAULT_RAINBOW_SPEED;
        effectsConfig.rainbow.saturation = rainbow["saturation"] | DEFAULT_RAINBOW_SATURATION;
        effectsConfig.rainbow.brightness = rainbow["brightness"] | DEFAULT_RAINBOW_BRIGHTNESS;
        effectsConfig.rainbow.hueStep = rainbow["hueStep"] | DEFAULT_RAINBOW_HUE_STEP;
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
