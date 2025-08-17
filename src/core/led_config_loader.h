/**
 * @file led_config_loader.h
 * @brief LED configuration loading and management
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef LED_CONFIG_LOADER_H
#define LED_CONFIG_LOADER_H

#include "led_config.h"

#ifdef ENABLE_LEDS

#include <ArduinoJson.h>

/**
 * @brief Initialize default LED effects configuration
 * @return LedEffectsConfig with default values from led_config.h
 */
LedEffectsConfig getDefaultLedEffectsConfig();

/**
 * @brief Load LED effects configuration from JSON object
 * @param leds JSON object containing LED configuration
 * @param effectsConfig Output: LED effects configuration structure
 */
void loadLedEffectsFromJson(JsonObject leds, LedEffectsConfig &effectsConfig);

/**
 * @brief Save LED effects configuration to JSON object
 * @param leds JSON object to save LED configuration to
 * @param effectsConfig LED effects configuration to save
 */
void saveLedEffectsToJson(JsonObject leds, const LedEffectsConfig &effectsConfig);

#endif // ENABLE_LEDS
#endif // LED_CONFIG_LOADER_H
