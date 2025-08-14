/**
 * @file api_client.cpp
 * @brief Implementation of HTTP client utilities for external API communication
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

#include "api_client.h"
#include "../core/config.h"
#include "../core/logging.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h>

String fetchFromAPI(const String &url, const String &userAgent, int timeoutMs)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_WARNING("API", "API fetch failed - WiFi not connected");
        return "";
    }

    LOG_VERBOSE("API", "Fetching from API: %s", url.c_str());

    // Feed watchdog before starting HTTP operations
    esp_task_wdt_reset();

    WiFiClientSecure client;
    client.setInsecure(); // Skip SSL certificate verification for simplicity
    HTTPClient http;

    // Explicitly specify HTTPS connection
    if (!http.begin(client, url))
    {
        LOG_ERROR("API", "Failed to begin HTTPS connection");
        return "";
    }

    // Feed watchdog after HTTP setup
    esp_task_wdt_reset();

    http.addHeader("Accept", "application/json");
    http.addHeader("User-Agent", userAgent);
    http.setTimeout(timeoutMs);

    // Feed watchdog before making the actual request
    esp_task_wdt_reset();

    int httpResponseCode = http.GET();
    String response = "";

    // Feed watchdog after API call
    esp_task_wdt_reset();

    if (httpResponseCode == 200)
    {
        response = http.getString();
    }
    else if (httpResponseCode == 301 || httpResponseCode == 302)
    {
        // Log redirect information for debugging
        String location = http.getLocation();
        LOG_WARNING("API", "Unexpected redirect to: %s", location.c_str());
        LOG_WARNING("API", "Original URL: %s", url.c_str());
    }
    else
    {
        LOG_WARNING("API", "API request failed with code: %d", httpResponseCode);
    }

    // Proper connection cleanup
    http.end();

    // Feed watchdog after cleanup
    esp_task_wdt_reset();

    return response;
}

String fetchFromAPIWithBearer(const String &url, const String &bearerToken, const String &userAgent, int timeoutMs)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_WARNING("API", "API fetch failed - WiFi not connected");
        return "";
    }

    LOG_VERBOSE("API", "Fetching from API (using Bearer token): %s", url.c_str());
    LOG_VERBOSE("API", "Bearer token length: %d characters", bearerToken.length());

    // Feed watchdog before starting HTTP operations
    esp_task_wdt_reset();

    WiFiClientSecure client;
    client.setInsecure(); // Skip SSL certificate verification for simplicity
    HTTPClient http;

    // Explicitly specify HTTPS connection
    if (!http.begin(client, url))
    {
        LOG_ERROR("API", "Failed to begin HTTPS connection");
        return "";
    }

    // Feed watchdog after HTTP setup
    esp_task_wdt_reset();

    http.addHeader("Accept", "application/json");
    http.addHeader("Authorization", bearerToken);
    http.addHeader("User-Agent", userAgent);
    http.setTimeout(timeoutMs);

    LOG_VERBOSE("API", "Sending GET request with headers set");

    // Feed watchdog before making the actual request
    esp_task_wdt_reset();

    int httpResponseCode = http.GET();
    String response = "";

    // Feed watchdog after API call
    esp_task_wdt_reset();

    LOG_VERBOSE("API", "HTTP response code: %d", httpResponseCode);

    if (httpResponseCode == 200)
    {
        response = http.getString();
        LOG_VERBOSE("API", "Bearer API call successful, response length: %d", response.length());
    }
    else if (httpResponseCode == 301 || httpResponseCode == 302)
    {
        // Log redirect information for debugging
        String location = http.getLocation();
        LOG_WARNING("API", "Unexpected redirect to: %s", location.c_str());
        LOG_WARNING("API", "Original URL: %s", url.c_str());
    }
    else if (httpResponseCode == 401)
    {
        LOG_ERROR("API", "Bearer API request failed - Unauthorized (401). Check Bearer token.");
    }
    else if (httpResponseCode == 403)
    {
        LOG_ERROR("API", "Bearer API request failed - Forbidden (403). Check API permissions.");
    }
    else if (httpResponseCode == 404)
    {
        LOG_ERROR("API", "Bearer API request failed - Not Found (404). Check URL: %s", url.c_str());
    }
    else
    {
        LOG_WARNING("API", "Bearer API request failed with code: %d", httpResponseCode);
    }

    // Proper connection cleanup
    http.end();

    // Feed watchdog after cleanup
    esp_task_wdt_reset();

    return response;
}

String postToAPIWithBearer(const String &url, const String &bearerToken, const String &jsonPayload, const String &userAgent, int timeoutMs)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_WARNING("API", "API POST failed - WiFi not connected");
        return "";
    }

    LOG_VERBOSE("API", "POSTing to API (using Bearer token): %s", url.c_str());
    LOG_VERBOSE("API", "Bearer token length: %d characters", bearerToken.length());
    LOG_VERBOSE("API", "JSON payload: %s", jsonPayload.c_str());

    // Feed watchdog before starting HTTP operations
    esp_task_wdt_reset();

    WiFiClientSecure client;
    client.setInsecure(); // Skip SSL certificate verification for simplicity
    HTTPClient http;

    // Explicitly specify HTTPS connection
    if (!http.begin(client, url))
    {
        LOG_ERROR("API", "Failed to begin HTTPS connection for POST");
        return "";
    }

    // Feed watchdog after HTTP setup
    esp_task_wdt_reset();

    http.addHeader("Accept", "application/json");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", bearerToken);
    http.addHeader("User-Agent", userAgent);
    http.setTimeout(timeoutMs);

    LOG_VERBOSE("API", "Sending POST request with headers set");

    // Feed watchdog before making the actual request
    esp_task_wdt_reset();

    int httpResponseCode = http.POST(jsonPayload);
    String response = "";

    // Feed watchdog after API call
    esp_task_wdt_reset();

    LOG_VERBOSE("API", "HTTP POST response code: %d", httpResponseCode);

    if (httpResponseCode == 200)
    {
        response = http.getString();
        LOG_VERBOSE("API", "Bearer POST API call successful, response length: %d", response.length());
    }
    else if (httpResponseCode == 301 || httpResponseCode == 302)
    {
        // Log redirect information for debugging
        String location = http.getLocation();
        LOG_WARNING("API", "Unexpected redirect to: %s", location.c_str());
        LOG_WARNING("API", "Original URL: %s", url.c_str());
    }
    else if (httpResponseCode == 401)
    {
        LOG_ERROR("API", "Bearer POST API request failed - Unauthorized (401). Check Bearer token.");
    }
    else if (httpResponseCode == 403)
    {
        LOG_ERROR("API", "Bearer POST API request failed - Forbidden (403). Check API permissions.");
    }
    else if (httpResponseCode == 404)
    {
        LOG_ERROR("API", "Bearer POST API request failed - Not Found (404). Check URL: %s", url.c_str());
    }
    else
    {
        LOG_WARNING("API", "Bearer POST API request failed with code: %d", httpResponseCode);
    }

    // Proper connection cleanup
    http.end();

    // Feed watchdog after cleanup
    esp_task_wdt_reset();

    return response;
}

String replaceTemplate(String templateStr, const String &placeholder, const String &value)
{
    String marker = "{{" + placeholder + "}}";
    templateStr.replace(marker, value);
    return templateStr;
}

String reverseString(const String &str)
{
    String reversed = "";
    for (int i = str.length() - 1; i >= 0; i--)
    {
        reversed += str[i];
    }
    return reversed;
}
