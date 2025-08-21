/**
 * @file api_system_handlers.h
 * @brief System and diagnostics API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_SYSTEM_HANDLERS_H
#define API_SYSTEM_HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

/**
 * @brief Handle system diagnostics request
 * @param request The HTTP request
 *
 * Endpoint: GET /api/diagnostics
 * Returns comprehensive system diagnostics including device info,
 * hardware status, memory usage, network status, and feature configuration.
 */
void handleDiagnostics(AsyncWebServerRequest *request);

/**
 * @brief Handle MQTT message sending request
 * @param request The HTTP request containing MQTT topic and message
 *
 * Endpoint: POST /api/print-mqtt
 * Body: JSON with "topic" and "message" fields
 *
 * Validates MQTT connectivity, topic format, and message content
 * before publishing to the MQTT broker
 */
void handleMQTTSend(AsyncWebServerRequest *request);

/**
 * @brief Handle WiFi network scanning request
 * @details Scans for available WiFi networks and returns them with signal strength
 *
 * Endpoint: GET /api/scan-wifi
 */
void handleWiFiScan(AsyncWebServerRequest *request);

#endif // API_SYSTEM_HANDLERS_H
