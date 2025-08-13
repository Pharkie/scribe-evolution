/**
 * @file web_server.cpp
 * @brief Core web server setup and routing for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
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

#include "web_server.h"
#include "web_handlers.h"
#include "api_handlers.h"
#include "validation.h"
#include "../content/content_handlers.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/network.h"
#include "../core/printer_discovery.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Global variables
extern AsyncWebServer server;

// SSE event source for real-time updates
AsyncEventSource sseEvents("/events");

// Global message storage for printing
Message currentMessage = {"", "", false};

// ========================================
// CAPTIVE PORTAL HANDLER FOR AP MODE
// ========================================

/**
 * @brief Captive portal handler that redirects all non-settings requests to settings.html
 * Used when in AP fallback mode to force configuration
 */
void handleCaptivePortal(AsyncWebServerRequest *request)
{
    // Check if this is already a settings-related request
    String uri = request->url();
    // DEBUG: Captive portal called for URI
    // DEBUG: URI check for captive portal handling

    if (uri.startsWith("/settings") ||
        uri.startsWith("/config") ||
        uri.startsWith("/css/") ||
        uri.startsWith("/js/") ||
        uri == "/favicon.ico")
    {
        // Let these requests proceed normally
        // DEBUG: Allowing request to proceed normally
        return;
    }

    // Handle captive portal detection requests
    if (uri == "/hotspot-detect.html" ||
        uri == "/generate_204" ||
        uri == "/connecttest.txt" ||
        uri == "/redirect" ||
        uri.startsWith("/fwlink"))
    {
        // DEBUG: Captive portal detection - responding with redirect to settings
        // Respond with captive portal page
        AsyncWebServerResponse *response = request->beginResponse(302, "text/html",
                                                                  "<!DOCTYPE html><html><head><title>WiFi Setup</title></head>"
                                                                  "<body><h1>Scribe WiFi Setup</h1>"
                                                                  "<p>Redirecting to configuration page...</p>"
                                                                  "<script>window.location.href='http://192.168.4.1/settings.html';</script>"
                                                                  "</body></html>");
        response->addHeader("Location", "http://192.168.4.1/settings.html");
        request->send(response);
        return;
    }

    LOG_VERBOSE("CAPTIVE", "Redirecting captive portal request: %s", uri.c_str());
    // DEBUG: Redirecting to settings
    // URI redirection handling

    // Redirect to settings page
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting to configuration page...");
    response->addHeader("Location", "/settings.html");
    request->send(response);
}

/**
 * @brief Check if request should be redirected in AP mode
 * @return true if request needs captive portal redirect
 */
bool shouldRedirectToSettings(AsyncWebServerRequest *request)
{
    if (!isAPMode())
        return false;

    String uri = request->url();
    // Allow settings page and its dependencies
    return !(uri.startsWith("/settings") ||
             uri.startsWith("/config") ||
             uri.startsWith("/css/") ||
             uri.startsWith("/js/") ||
             uri == "/favicon.ico");
}

// Helper functions for POST body handling using request's built-in storage
void storeRequestBody(AsyncWebServerRequest *request, const String &body)
{
    // Use the request's built-in _tempObject to store the body
    // This automatically cleans up when the request is destroyed
    request->_tempObject = new String(body);
}

String getRequestBody(AsyncWebServerRequest *request)
{
    if (request->_tempObject)
    {
        String *bodyPtr = static_cast<String *>(request->_tempObject);
        String body = *bodyPtr;
        delete bodyPtr; // Clean up the allocated memory
        request->_tempObject = nullptr;
        return body;
    }
    return "";
}

// Helper function to register POST routes with body handling
void registerPOSTRoute(const char *path, ArRequestHandlerFunction handler)
{
    server.on(path, HTTP_POST, [handler](AsyncWebServerRequest *request)
              { handler(request); }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
              {
            String body;
            for (size_t i = 0; i < len; i++) body += (char)data[i];
            storeRequestBody(request, body); });
}

void setupWebServerRoutes(int maxChars)
{
    // Store the maxChars value for validation
    setMaxCharacters(maxChars);

    LOG_NOTICE("WEB", "Setting up web server routes for WiFi mode: %d (AP=%s)", currentWiFiMode, isAPMode() ? "true" : "false");
    // DEBUG: Setting up web routes - WiFi mode and AP mode status

    // Always serve settings page and its dependencies (needed for both STA and AP modes)
    server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (shouldRedirectToSettings(request)) {
            handleCaptivePortal(request);
            return;
        }
        request->send(LittleFS, "/html/settings.html", "text/html"); });

    // Configuration endpoints (needed for settings page)
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request)
              { handleConfigGet(request); });
    server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request)
              { 
        handleConfigPost(request);
        // After successful config save in AP mode, trigger reboot to try new WiFi settings
        if (isAPMode() && request->hasArg("wifi_ssid")) {
            LOG_NOTICE("WEB", "WiFi configuration saved in AP mode, rebooting to try new settings...");
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration saved. Device will reboot and try to connect with new WiFi settings.\"}");
            delay(1000);
            ESP.restart();
        } });

    // CSS and JS files (always needed) - serve directly from LittleFS
    server.serveStatic("/css/tailwind.css", LittleFS, "/css/tailwind.css", "text/css").setTryGzipFirst(false);
    server.serveStatic("/js/app.min.js", LittleFS, "/js/app.min.js", "application/javascript").setTryGzipFirst(false);
    server.serveStatic("/js/settings.js", LittleFS, "/js/settings.min.js", "application/javascript").setTryGzipFirst(false);
    server.serveStatic("/js/settings.min.js", LittleFS, "/js/settings.min.js", "application/javascript").setTryGzipFirst(false);
    server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico", "image/x-icon").setTryGzipFirst(false);

    // In AP mode, set up captive portal for all other requests
    if (isAPMode())
    {
        LOG_VERBOSE("WEB", "Setting up captive portal for AP mode");

        // Catch all other requests and redirect to settings
        server.onNotFound(handleCaptivePortal);

        // Redirect root to settings
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting to configuration page...");
            response->addHeader("Location", "/settings.html");
            request->send(response); });
    }
    else
    {
        LOG_VERBOSE("WEB", "Setting up full routes for STA mode");

        // Add SSE endpoint for real-time updates
        server.addHandler(&sseEvents);

        // HTML and JS
        // Can't use serveStatic to serve index.html from root / without causing problems. Do it this way.
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(LittleFS, "/html/index.html", "text/html"); });
        server.serveStatic("/diagnostics.html", LittleFS, "/html/diagnostics.html", "text/html").setTryGzipFirst(false);
        server.serveStatic("/js/index.min.js", LittleFS, "/js/index.min.js", "application/javascript").setTryGzipFirst(false);
        server.serveStatic("/js/diagnostics.min.js", LittleFS, "/js/diagnostics.min.js", "application/javascript").setTryGzipFirst(false);

        // Form submission handlers
        registerPOSTRoute("/api/print-local", handlePrintContent);
        server.on("/api/print-local", HTTP_GET, handlePrintContent);

        // Content generation endpoints (API)
        registerPOSTRoute("/api/print-test", handlePrintTest);
        registerPOSTRoute("/api/riddle", handleRiddle);
        registerPOSTRoute("/api/joke", handleJoke);
        registerPOSTRoute("/api/quote", handleQuote);
        registerPOSTRoute("/api/quiz", handleQuiz);
        registerPOSTRoute("/api/poke", handlePoke);
        registerPOSTRoute("/api/unbidden-ink", handleUnbiddenInk);
        registerPOSTRoute("/api/user-message", handleUserMessage);

        // API endpoints
        server.on("/api/status", HTTP_GET, handleStatus);
        server.on("/api/buttons", HTTP_GET, handleButtons);
        registerPOSTRoute("/api/mqtt-send", handleMQTTSend);
        server.on("/api/config", HTTP_GET, handleConfigGet);
        registerPOSTRoute("/api/config", handleConfigPost);

        // Debug endpoint to list LittleFS contents (only in STA mode)
        server.on("/debug/filesystem", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            String output = "LittleFS Debug:\n\nTotal space: " + String(LittleFS.totalBytes()) + " bytes\n";
            output += "Used space: " + String(LittleFS.usedBytes()) + " bytes\n";
            output += "Free space: " + String(LittleFS.totalBytes() - LittleFS.usedBytes()) + " bytes\n\n";
            output += "Files:\n";
            
            File root = LittleFS.open("/");
            if (!root || !root.isDirectory()) {
                output += "Failed to open root directory\n";
            } else {
                listDirectory(root, output, 0);
            }
            
            request->send(200, "text/plain", output); });

        // 404 handler for STA mode
        server.onNotFound(handleNotFound);
    }

    LOG_VERBOSE("WEB", "Web server routes configured");
}

// Helper function to recursively list directory contents
void listDirectory(File dir, String &output, int level)
{
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            break;
        }

        // Add indentation
        for (int i = 0; i < level; i++)
        {
            output += "  ";
        }

        if (entry.isDirectory())
        {
            output += "[DIR] " + String(entry.name()) + "/\n";
            listDirectory(entry, output, level + 1);
        }
        else
        {
            output += "[FILE] " + String(entry.name()) + " (" + String(entry.size()) + " bytes)\n";
        }
        entry.close();
    }
}

// Simple, robust real-time updates via smart polling
static String lastPrinterListHash = "";

// Helper function to get printer JSON data for SSE
String getDiscoveredPrintersJson()
{
    DynamicJsonDocument doc(2048);
    JsonArray printersArray = doc.createNestedArray("discovered_printers");

    std::vector<DiscoveredPrinter> discovered = getDiscoveredPrinters();
    for (const auto &printer : discovered)
    {
        if (printer.status == "online")
        {
            JsonObject printerObj = printersArray.createNestedObject();
            printerObj["printer_id"] = printer.printerId;
            printerObj["name"] = printer.name;
            printerObj["firmware_version"] = printer.firmwareVersion;
            printerObj["mdns"] = printer.mdns;
            printerObj["ip_address"] = printer.ipAddress;
            printerObj["status"] = printer.status;
            printerObj["last_power_on"] = printer.lastPowerOn;
            printerObj["timezone"] = printer.timezone;
        }
    }

    doc["count"] = printersArray.size();
    doc["our_printer_id"] = getPrinterId();

    String response;
    serializeJson(doc, response);
    return response;
}

void handlePrinterUpdates(AsyncWebServerRequest *request)
{
    // Get current printer list directly (already complete JSON)
    String response = getDiscoveredPrintersJson();

    // Calculate ETag on the complete response
    uint32_t responseHash = 0;
    for (int i = 0; i < response.length(); i++)
    {
        responseHash = responseHash * 31 + response.charAt(i);
    }
    String currentETag = String(responseHash);

    // Check for If-None-Match header (ETag conditional request)
    String clientETag = "";
    if (request->hasHeader("If-None-Match"))
    {
        clientETag = request->header("If-None-Match");
    }
    LOG_VERBOSE("WEB", "ETag check - Client: %s, Current: %s", clientETag.c_str(), currentETag.c_str());

    if (clientETag.length() > 0 && clientETag.equals("\"" + currentETag + "\""))
    {
        LOG_VERBOSE("WEB", "ETag match - sending 304 Not Modified");
        // Response hasn't changed, send 304 Not Modified
        AsyncWebServerResponse *response = request->beginResponse(304, "application/json", "");
        response->addHeader("ETag", "\"" + currentETag + "\"");
        response->addHeader("Cache-Control", "no-cache");
        response->addHeader("Content-Length", "0");
        request->send(response);
        return;
    }

    LOG_VERBOSE("WEB", "ETag different - sending 200 with new data (length: %d)", response.length());
    // Send response with ETag
    AsyncWebServerResponse *resp = request->beginResponse(200, "application/json", response);
    resp->addHeader("ETag", "\"" + currentETag + "\"");
    resp->addHeader("Cache-Control", "no-cache");
    resp->addHeader("Pragma", "no-cache");
    resp->addHeader("Expires", "0");
    request->send(resp);
}

// ========================================
// SSE (Server-Sent Events) Functions
// ========================================

void sendPrinterUpdate()
{
    if (sseEvents.count() > 0) // Only send if there are connected clients
    {
        String printerData = getDiscoveredPrintersJson();
        sseEvents.send(printerData.c_str(), "printer-update", millis());
        LOG_VERBOSE("WEB", "Sent SSE printer update to %d clients", sseEvents.count());
    }
}

void sendSystemStatus(const String &status, const String &message)
{
    if (sseEvents.count() > 0)
    {
        DynamicJsonDocument doc(512);
        doc["status"] = status;
        doc["message"] = message;
        doc["timestamp"] = millis();

        String statusData;
        serializeJson(doc, statusData);
        sseEvents.send(statusData.c_str(), "system-status", millis());
        LOG_VERBOSE("WEB", "Sent SSE system status: %s", status.c_str());
    }
}