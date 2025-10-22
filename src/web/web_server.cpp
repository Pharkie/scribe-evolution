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
#include "auth_middleware.h"
#include <content/content_handlers.h>
#include <config/config.h>
#include <core/logging.h>
#include <core/network.h>
#include <core/printer_discovery.h>
#include <vector>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

// Global variables
extern AsyncWebServer server;

// SSE event source for real-time updates
AsyncEventSource sseEvents("/mqtt-printers");

// Global message storage for printing
Message currentMessage = {"", "", false};

// Mutex to protect concurrent access to currentMessage between cores
SemaphoreHandle_t currentMessageMutex = nullptr;

// ========================================
// CAPTIVE PORTAL HANDLER FOR AP MODE
// ========================================

void handleCaptivePortal(AsyncWebServerRequest *request)
{
    // In AP mode, anything reaching onNotFound should redirect to setup
    request->redirect("/setup.html");
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
// Lightweight Route Registry
// ========================================

struct RouteInfo
{
    String method;
    String path;
    String description;
    bool isAPI;
};

static std::vector<RouteInfo> registeredRoutes;

void registerRoute(const char *method, const char *path, const char *description, bool isAPI)
{
    RouteInfo route;
    route.method = String(method);
    route.path = String(path);
    route.description = String(description);
    route.isAPI = isAPI;
    registeredRoutes.push_back(route);
    LOG_VERBOSE("WEB", "Registered route: %s %s - %s", method, path, description);
}

void addRegisteredRoutesToJson(JsonObject &endpoints)
{
    JsonArray webPages = endpoints["web_pages"].to<JsonArray>();
    JsonArray apiEndpoints = endpoints["api_endpoints"].to<JsonArray>();

    for (const auto &route : registeredRoutes)
    {
        if (route.isAPI)
        {
            JsonObject api = apiEndpoints.add<JsonObject>();
            api["method"] = route.method;
            api["path"] = route.path;
            api["description"] = route.description;
        }
        else
        {
            JsonObject page = webPages.add<JsonObject>();
            page["path"] = route.path;
            page["description"] = route.description;
        }
    }
}

static void setupStaticFileServing(bool isAP)
{
    if (isAP)
    {
        // AP mode - serve files for captive portal
        server.serveStatic("/", LittleFS, "/")
            .setDefaultFile("setup.html")
            .setCacheControl("no-cache");
    }
    else
    {
        // STA mode - Custom handler for index.html to create sessions and set cookie header
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            IPAddress clientIP = request->client()->remoteIP();
            String sessionToken = createSession(clientIP);

            // Let AsyncWebServer handle compression automatically
            if (!LittleFS.exists("/index.html") && !LittleFS.exists("/index.html.gz")) {
                request->send(404, "text/plain", "index.html not found");
                return;
            }

            // Build response so we can add Set-Cookie - AsyncWebServer will find .gz automatically
            AsyncWebServerResponse* response = request->beginResponse(LittleFS, "/index.html", "text/html");

            if (sessionToken.length() > 0) {
                String sessionCookie = getSessionCookieValue(sessionToken);
                if (sessionCookie.length() > 0) {
                    response->addHeader("Set-Cookie", sessionCookie);
                }
                // Also attach CSRF cookie (readable by JS)
                String csrfToken = getCsrfForSession(sessionToken, clientIP);
                String csrfCookie = csrfToken.length() > 0 ? getCsrfCookieValue(csrfToken) : "";
                if (csrfCookie.length() > 0) {
                    response->addHeader("Set-Cookie", csrfCookie);
                }
                LOG_VERBOSE("AUTH", "Created session and set cookies for %s", clientIP.toString().c_str());
            }

            request->send(response);
        });

        // Serve all other static files with compression (no session needed)
        server.serveStatic("/", LittleFS, "/")
            .setDefaultFile("index.html")
            .setCacheControl("max-age=31536000");
    }
}

void setupWebServerRoutes(int maxChars)
{
    setMaxCharacters(maxChars);
    bool isAP = isAPMode();

    // Initialize authentication system
    initAuthSystem();

    LOG_NOTICE("WEB", "Setting up %s mode routes", isAP ? "AP (captive portal)" : "STA (full web interface)");

    if (isAP)
    {
        setupAPModeRoutes();
    }
    else
    {
        setupSTAModeRoutes();
    }

    LOG_VERBOSE("WEB", "Web server routes configured for %s mode", isAP ? "AP" : "STA");
}

void setupAPModeRoutes()
{
    // Setup page - main captive portal entry point
    server.on("/setup.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/setup.html", "text/html"); });

    // Setup API endpoints
    server.on("/api/setup", HTTP_GET, handleSetupGet);
    server.on("/api/setup", HTTP_POST, [](AsyncWebServerRequest *request)
              { handleSetupPost(request); }, NULL, handleChunkedUpload);
    server.on("/api/wifi-scan", HTTP_GET, handleWiFiScan);
    server.on("/api/test-wifi", HTTP_POST, [](AsyncWebServerRequest *request)
              { handleTestWiFi(request); }, NULL, handleChunkedUpload);

    // Captive portal detection - redirect to setup
    const char *captiveUrls[] = {"/hotspot-detect.html", "/generate_204", "/connectivity-check.html", "/ncsi.txt"};
    for (const char *url : captiveUrls)
    {
        server.on(url, HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->redirect("/setup.html"); });
    }

    // Block diagnostics and settings pages in AP mode
    server.on("^\\/diagnostics\\/.*", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->redirect("/setup.html"); });
    server.on("^\\/settings\\/.*", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->redirect("/setup.html"); });

    // Static files with captive portal defaults
    setupStaticFileServing(true);

    // Catch-all redirects to setup
    server.onNotFound(handleCaptivePortal);
}

void setupSTAModeRoutes()
{
    // Clear previous route registry for STA mode
    registeredRoutes.clear();

    // SSE for real-time updates
    sseEvents.onConnect([](AsyncEventSourceClient *client)
                        {
        String printerData = getDiscoveredPrintersJson();
        client->send(printerData.c_str(), "printer-update", millis()); });
    server.addHandler(&sseEvents);
    registerRoute("GET", "/mqtt-printers", "Server-sent events");

    // Connectivity check endpoints (return success, not redirects)
    server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(204); });
    server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "<html><body>OK</body></html>"); });
    server.on("/connectivity-check.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", "<html><body>OK</body></html>"); });
    server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Microsoft NCSI"); });
    registerRoute("GET", "/generate_204", "Connectivity check");
    registerRoute("GET", "/hotspot-detect.html", "Captive portal detection");
    registerRoute("GET", "/connectivity-check.html", "Network connectivity test");
    registerRoute("GET", "/ncsi.txt", "Network connectivity status indicator");

    setupAPIRoutes();
    setupStaticAssets();

    // 404 handler
    server.onNotFound(handleNotFound);
}

void setupAPIRoutes()
{
    // Print endpoints (with authentication)
    server.on("/api/print-local", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handlePrintLocal);
    });
    registerRoute("GET", "/api/print-local", "Print custom message");
    server.on("/api/print-local", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handlePrintLocal);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/print-local", "Print custom message");

    // Content generation (with authentication)
    server.on("/api/riddle", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleRiddle);
    });
    registerRoute("GET", "/api/riddle", "Generate random riddle");
    server.on("/api/joke", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleJoke);
    });
    registerRoute("GET", "/api/joke", "Generate random joke");
    server.on("/api/quote", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleQuote);
    });
    registerRoute("GET", "/api/quote", "Generate random quote");
    server.on("/api/quiz", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleQuiz);
    });
    registerRoute("GET", "/api/quiz", "Generate random quiz");
    server.on("/api/news", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleNews);
    });
    registerRoute("GET", "/api/news", "Generate BBC news headlines");
    server.on("/api/poke", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handlePoke);
    });
    registerRoute("GET", "/api/poke", "Generate poke message");
    server.on("/api/unbidden-ink", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleUnbiddenInk);
    });
    registerRoute("GET", "/api/unbidden-ink", "Generate unbidden ink content");
    server.on("/api/user-message", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleUserMessage);
    });
    registerRoute("GET", "/api/user-message", "Generate user message");

    // Memo endpoints (regex for path parameters)
    server.on("^\\/api\\/memo\\/([1-4])$", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleMemoGet);
    });
    registerRoute("GET", "/api/memo/{id}", "Get processed memo content");
    server.on("^\\/api\\/memo\\/([1-4])$", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleMemoUpdate);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/memo/{id}", "Update specific memo");
    server.on("/api/memos", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleMemosGet);
    });
    registerRoute("GET", "/api/memos", "Get all memos");
    server.on("/api/memos", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleMemosPost);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/memos", "Update all memos");

    // System endpoints
    server.on("/api/diagnostics", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleDiagnostics);
    });
    registerRoute("GET", "/api/diagnostics", "System diagnostics");
    server.on("/api/routes", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleRoutes);
    });
    registerRoute("GET", "/api/routes", "List all routes and endpoints");
    server.on("/api/nvs-dump", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleNVSDump);
    });
    registerRoute("GET", "/api/nvs-dump", "Raw NVS storage dump");
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleConfigGet);
    });
    registerRoute("GET", "/api/config", "Get configuration");
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleConfigPost);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/config", "Update configuration");
    server.on("/api/wifi-scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleWiFiScan);
    });
    registerRoute("GET", "/api/wifi-scan", "Scan WiFi networks");

    // MQTT endpoints
    server.on("/api/print-mqtt", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handlePrintMQTT);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/print-mqtt", "Send MQTT message");
    server.on("/api/test-mqtt", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleTestMQTT);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/test-mqtt", "Test MQTT connection");

    // ChatGPT test endpoint
    server.on("/api/test-chatgpt", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleTestChatGPT);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/test-chatgpt", "Test ChatGPT API token");


#if ENABLE_LEDS
    server.on("/api/leds/test", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleLedEffect);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/leds/test", "Trigger LED Effect");
    server.on("/api/leds/off", HTTP_POST, [](AsyncWebServerRequest *request) {
        authenticatedHandler(request, handleLedOff);
    }, NULL, handleChunkedUpload);
    registerRoute("POST", "/api/leds/off", "Turn LEDs Off");
#endif

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
    registerRoute("GET", "/debug/filesystem", "LittleFS debug info");
}

void setupStaticAssets()
{
    // Explicit favicon handling (no compression needed)
    server.serveStatic("/favicon.ico", LittleFS, "/favicon.ico")
        .setCacheControl("max-age=604800");
    registerRoute("GET", "/favicon.ico", "Favicon ICO file", false);
    server.serveStatic("/favicon-96x96.png", LittleFS, "/favicon-96x96.png")
        .setCacheControl("max-age=604800");
    registerRoute("GET", "/favicon-96x96.png", "Favicon PNG file", false);
    server.serveStatic("/apple-touch-icon.png", LittleFS, "/apple-touch-icon.png")
        .setCacheControl("max-age=604800");
    registerRoute("GET", "/apple-touch-icon.png", "Apple touch icon", false);

    // Font: explicitly disable gzip fallback (no .woff2.gz present / immutable)
    server.serveStatic("/fonts/outfit-variable.woff2", LittleFS, "/fonts/outfit-variable.woff2")
        .setCacheControl("max-age=31536000");
    registerRoute("GET", "/fonts/outfit-variable.woff2", "Outfit variable font", false);

    // All other static files
    setupStaticFileServing(false);

    // Register major static routes
    registerRoute("GET", "/", "Main interface", false);
    registerRoute("GET", "/index.html", "Main interface", false);
    registerRoute("GET", "/setup.html", "Device setup (AP mode only)", false);
    registerRoute("GET", "/settings/*", "Settings pages", false);
    registerRoute("GET", "/diagnostics/*", "Diagnostics pages", false);
    registerRoute("GET", "/css/*", "Stylesheets", false);
    registerRoute("GET", "/js/*", "JavaScript files", false);
    registerRoute("GET", "/images/*", "Image assets", false);
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
    JsonDocument doc;
    JsonArray printersArray = doc["discovered_printers"].to<JsonArray>();

    std::vector<DiscoveredPrinter> discovered = getDiscoveredPrinters();
    for (const auto &printer : discovered)
    {
        if (printer.status == "online")
        {
            JsonObject printerObj = printersArray.add<JsonObject>();
            printerObj["printerId"] = printer.printerId;
            printerObj["name"] = printer.name;
            printerObj["firmwareVersion"] = printer.firmwareVersion;
            printerObj["mdns"] = printer.mdns;
            printerObj["ipAddress"] = printer.ipAddress;
            printerObj["status"] = printer.status;
            printerObj["lastPowerOn"] = printer.lastPowerOn;
            printerObj["timezone"] = printer.timezone;
        }
    }

    doc["count"] = printersArray.size();
    doc["ourPrinterId"] = getPrinterId();

    String response;
    serializeJson(doc, response);
    return response;
}

// Removed handlePrinterUpdates (unused)

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
        JsonDocument doc;
        doc["status"] = status;
        doc["message"] = message;
        doc["timestamp"] = millis();

        String statusData;
        serializeJson(doc, statusData);
        sseEvents.send(statusData.c_str(), "system-status", millis());
        LOG_VERBOSE("WEB", "Sent SSE system status: %s", status.c_str());
    }
}
