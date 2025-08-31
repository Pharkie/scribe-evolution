/**
 * @file web_handlers.cpp
 * @brief Implementation of basic web request handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "web_handlers.h"
#include "validation.h"
#include "../core/config.h"
#include "../core/config_utils.h"
#include "../core/logging.h"
#include "../utils/api_client.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// External declarations
extern AsyncWebServer server;

// ========================================
// STATIC FILE HANDLERS
// ========================================

void handleNotFound(AsyncWebServerRequest *request)
{
    // Rate limit 404 requests to prevent abuse
    if (isRateLimited())
    {
        request->send(429, "text/plain", getRateLimitReason());
        return;
    }

    String uri = request->url();
    String method = (request->method() == HTTP_GET) ? "GET" : "POST";

    // Validate URI to prevent log injection
    if (uri.length() > maxUriDisplayLength)
    {
        uri = uri.substring(0, maxUriDisplayLength) + "...";
    }

    // Remove any potential log injection characters
    uri.replace('\n', ' ');
    uri.replace('\r', ' ');

    // Build comprehensive 404 error message for logging
    String errorDetails = "=== 404 Error === | Method: " + method + " | URI: " + uri + " | Args: " + String(request->args());

    // Limit argument logging to prevent log flooding
    int maxArgs = min((int)request->args(), 5);
    for (int i = 0; i < maxArgs; i++)
    {
        String argName = request->argName(i);
        String argValue = request->arg(i);

        // Sanitize and truncate arguments
        if (argName.length() > 50)
            argName = argName.substring(0, 50) + "...";
        if (argValue.length() > 100)
            argValue = argValue.substring(0, 100) + "...";
        argName.replace('\n', ' ');
        argName.replace('\r', ' ');
        argValue.replace('\n', ' ');
        argValue.replace('\r', ' ');

        errorDetails += " | " + argName + ": " + argValue;
    }
    errorDetails += " | ================";

    LOG_WARNING("WEB", "%s", errorDetails.c_str());

    // Load 404 template from LittleFS
    File templateFile = LittleFS.open("/404.html", "r");
    if (!templateFile)
    {
        // Fallback if template file doesn't exist
        request->send(404, "text/plain", "404 - Page not found: " + method + " " + uri);
        return;
    }

    String template404 = templateFile.readString();
    templateFile.close();

    // Replace template placeholders with dynamic content
    template404 = replaceTemplate(template404, "METHOD", method);
    template404 = replaceTemplate(template404, "URI", uri);
    template404 = replaceTemplate(template404, "HOSTNAME", String(getMdnsHostname()));

    request->send(404, "text/html; charset=UTF-8", template404);
}

// ========================================
// UTILITY FUNCTIONS
// ========================================
