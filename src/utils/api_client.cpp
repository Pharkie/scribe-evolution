/**
 * @file api_client.cpp
 * @brief Implementation of thread-safe HTTP client singleton
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

#include "api_client.h"
#include "retry_utils.h"
#include <config/config.h>
#include <core/logging.h>
#include <core/ManagerLock.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <esp_task_wdt.h>

// Singleton instance
APIClient& APIClient::instance() {
    static APIClient instance;
    return instance;
}

// Constructor
APIClient::APIClient() {
    // Mutex and clients created in begin()
}

// Initialize (must be called in setup)
void APIClient::begin() {
    if (initialized) {
        LOG_VERBOSE("API", "APIClient already initialized");
        return;
    }

    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        LOG_ERROR("API", "Failed to create HTTPClient mutex!");
        return;
    }

    wifiClient = new WiFiClientSecure();
    if (wifiClient == nullptr) {
        LOG_ERROR("API", "Failed to allocate WiFiClientSecure!");
        return;
    }
    wifiClient->setInsecure(); // Skip SSL certificate verification

    httpClient = new HTTPClient();
    if (httpClient == nullptr) {
        LOG_ERROR("API", "Failed to allocate HTTPClient!");
        return;
    }

    initialized = true;
    LOG_NOTICE("API", "APIClient initialized (thread-safe singleton)");
}

// Public method: fetchFromAPI
String APIClient::fetchFromAPI(const String &url, const String &userAgent, int timeoutMs) {
    if (!initialized) {
        LOG_ERROR("API", "APIClient not initialized - call begin() first!");
        return "";
    }

    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARNING("API", "API fetch failed - WiFi not connected");
        return "";
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "API");
    if (!lock.isLocked()) {
        LOG_ERROR("API", "Failed to acquire HTTP mutex!");
        return "";
    }

    LOG_VERBOSE("API", "Fetching from API: %s", url.c_str());

    String result = "";
    bool success = retryWithBackoff([&]() -> bool {
        return performSingleAPIRequest(url, userAgent, timeoutMs, result);
    });

    // Mutex automatically released by ManagerLock destructor
    return success ? result : "";
}

// Public method: fetchFromAPIWithBearer
String APIClient::fetchFromAPIWithBearer(const String &url, const String &bearerToken, const String &userAgent, int timeoutMs) {
    if (!initialized) {
        LOG_ERROR("API", "APIClient not initialized - call begin() first!");
        return "";
    }

    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARNING("API", "API fetch failed - WiFi not connected");
        return "";
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "API");
    if (!lock.isLocked()) {
        LOG_ERROR("API", "Failed to acquire HTTP mutex!");
        return "";
    }

    LOG_VERBOSE("API", "Fetching from API (using Bearer token): %s", url.c_str());
    LOG_VERBOSE("API", "Bearer token length: %d characters", bearerToken.length());

    String result = "";
    bool success = retryWithBackoff([&]() -> bool {
        return performSingleBearerAPIRequest(url, bearerToken, userAgent, timeoutMs, result);
    });

    // Mutex automatically released by ManagerLock destructor
    return success ? result : "";
}

// Public method: postToAPIWithBearer
String APIClient::postToAPIWithBearer(const String &url, const String &bearerToken, const String &jsonPayload, const String &userAgent, int timeoutMs) {
    if (!initialized) {
        LOG_ERROR("API", "APIClient not initialized - call begin() first!");
        return "";
    }

    if (WiFi.status() != WL_CONNECTED) {
        LOG_WARNING("API", "API POST failed - WiFi not connected");
        return "";
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "API");
    if (!lock.isLocked()) {
        LOG_ERROR("API", "Failed to acquire HTTP mutex!");
        return "";
    }

    LOG_VERBOSE("API", "POSTing to API (using Bearer token): %s", url.c_str());
    LOG_VERBOSE("API", "Bearer token length: %d characters", bearerToken.length());
    LOG_VERBOSE("API", "JSON payload: %s", jsonPayload.c_str());

    String result = "";
    bool success = retryWithBackoff([&]() -> bool {
        return performSinglePostAPIRequest(url, bearerToken, jsonPayload, userAgent, timeoutMs, result);
    });

    // Mutex automatically released by ManagerLock destructor
    return success ? result : "";
}

// Internal helper: performSingleAPIRequest (mutex already held)
bool APIClient::performSingleAPIRequest(const String &url, const String &userAgent, int timeoutMs, String &result) {
    esp_task_wdt_reset();

    // Use singleton's WiFiClientSecure and HTTPClient
    if (!httpClient->begin(*wifiClient, url)) {
        LOG_ERROR("API", "Failed to begin HTTPS connection");
        return false;
    }

    esp_task_wdt_reset();

    httpClient->addHeader("Accept", "application/json");
    httpClient->addHeader("User-Agent", userAgent);
    httpClient->setTimeout(timeoutMs);

    esp_task_wdt_reset();

    int httpResponseCode = httpClient->GET();

    esp_task_wdt_reset();

    if (httpResponseCode == 200) {
        result = httpClient->getString();
        httpClient->end();
        esp_task_wdt_reset();
        return true;
    } else if (httpResponseCode == 301 || httpResponseCode == 302) {
        String location = httpClient->getLocation();
        LOG_WARNING("API", "Unexpected redirect to: %s", location.c_str());
        LOG_WARNING("API", "Original URL: %s", url.c_str());
    } else {
        LOG_WARNING("API", "API request failed with code: %d", httpResponseCode);
    }

    httpClient->end();
    esp_task_wdt_reset();

    return false;
}

// Internal helper: performSingleBearerAPIRequest (mutex already held)
bool APIClient::performSingleBearerAPIRequest(const String &url, const String &bearerToken, const String &userAgent, int timeoutMs, String &result) {
    esp_task_wdt_reset();

    if (!httpClient->begin(*wifiClient, url)) {
        LOG_ERROR("API", "Failed to begin HTTPS connection for Bearer request");
        return false;
    }

    esp_task_wdt_reset();

    httpClient->addHeader("Accept", "application/json");
    httpClient->addHeader("Authorization", bearerToken);
    httpClient->addHeader("User-Agent", userAgent);
    httpClient->setTimeout(timeoutMs);

    LOG_VERBOSE("API", "Sending GET request with Bearer token headers set");

    esp_task_wdt_reset();

    int httpResponseCode = httpClient->GET();

    esp_task_wdt_reset();

    LOG_VERBOSE("API", "HTTP response code: %d", httpResponseCode);

    if (httpResponseCode == 200) {
        result = httpClient->getString();
        LOG_VERBOSE("API", "Bearer API call successful, response length: %d", result.length());
        httpClient->end();
        esp_task_wdt_reset();
        return true;
    } else if (httpResponseCode == 301 || httpResponseCode == 302) {
        String location = httpClient->getLocation();
        LOG_WARNING("API", "Unexpected redirect to: %s", location.c_str());
        LOG_WARNING("API", "Original URL: %s", url.c_str());
    } else if (httpResponseCode == 401) {
        LOG_ERROR("API", "Bearer API request failed - Unauthorized (401). Check Bearer token.");
    } else if (httpResponseCode == 403) {
        LOG_ERROR("API", "Bearer API request failed - Forbidden (403). Check API permissions.");
    } else if (httpResponseCode == 404) {
        LOG_ERROR("API", "Bearer API request failed - Not Found (404). Check URL: %s", url.c_str());
    } else {
        LOG_WARNING("API", "Bearer API request failed with code: %d", httpResponseCode);
    }

    httpClient->end();
    esp_task_wdt_reset();

    return false;
}

// Internal helper: performSinglePostAPIRequest (mutex already held)
bool APIClient::performSinglePostAPIRequest(const String &url, const String &bearerToken, const String &jsonPayload, const String &userAgent, int timeoutMs, String &result) {
    esp_task_wdt_reset();

    if (!httpClient->begin(*wifiClient, url)) {
        LOG_ERROR("API", "Failed to begin HTTPS connection for POST");
        return false;
    }

    esp_task_wdt_reset();

    httpClient->addHeader("Accept", "application/json");
    httpClient->addHeader("Content-Type", "application/json");
    httpClient->addHeader("Authorization", bearerToken);
    httpClient->addHeader("User-Agent", userAgent);
    httpClient->setTimeout(timeoutMs);

    LOG_VERBOSE("API", "Sending POST request with headers set");

    esp_task_wdt_reset();

    int httpResponseCode = httpClient->POST(jsonPayload);

    esp_task_wdt_reset();

    LOG_VERBOSE("API", "HTTP POST response code: %d", httpResponseCode);

    if (httpResponseCode == 200) {
        result = httpClient->getString();
        LOG_VERBOSE("API", "Bearer POST API call successful, response length: %d", result.length());
        httpClient->end();
        esp_task_wdt_reset();
        return true;
    } else if (httpResponseCode == 301 || httpResponseCode == 302) {
        String location = httpClient->getLocation();
        LOG_WARNING("API", "Unexpected redirect to: %s", location.c_str());
        LOG_WARNING("API", "Original URL: %s", url.c_str());
    } else if (httpResponseCode == 401) {
        LOG_ERROR("API", "Bearer POST API request failed - Unauthorized (401). Check Bearer token.");
    } else if (httpResponseCode == 403) {
        LOG_ERROR("API", "Bearer POST API request failed - Forbidden (403). Check API permissions.");
    } else if (httpResponseCode == 404) {
        LOG_ERROR("API", "Bearer POST API request failed - Not Found (404). Check URL: %s", url.c_str());
    } else {
        LOG_WARNING("API", "Bearer POST API request failed with code: %d", httpResponseCode);
    }

    httpClient->end();
    esp_task_wdt_reset();

    return false;
}

// Utility functions (not part of singleton)
String replaceTemplate(String templateStr, const String &placeholder, const String &value) {
    String marker = "{{" + placeholder + "}}";
    templateStr.replace(marker, value);
    return templateStr;
}

String reverseString(const String &str) {
    String reversed = "";
    for (int i = str.length() - 1; i >= 0; i--) {
        reversed += str[i];
    }
    return reversed;
}
