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
#include "api_system_handlers.h"
#include "api_nvs_handlers.h"
#include "api_config_handlers.h"
#if ENABLE_LEDS
#include "api_led_handlers.h"
#endif
#include "validation.h"
#include "../content/content_handlers.h"
#include "../core/config.h"
#include "../core/logging.h"
#include "../core/network.h"
#include "../core/printer_discovery.h"
#include <vector>
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
        uri.startsWith("/images/") ||
        uri == "/favicon.ico" ||
        uri == "/favicon.svg" ||
        uri == "/favicon-96x96.png" ||
        uri == "/apple-touch-icon.png" ||
        uri == "/site.webmanifest")
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
             uri.startsWith("/images/") ||
             uri == "/favicon.ico" ||
             uri == "/favicon.svg" ||
             uri == "/favicon-96x96.png" ||
             uri == "/apple-touch-icon.png" ||
             uri == "/site.webmanifest");
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

// ========================================
// Route Registration System
// ========================================

struct RouteInfo
{
    String method;
    String path;
    String description;
    bool isAPI;
};

static std::vector<RouteInfo> registeredRoutes;

void registerRoute(const char *method, const char *path, const char *description, ArRequestHandlerFunction handler, bool isAPI)
{
    // Register with AsyncWebServer
    if (strcmp(method, "GET") == 0)
    {
        server.on(path, HTTP_GET, handler);
    }
    else if (strcmp(method, "POST") == 0)
    {
        server.on(path, HTTP_POST, [handler](AsyncWebServerRequest *request)
                  { handler(request); }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                  {
            String body;
            for (size_t i = 0; i < len; i++) body += (char)data[i];
            storeRequestBody(request, body); });
    }

    // Track for diagnostics
    RouteInfo route;
    route.method = String(method);
    route.path = String(path);
    route.description = description ? String(description) : String("No description");
    route.isAPI = isAPI;
    registeredRoutes.push_back(route);

    LOG_VERBOSE("WEB", "Registered route %d: %s %s - %s (API: %s)", registeredRoutes.size(), method, path, route.description.c_str(), isAPI ? "true" : "false");
}

void addRegisteredRoutesToJson(JsonObject &endpoints)
{
    JsonArray webPages = endpoints.createNestedArray("web_pages");
    JsonArray apiEndpoints = endpoints.createNestedArray("api_endpoints");

    LOG_VERBOSE("WEB", "Generating endpoints JSON - found %d registered routes", registeredRoutes.size());

    for (size_t i = 0; i < registeredRoutes.size(); i++)
    {
        const auto &route = registeredRoutes[i];
        LOG_VERBOSE("WEB", "Processing route %d: %s %s (isAPI: %s)", i, route.method.c_str(), route.path.c_str(), route.isAPI ? "true" : "false");

        if (route.isAPI)
        {
            JsonObject api = apiEndpoints.createNestedObject();
            api["method"] = route.method;
            api["path"] = route.path;
            api["description"] = route.description;
        }
        else
        {
            JsonObject page = webPages.createNestedObject();
            page["path"] = route.path;
            page["description"] = route.description;
        }
    }
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

    // In AP mode, set up minimal captive portal - no route tracking needed
    if (isAPMode())
    {
        LOG_VERBOSE("WEB", "Setting up captive portal for AP mode");

        // Always serve settings page and its dependencies (needed for AP mode)
        server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            if (shouldRedirectToSettings(request)) {
                handleCaptivePortal(request);
                return;
            }
            request->send(LittleFS, "/html/settings.html", "text/html"); });

        // Configuration endpoints (needed for settings page)
        server.on("/config", HTTP_GET, handleConfigGet);
        server.on("/config", HTTP_POST, [](AsyncWebServerRequest *request)
                  { handleConfigPost(request); }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                  {
            String body;
            for (size_t i = 0; i < len; i++) body += (char)data[i];
            storeRequestBody(request, body); });

        // CSS and JS files (always needed) - serve entire directories from LittleFS
        server.serveStatic("/css/", LittleFS, "/css/").setDefaultFile("").setCacheControl("max-age=86400").setTryGzipFirst(false);
        server.serveStatic("/js/", LittleFS, "/js/").setDefaultFile("").setCacheControl("max-age=86400").setTryGzipFirst(false);
        server.serveStatic("/images/", LittleFS, "/images/").setDefaultFile("").setCacheControl("max-age=86400").setTryGzipFirst(false);

        // Favicon files
        server.serveStatic("/favicon.ico", LittleFS, "/favicon/favicon.ico", "image/x-icon").setTryGzipFirst(false);
        server.serveStatic("/favicon.svg", LittleFS, "/favicon/favicon.svg", "image/svg+xml").setTryGzipFirst(false);
        server.serveStatic("/favicon-96x96.png", LittleFS, "/favicon/favicon-96x96.png", "image/png").setTryGzipFirst(false);
        server.serveStatic("/apple-touch-icon.png", LittleFS, "/favicon/apple-touch-icon.png", "image/png").setTryGzipFirst(false);
        server.serveStatic("/site.webmanifest", LittleFS, "/favicon/site.webmanifest", "application/manifest+json").setTryGzipFirst(false);

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
        LOG_VERBOSE("WEB", "Setting up full routes for STA mode with route tracking");

        // Initialize route tracking for STA mode only
        registeredRoutes.clear();

        // Static files - serve entire directories from LittleFS
        server.serveStatic("/css/", LittleFS, "/css/").setDefaultFile("").setCacheControl("max-age=86400").setTryGzipFirst(false);
        server.serveStatic("/js/", LittleFS, "/js/").setDefaultFile("").setCacheControl("max-age=86400").setTryGzipFirst(false);
        server.serveStatic("/images/", LittleFS, "/images/").setDefaultFile("").setCacheControl("max-age=86400").setTryGzipFirst(false);

        // Favicon files
        server.serveStatic("/favicon.ico", LittleFS, "/favicon/favicon.ico", "image/x-icon").setTryGzipFirst(false);
        server.serveStatic("/favicon.svg", LittleFS, "/favicon/favicon.svg", "image/svg+xml").setTryGzipFirst(false);
        server.serveStatic("/favicon-96x96.png", LittleFS, "/favicon/favicon-96x96.png", "image/png").setTryGzipFirst(false);
        server.serveStatic("/apple-touch-icon.png", LittleFS, "/favicon/apple-touch-icon.png", "image/png").setTryGzipFirst(false);
        server.serveStatic("/site.webmanifest", LittleFS, "/favicon/site.webmanifest", "application/manifest+json").setTryGzipFirst(false);

        // Manually track static routes (since serveStatic can't be wrapped)
        registeredRoutes.push_back({"GET", "/css/*", "CSS static files", false});
        registeredRoutes.push_back({"GET", "/js/*", "JavaScript static files", false});
        registeredRoutes.push_back({"GET", "/images/*", "Image static files", false});
        registeredRoutes.push_back({"GET", "/favicon.ico", "Site favicon (ICO)", false});
        registeredRoutes.push_back({"GET", "/favicon.svg", "Site favicon (SVG)", false});
        registeredRoutes.push_back({"GET", "/favicon-96x96.png", "Site favicon (PNG 96x96)", false});
        registeredRoutes.push_back({"GET", "/apple-touch-icon.png", "Apple touch icon", false});
        registeredRoutes.push_back({"GET", "/site.webmanifest", "Web app manifest", false});

        // Register web pages
        registerRoute("GET", "/", "Main printer interface", [](AsyncWebServerRequest *request)
                      { request->send(LittleFS, "/html/index.html", "text/html"); }, false);

        registerRoute("GET", "/settings.html", "Configuration settings", [](AsyncWebServerRequest *request)
                      { request->send(LittleFS, "/html/settings.html", "text/html"); }, false);

        registerRoute("GET", "/diagnostics.html", "System diagnostics", [](AsyncWebServerRequest *request)
                      { request->send(LittleFS, "/html/diagnostics.html", "text/html"); }, false);

        // Serve static files (in addition to tracked routes above)
        server.serveStatic("/diagnostics.html", LittleFS, "/html/diagnostics.html", "text/html").setTryGzipFirst(false);

        // Add SSE endpoint for real-time updates
        sseEvents.onConnect([](AsyncEventSourceClient *client)
                            {
            LOG_VERBOSE("WEB", "New SSE client connected - sending current printer data");
            // Send current printer data immediately to new client
            String printerData = getDiscoveredPrintersJson();
            client->send(printerData.c_str(), "printer-update", millis()); });
        server.addHandler(&sseEvents);

        // Track the SSE endpoint
        registeredRoutes.push_back({"GET", "/events", "Server-sent events", true});

        // Register API endpoints
        registerRoute("POST", "/api/print-local", "Print custom message", handlePrintContent, true);
        registerRoute("GET", "/api/print-local", "Print custom message", handlePrintContent, true);

        registerRoute("POST", "/api/character-test", "Print character test pattern", handlePrintTest, true);
        registerRoute("POST", "/api/riddle", "Print random riddle", handleRiddle, true);
        registerRoute("POST", "/api/joke", "Print random joke", handleJoke, true);
        registerRoute("POST", "/api/quote", "Print random quote", handleQuote, true);
        registerRoute("POST", "/api/quiz", "Print random quiz", handleQuiz, true);
        registerRoute("POST", "/api/news", "Print BBC news headlines", handleNews, true);
        registerRoute("POST", "/api/poke", "Send poke message", handlePoke, true);
        registerRoute("POST", "/api/unbidden-ink", "Trigger unbidden ink", handleUnbiddenInk, true);
        registerRoute("POST", "/api/user-message", "Send user message", handleUserMessage, true);
        registerRoute("GET", "/api/diagnostics", "System diagnostics", handleDiagnostics, true);
        registerRoute("GET", "/api/nvs-dump", "Raw NVS storage dump", handleNVSDump, true);
        registerRoute("POST", "/api/print-mqtt", "Send MQTT message", handleMQTTSend, true);
        registerRoute("GET", "/api/config", "Get configuration", handleConfigGet, true);
        registerRoute("POST", "/api/config", "Update configuration", handleConfigPost, true);
        registerRoute("GET", "/api/scan-wifi", "Scan WiFi networks", handleWiFiScan, true);

#if ENABLE_LEDS
        // Consolidated LED effect endpoints
        registerRoute("POST", "/api/led-effect", "Trigger LED Effect", handleLedEffect, true);
        registerRoute("POST", "/api/leds-off", "Turn LEDs Off", handleLedOff, true);
#endif

        // Debug endpoint to list LittleFS contents (only in STA mode)
        registerRoute("GET", "/debug/filesystem", "LittleFS debug info", [](AsyncWebServerRequest *request)
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
            
            request->send(200, "text/plain", output); }, true);

        // 404 handler for STA mode
        server.onNotFound(handleNotFound);

        // Add 404 handler to route tracking for diagnostics
        RouteInfo notFoundRoute;
        notFoundRoute.method = "ALL";
        notFoundRoute.path = "(unmatched routes)";
        notFoundRoute.description = "404 Not Found handler (serves /html/404.html)";
        notFoundRoute.isAPI = false;
        registeredRoutes.push_back(notFoundRoute);
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