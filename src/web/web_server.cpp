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
#include "validation.h"
#include "web_handlers.h"
#include "../content/content_handlers.h"
#include "api_handlers.h"
#include "../core/shared_types.h"
#include "../core/logging.h"
#include "../core/network.h"
#include <LittleFS.h>
#include <functional>

// External variable declarations
extern WebServer server;

// Global message storage for printing
Message currentMessage = {"", "", false};

// ========================================
// CAPTIVE PORTAL HANDLER FOR AP MODE
// ========================================

/**
 * @brief Captive portal handler that redirects all non-settings requests to settings.html
 * Used when in AP fallback mode to force configuration
 */
void handleCaptivePortal()
{
    // Check if this is already a settings-related request
    String uri = server.uri();
    if (uri.startsWith("/settings") ||
        uri.startsWith("/config") ||
        uri.startsWith("/css/") ||
        uri.startsWith("/js/") ||
        uri == "/favicon.ico")
    {
        // Let these requests proceed normally
        return;
    }

    LOG_VERBOSE("CAPTIVE", "Redirecting captive portal request: %s", uri.c_str());

    // Redirect to settings page
    server.sendHeader("Location", "/settings.html", true);
    server.send(302, "text/plain", "Redirecting to configuration page...");
}

/**
 * @brief Check if request should be redirected in AP mode
 * @return true if request needs captive portal redirect
 */
bool shouldRedirectToSettings()
{
    if (!isAPMode())
        return false;

    String uri = server.uri();
    // Allow settings page and its dependencies
    return !(uri.startsWith("/settings") ||
             uri.startsWith("/config") ||
             uri.startsWith("/css/") ||
             uri.startsWith("/js/") ||
             uri == "/favicon.ico");
}

// Helper function to create static file handlers with logging
std::function<void()> createStaticHandler(const String &filePath, const String &contentType)
{
    return [filePath, contentType]()
    {
        if (!serveFileFromLittleFS(filePath, contentType))
        {
            LOG_ERROR("WEB", "Failed to serve %s", filePath.c_str());
        }
        else
        {
            LOG_VERBOSE("WEB", "Served %s successfully", filePath.c_str());
        }
    };
}

void setupWebServerRoutes(int maxChars)
{
    // Store the maxChars value for validation
    setMaxCharacters(maxChars);

    LOG_VERBOSE("WEB", "Setting up web server routes for WiFi mode: %d", currentWiFiMode);

    // Always serve settings page and its dependencies (needed for both STA and AP modes)
    server.on("/settings.html", HTTP_GET, []()
              {
        if (shouldRedirectToSettings()) {
            handleCaptivePortal();
            return;
        }
        createStaticHandler("/html/settings.html", "text/html")(); });

    // Configuration endpoints (needed for settings page)
    server.on("/config", HTTP_GET, handleConfigGet);
    server.on("/config", HTTP_POST, []()
              { 
        handleConfigPost();
        // After successful config save in AP mode, trigger reboot to try new WiFi settings
        if (isAPMode() && server.hasArg("wifi_ssid")) {
            LOG_NOTICE("WEB", "WiFi configuration saved in AP mode, rebooting to try new settings...");
            server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Configuration saved. Device will reboot and try to connect with new WiFi settings.\"}");
            delay(1000);
            ESP.restart();
        } });

    // CSS and JS files (always needed)
    server.on("/css/tailwind.css", HTTP_GET, []()
              {
        if (shouldRedirectToSettings()) {
            handleCaptivePortal();
            return;
        }
        createStaticHandler("/css/tailwind.css", "text/css")(); });
    server.on("/js/settings.js", HTTP_GET, []()
              {
        if (shouldRedirectToSettings()) {
            handleCaptivePortal();
            return;
        }
        createStaticHandler("/js/settings.js", "application/javascript")(); });
    server.on("/favicon.ico", HTTP_GET, []()
              {
        if (shouldRedirectToSettings()) {
            handleCaptivePortal();
            return;
        }
        createStaticHandler("/favicon.ico", "image/x-icon")(); });

    // In AP mode, set up captive portal for all other requests
    if (isAPMode())
    {
        LOG_VERBOSE("WEB", "Setting up captive portal for AP mode");

        // Catch all other requests and redirect to settings
        server.onNotFound(handleCaptivePortal);

        // Redirect root to settings
        server.on("/", HTTP_GET, []()
                  {
            server.sendHeader("Location", "/settings.html", true);
            server.send(302, "text/plain", "Redirecting to configuration page..."); });
    }
    else
    {
        LOG_VERBOSE("WEB", "Setting up full routes for STA mode");

        // Full functionality routes for STA mode
        server.on("/", HTTP_GET, createStaticHandler("/html/index.html", "text/html"));
        server.on("/diagnostics.html", HTTP_GET, createStaticHandler("/html/diagnostics.html", "text/html"));

        // Additional JS files for full functionality
        server.on("/js/app.min.js", HTTP_GET, createStaticHandler("/js/app.min.js", "application/javascript"));

        // Form submission handlers
        server.on("/print-local", HTTP_POST, handlePrintContent);
        server.on("/print-local", HTTP_GET, handlePrintContent);

        // Content generation endpoints
        server.on("/print-test", HTTP_POST, handlePrintTest);
        server.on("/riddle", HTTP_POST, handleRiddle);
        server.on("/joke", HTTP_POST, handleJoke);
        server.on("/quote", HTTP_POST, handleQuote);
        server.on("/quiz", HTTP_POST, handleQuiz);
        server.on("/poke", HTTP_POST, handlePoke);
        server.on("/unbidden-ink", HTTP_POST, handleUnbiddenInk);
        server.on("/user-message", HTTP_POST, handleUserMessage);

        // API endpoints
        server.on("/status", HTTP_GET, handleStatus);
        server.on("/buttons", HTTP_GET, handleButtons);
        server.on("/mqtt-send", HTTP_POST, handleMQTTSend);
        server.on("/api/config", HTTP_GET, handleConfigGet);
        server.on("/api/config", HTTP_POST, handleConfigPost);

        // Debug endpoint to list LittleFS contents (only in STA mode)
        server.on("/debug/filesystem", HTTP_GET, []()
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
            
            server.send(200, "text/plain", output); });
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
