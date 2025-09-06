/**
 * @file api_client.h
 * @brief HTTP client utilities for external API communication
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

/**
 * @brief HTTP client utilities for external API communication
 *
 * This module provides functions for making HTTPS requests to external APIs
 * and processing template strings.
 */

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
