/**
 * @file web_server.h
 * @brief Core web server setup and routing for Scribe Evolution ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe Evolution ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 * Based on the original Project Scribe by UrbanCircles.
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// External declarations
extern AsyncEventSource sseEvents;

/**
 * @brief Initialize web server routes and handlers
 * @param maxChars Maximum characters allowed in a message
 */
void setupWebServerRoutes(int maxChars);

/**
 * @brief Get stored request body for POST requests
 * @param request The request to get the body for
 * @return The stored request body, or empty string if none
 */
String getRequestBody(AsyncWebServerRequest *request);

/**
 * @brief Get registered routes as JSON for diagnostics
 * @return JSON object containing all registered web pages and API endpoints
 */
void addRegisteredRoutesToJson(JsonObject &endpoints);

/**
 * @brief Setup AP mode routes (captive portal)
 */
void setupAPModeRoutes();

/**
 * @brief Setup STA mode routes (full web interface)
 */
void setupSTAModeRoutes();

/**
 * @brief Setup API endpoints for STA mode
 */
void setupAPIRoutes();

/**
 * @brief Setup static assets (favicons, etc.)
 */
void setupStaticAssets();

/**
 * @brief Get JSON data for discovered printers (includes self via MQTT)
 * @return JSON string containing discovered printers
 */
String getDiscoveredPrintersJson();

// Removed: handlePrinterUpdates (no longer used)

/**
 * @brief Handle captive portal redirects in AP mode
 */
void handleCaptivePortal(AsyncWebServerRequest *request);

/**
 * @brief Helper function for handling chunked uploads (DRY principle)
 */
void handleChunkedUpload(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

/**
 * @brief Check if current request should be redirected to settings in AP mode
 * @param request The request to check
 * @return true if redirect is needed
 */

/**
 * @brief Helper function to recursively list directory contents for debugging
 * @param dir Directory file handle
 * @param output String to append directory listing to
 * @param level Indentation level for nested directories
 */
void listDirectory(File dir, String &output, int level);

// ========================================
// SSE (Server-Sent Events) Functions
// ========================================

/**
 * @brief Send real-time printer discovery updates via SSE
 * Notifies all connected clients when printer status changes
 */
void sendPrinterUpdate();

/**
 * @brief Send system status notifications via SSE
 * @param status Status type (e.g., "connected", "error", "info")
 * @param message Human-readable status message
 */
void sendSystemStatus(const String &status, const String &message = "");

#endif // WEB_SERVER_H
