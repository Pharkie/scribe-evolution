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
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// External declarations
extern WebServer server;

// ========================================
// STATIC FILE HANDLERS
// ========================================

void handleConfig()
{
    DynamicJsonDocument doc(jsonDocumentSize);

    // Set max message chars and prompt chars
    doc["maxMessageChars"] = maxCharacters;
    doc["maxPromptChars"] = maxPromptCharacters;

    // Create remote printers array
    JsonArray printers = doc.createNestedArray("remotePrinters");

    // Add local printer first (for self-sending)
    JsonObject localPrinter = printers.createNestedObject();
    localPrinter["name"] = String(getLocalPrinterName());
    localPrinter["topic"] = String(getLocalPrinterTopic());

    // Add other printers from config
    const char *others[maxOtherPrinters][2];
    int numOthers = getOtherPrinters(others, maxOtherPrinters);
    for (int i = 0; i < numOthers; i++)
    {
        JsonObject printer = printers.createNestedObject();
        printer["name"] = String(others[i][0]);
        printer["topic"] = String(others[i][1]);
    }

    // Serialize and send
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleNotFound()
{
    // Rate limit 404 requests to prevent abuse
    if (isRateLimited())
    {
        server.send(429, "text/plain", getRateLimitReason());
        return;
    }

    String uri = server.uri();
    String method = (server.method() == HTTP_GET) ? "GET" : "POST";

    // Validate URI to prevent log injection
    if (uri.length() > maxUriDisplayLength)
    {
        uri = uri.substring(0, maxUriDisplayLength) + "...";
    }

    // Remove any potential log injection characters
    uri.replace('\n', ' ');
    uri.replace('\r', ' ');

    // Build comprehensive 404 error message for logging
    String errorDetails = "=== 404 Error === | Method: " + method + " | URI: " + uri + " | Args: " + String(server.args());

    // Limit argument logging to prevent log flooding
    int maxArgs = min(server.args(), 5);
    for (int i = 0; i < maxArgs; i++)
    {
        String argName = server.argName(i);
        String argValue = server.arg(i);

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
    File templateFile = LittleFS.open("/html/404.html", "r");
    if (!templateFile)
    {
        // Fallback if template file doesn't exist
        server.send(404, "text/plain", "404 - Page not found: " + method + " " + uri);
        return;
    }

    String template404 = templateFile.readString();
    templateFile.close();

    // Replace template placeholders with dynamic content
    template404 = replaceTemplate(template404, "METHOD", method);
    template404 = replaceTemplate(template404, "URI", uri);
    template404 = replaceTemplate(template404, "HOSTNAME", String(getMdnsHostname()));

    server.send(404, "text/html; charset=UTF-8", template404);
}

// ========================================
// UTILITY FUNCTIONS
// ========================================

bool serveFileFromLittleFS(const String &path, const String &contentType)
{
    File file = LittleFS.open(path, "r");
    if (!file)
    {
        server.send(404, "text/plain", path + " not found");
        return false;
    }

    String content = file.readString();
    file.close();

    server.send(200, contentType, content);
    return true;
}
