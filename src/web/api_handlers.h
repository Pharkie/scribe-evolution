/**
 * @file api_handlers.h
 * @brief API endpoint handlers (status, MQTT, settings, etc.)
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include <Arduino.h>
#include "../core/shared_types.h"

// ========================================
// API ENDPOINT HANDLERS
// ========================================

/**
 * @brief Handle system status request
 */
void handleStatus(AsyncWebServerRequest* request);

/**
 * @brief Handle hardware button configuration request
 */
void handleButtons(AsyncWebServerRequest* request);

/**
 * @brief Handle MQTT message sending request
 */
void handleMQTTSend(AsyncWebServerRequest* request);

/**
 * @brief Handle configuration GET request (read config.json)
 */
void handleConfigGet(AsyncWebServerRequest* request);

/**
 * @brief Handle configuration POST request (write config.json)
 */
void handleConfigPost(AsyncWebServerRequest* request);

/**
 * @brief Handle discovered printers request
 */
void handleDiscoveredPrinters(AsyncWebServerRequest* request);

#endif // API_HANDLERS_H
