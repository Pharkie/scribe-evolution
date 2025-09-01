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
#include "api_memo_handlers.h"
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
 * @brief Captive portal handler that redirects all non-setup requests to setup.html
 * Used when in AP fallback mode to force configuration
 */
// Simple rate limiter for captive portal (AP mode only)
static unsigned long lastCaptivePortalRequest = 0;
static const unsigned long CAPTIVE_PORTAL_MIN_INTERVAL_MS = 100; // Max 10 requests per second

void handleCaptivePortal(AsyncWebServerRequest *request)
{
    String uri = request->url();

    // Rate limit ALL requests except static assets
    bool isStaticAsset = (uri.startsWith("/css/") ||
                          uri.startsWith("/js/") ||
                          uri.startsWith("/images/") ||
                          uri == "/favicon.ico" ||
                          uri == "/favicon.svg" ||
                          uri == "/favicon-96x96.png" ||
                          uri == "/apple-touch-icon.png" ||
                          uri == "/site.webmanifest");

    if (!isStaticAsset)
    {
        // Rate limit all non-static requests to prevent system overload
        unsigned long now = millis();
        if (now - lastCaptivePortalRequest < CAPTIVE_PORTAL_MIN_INTERVAL_MS)
        {
            // Too many requests - send minimal response
            request->send(429, "text/plain", "Rate limited");
            return;
        }
        lastCaptivePortalRequest = now;
    }

    // Allow setup-related requests to proceed normally (but rate-limited)
    if (uri == "/setup.html" ||
        uri == "/api/setup" ||
        uri == "/api/wifi-scan" ||
        isStaticAsset)
    {
        return; // Let these proceed to their handlers
    }

    // Redirect everything else to setup page
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
    response->addHeader("Location", "http://192.168.4.1/setup.html");
    request->send(response);
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

// Helper function for chunked upload handling (DRY principle)
void handleChunkedUpload(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    String *bodyPtr = static_cast<String *>(request->_tempObject);
    if (index == 0)
    {
        // First chunk - create new string
        if (bodyPtr)
            delete bodyPtr;
        bodyPtr = new String();
        request->_tempObject = bodyPtr;
        bodyPtr->reserve(total); // Reserve space for entire body
    }

    // Append this chunk
    for (size_t i = 0; i < len; i++)
    {
        *bodyPtr += (char)data[i];
    }
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
                  { handler(request); }, NULL, handleChunkedUpload);
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
              { handler(request); }, NULL, handleChunkedUpload);
}

// Helper function to setup static file serving (DRY principle)
void setupStaticRoutes()
{
    // Single route for entire static filesystem - AsyncWebServer handles everything automatically
    server.serveStatic("/", LittleFS, "/")
        .setDefaultFile("index.html")
        .setCacheControl("max-age=31536000");
}

void setupWebServerRoutes(int maxChars)
{
    // Store the maxChars value for validation
    setMaxCharacters(maxChars);

    if (isAPMode())
    {
        LOG_NOTICE("WEB", "Setting up captive portal for AP mode setup");
    }
    else
    {
        LOG_NOTICE("WEB", "Setting up web server routes for WiFi mode");
    }

    // In AP mode, set up minimal captive portal - no route tracking needed
    if (isAPMode())
    {
        LOG_VERBOSE("WEB", "Setting up captive portal for AP mode");

        // Setup page (only available in AP mode) - request->send requires explicit .gz
        server.on("/setup.html", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            if (!isAPMode()) {
                request->send(LittleFS, "/404.html.gz", "text/html", 404);
                return;
            }
            request->send(LittleFS, "/setup.html.gz", "text/html"); });

        // Setup endpoints for AP mode initial configuration
        server.on("/api/setup", HTTP_GET, handleSetupGet);
        server.on("/api/setup", HTTP_POST, [](AsyncWebServerRequest *request)
                  { handleSetupPost(request); }, NULL, handleChunkedUpload);

        // WiFi scanning endpoint (needed for setup)
        server.on("/api/wifi-scan", HTTP_GET, handleWiFiScan);

        // Setup static file serving
        setupStaticRoutes();

        // Catch all other requests and redirect to setup
        server.onNotFound(handleCaptivePortal);

        // Redirect root to setup
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Redirecting to setup page...");
            response->addHeader("Location", "/setup.html");
            request->send(response); });
    }
    else
    {
        LOG_VERBOSE("WEB", "Setting up full routes for STA mode with route tracking");

        // Initialize route tracking for STA mode only
        registeredRoutes.clear();

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
        registerRoute("GET", "/api/riddle", "Generate random riddle", handleRiddle, true);
        registerRoute("GET", "/api/joke", "Generate random joke", handleJoke, true);
        registerRoute("GET", "/api/quote", "Generate random quote", handleQuote, true);
        registerRoute("GET", "/api/quiz", "Generate random quiz", handleQuiz, true);
        registerRoute("GET", "/api/news", "Generate BBC news headlines", handleNews, true);
        registerRoute("GET", "/api/poke", "Generate poke message", handlePoke, true);
        registerRoute("GET", "/api/unbidden-ink", "Generate unbidden ink content", handleUnbiddenInk, true);
        registerRoute("GET", "/api/user-message", "Generate user message", handleUserMessage, true);

        // Memo API endpoints - registered later with other handlers

        // Individual memo operations - using path parameters
        server.on("^\\/api\\/memo\\/([1-4])$", HTTP_GET, [](AsyncWebServerRequest *request)
                  { handleMemoGet(request); });
        server.on("^\\/api\\/memo\\/([1-4])$", HTTP_POST, [](AsyncWebServerRequest *request)
                  { handleMemoUpdate(request); }, NULL, handleChunkedUpload);

        // Track memo routes for diagnostics
        registeredRoutes.push_back({"GET", "/api/memo/{id}", "Get processed memo content", true});
        registeredRoutes.push_back({"POST", "/api/memo/{id}", "Update specific memo", true});

        registerRoute("GET", "/api/diagnostics", "System diagnostics", handleDiagnostics, true);
        registerRoute("GET", "/api/routes", "List all routes and endpoints", handleRoutes, true);
        registerRoute("GET", "/api/nvs-dump", "Raw NVS storage dump", handleNVSDump, true);
        registerRoute("POST", "/api/print-mqtt", "Send MQTT message", handleMQTTSend, true);
        registerRoute("GET", "/api/config", "Get configuration", handleConfigGet, true);
        registerRoute("POST", "/api/config", "Update configuration", handleConfigPost, true);
        registerRoute("POST", "/api/test-mqtt", "Test MQTT connection", handleTestMQTT, true);
        // Timezone data: serve static file with proper caching (24 hours)
        server.serveStatic("/api/timezones", LittleFS, "/resources/timezones.json")
              .setCacheControl("public, max-age=86400");
        registeredRoutes.push_back({"GET", "/api/timezones", "Get IANA timezone data (static file)", true});
        registerRoute("GET", "/api/memos", "Get all memos", handleMemosGet, true);
        registerRoute("POST", "/api/memos", "Update all memos", handleMemosPost, true);
        registerRoute("GET", "/api/wifi-scan", "Scan WiFi networks", handleWiFiScan, true);

#if ENABLE_LEDS
        // Consolidated LED effect endpoints
        registerRoute("POST", "/api/leds/test", "Trigger LED Effect", handleLedEffect, true);
        registerRoute("POST", "/api/leds/off", "Turn LEDs Off", handleLedOff, true);
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

        // Favicons/icons: already-compressed formats → disable .gz lookup
        server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico")
            .setTryGzipFirst(false)
            .setCacheControl("max-age=604800");
        server.serveStatic("/favicon-96x96.png", LittleFS, "/favicon-96x96.png")
            .setTryGzipFirst(false)
            .setCacheControl("max-age=604800");
        server.serveStatic("/apple-touch-icon.png", LittleFS, "/apple-touch-icon.png")
            .setTryGzipFirst(false)
            .setCacheControl("max-age=604800");

        // CRITICAL: Setup static file serving AFTER all API routes AND explicit favicon routes
        // This ensures API endpoints and explicit routes are matched before the catch-all static handler
        setupStaticRoutes();

        // Track explicit favicon routes for diagnostics
        registeredRoutes.push_back({"GET", "/favicon-96x96.png", "Favicon PNG file", false});
        registeredRoutes.push_back({"GET", "/favicon.ico", "Favicon ICO file", false});
        registeredRoutes.push_back({"GET", "/apple-touch-icon.png", "Apple touch icon", false});

        // Track static routes for diagnostics
        registeredRoutes.push_back({"GET", "/*", "All other static files (HTML, CSS, JS, images)", false});

        // 404 handler for STA mode (must be LAST)
        server.onNotFound(handleNotFound);

        // Add 404 handler to route tracking for diagnostics
        RouteInfo notFoundRoute;
        notFoundRoute.method = "ALL";
        notFoundRoute.path = "(unmatched routes)";
        notFoundRoute.description = "404 Not Found handler (serves /404.html)";
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