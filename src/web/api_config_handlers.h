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

/**
 * @brief Handle setup GET request (AP mode configuration template)
 * @param request The HTTP request
 *
 * Endpoint: GET /api/setup
 * 
 * Returns minimal configuration structure for initial setup with blanks for user input
 */
void handleSetupGet(AsyncWebServerRequest *request);

/**
 * @brief Handle setup POST request (initial device setup with minimal validation)
 * @param request The HTTP request containing basic setup configuration
 *
 * Endpoint: POST /api/setup
 * Body: JSON with device.owner, device.timezone, device.wifi.ssid, device.wifi.password
 * 
 * Performs minimal validation and updates only device settings, preserving all other config
 */
void handleSetupPost(AsyncWebServerRequest *request);

/**
 * @brief Handle memos GET request (read all memos)
 * @param request The HTTP request
 *
 * Endpoint: GET /api/memos
 */
void handleMemosGet(AsyncWebServerRequest *request);

/**
 * @brief Handle memos POST request (save all memos)
 * @param request The HTTP request
 *
 * Endpoint: POST /api/memos
 */
void handleMemosPost(AsyncWebServerRequest *request);

/**
 * @brief Handle MQTT connection test
 * @param request The HTTP request containing MQTT connection details
 *
 * Endpoint: POST /api/test-mqtt
 * Body: JSON with server, port, username, password (optional)
 * 
 * Tests MQTT connection without saving configuration
 */
void handleTestMQTT(AsyncWebServerRequest *request);

/**
 * @brief Handle timezone data GET request
 * @param request The HTTP request
 *
 * Endpoint: GET /api/timezones
 * 
 * Returns IANA timezone database in JSON format with lazy loading and caching
 * Data loaded from /resources/timezones.json on first request
 */
void handleTimezonesGet(AsyncWebServerRequest *request);

#endif // API_CONFIG_HANDLERS_H
