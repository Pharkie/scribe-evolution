/**
 * @file api_client.h
 * @brief Thread-safe HTTP client singleton for external API communication
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

#ifndef API_CLIENT_H
#define API_CLIENT_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Forward declarations (keep includes in .cpp)
class WiFiClientSecure;
class HTTPClient;

/**
 * @brief Thread-safe HTTP client singleton for external API communication
 *
 * Provides mutex-protected HTTPS requests to prevent concurrent access
 * from multiple tasks (AsyncWebServer, button tasks, Unbidden Ink).
 *
 * Thread-safe for multi-core ESP32 operation (S3 and C3):
 * - Public methods acquire mutex before HTTP operations
 * - Single WiFiClientSecure/HTTPClient instance prevents resource conflicts
 * - Prevents concurrent SSL/TCP operations
 *
 * Pattern follows LedEffects singleton for consistency.
 */
class APIClient
{
public:
    /**
     * @brief Get singleton instance
     * @return Reference to the singleton APIClient instance
     */
    static APIClient& instance();

    /**
     * @brief Initialize HTTP client (must be called in setup)
     */
    void begin();

    /**
     * @brief Make HTTPS API calls with JSON response
     * @param url The API endpoint URL
     * @param userAgent User agent string for the request
     * @param timeoutMs Request timeout in milliseconds (default: 5000)
     * @return String containing the API response, or empty string on failure
     */
    String fetchFromAPI(const String &url, const String &userAgent, int timeoutMs = 5000);

    /**
     * @brief Make HTTPS API calls with Bearer token authorization
     * @param url The API endpoint URL
     * @param bearerToken The Bearer token (including "Bearer " prefix)
     * @param userAgent User agent string for the request
     * @param timeoutMs Request timeout in milliseconds (default: 5000)
     * @return String containing the API response, or empty string on failure
     */
    String fetchFromAPIWithBearer(const String &url, const String &bearerToken, const String &userAgent, int timeoutMs = 5000);

    /**
     * @brief Make HTTPS POST API calls with Bearer token authorization and JSON payload
     * @param url The API endpoint URL
     * @param bearerToken The Bearer token (including "Bearer " prefix)
     * @param jsonPayload The JSON payload to send in the POST body
     * @param userAgent User agent string for the request
     * @param timeoutMs Request timeout in milliseconds (default: 5000)
     * @return String containing the API response, or empty string on failure
     */
    String postToAPIWithBearer(const String &url, const String &bearerToken, const String &jsonPayload, const String &userAgent, int timeoutMs = 5000);

    /**
     * @brief Make HTTPS POST API calls with custom headers and JSON payload
     * @param url The API endpoint URL
     * @param jsonPayload The JSON payload to send in the POST body
     * @param userAgent User agent string for the request
     * @param headers Array of header key-value pairs (terminated by nullptr key)
     * @param headerCount Number of header pairs in the array
     * @param timeoutMs Request timeout in milliseconds (default: 5000)
     * @return String containing the API response, or empty string on failure
     */
    String postToAPIWithCustomHeaders(const String &url, const String &jsonPayload, const String &userAgent, const char *headers[][2], int headerCount, int timeoutMs = 5000);

private:
    APIClient();
    ~APIClient() = default;
    APIClient(const APIClient&) = delete;
    APIClient& operator=(const APIClient&) = delete;

    // Internal helper methods (assume mutex already held)
    bool performSingleAPIRequest(const String &url, const String &userAgent, int timeoutMs, String &result);
    bool performSingleBearerAPIRequest(const String &url, const String &bearerToken, const String &userAgent, int timeoutMs, String &result);
    bool performSinglePostAPIRequest(const String &url, const String &bearerToken, const String &jsonPayload, const String &userAgent, int timeoutMs, String &result);
    bool performSingleCustomHeadersPostRequest(const String &url, const String &jsonPayload, const String &userAgent, const char *headers[][2], int headerCount, int timeoutMs, String &result);

    SemaphoreHandle_t mutex = nullptr;
    WiFiClientSecure* wifiClient = nullptr;
    HTTPClient* httpClient = nullptr;
    bool initialized = false;
};

// Backward-compatible wrapper functions (delegate to singleton)
inline String fetchFromAPI(const String &url, const String &userAgent, int timeoutMs = 5000) {
    return APIClient::instance().fetchFromAPI(url, userAgent, timeoutMs);
}

inline String fetchFromAPIWithBearer(const String &url, const String &bearerToken, const String &userAgent, int timeoutMs = 5000) {
    return APIClient::instance().fetchFromAPIWithBearer(url, bearerToken, userAgent, timeoutMs);
}

inline String postToAPIWithBearer(const String &url, const String &bearerToken, const String &jsonPayload, const String &userAgent, int timeoutMs = 5000) {
    return APIClient::instance().postToAPIWithBearer(url, bearerToken, jsonPayload, userAgent, timeoutMs);
}

/**
 * @brief Simple template replacement function
 * @param templateStr The template string with {{PLACEHOLDER}} markers
 * @param placeholder The placeholder name (without braces)
 * @param value The value to replace the placeholder with
 * @return String with placeholder replaced
 */
String replaceTemplate(String templateStr, const String &placeholder, const String &value);

/**
 * @brief Reverse a string (utility function for answer obfuscation)
 * @param str String to reverse
 * @return Reversed string
 */
String reverseString(const String &str);

#endif // API_CLIENT_H
