/**
 * @file api_config_handlers.h
 * @brief Configuration API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_CONFIG_HANDLERS_H
#define API_CONFIG_HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

/**
 * @brief Handle configuration GET request (read config.json)
 * @param request The HTTP request
 *
 * Endpoint: GET /api/config
 * Returns the current device configuration with runtime status information
 */
void handleConfigGet(AsyncWebServerRequest *request);

/**
 * @brief Handle configuration POST request (write config.json)
 * @param request The HTTP request containing new configuration
 *
 * Endpoint: POST /api/config
 * Body: JSON configuration object with device, wifi, mqtt, apis, validation,
 *       unbiddenInk, buttons, and leds sections
 *
 * Validates all configuration fields and saves to /config.json
 * Triggers system reload and potential reboot in AP mode
 */
void handleConfigPost(AsyncWebServerRequest *request);

#endif // API_CONFIG_HANDLERS_H
