/**
 * @file validation.h
 * @brief Input validation utilities for web server
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef VALIDATION_H
#define VALIDATION_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Forward declaration
extern AsyncWebServer server;

/**
 * @brief Result structure for validation operations
 */
struct ValidationResult
{
    bool isValid;
    String errorMessage;

    ValidationResult(bool valid, const String &error = "") : isValid(valid), errorMessage(error) {}
};

// ========================================
// RATE LIMITING
// ========================================

/**
 * @brief Check if request should be rate limited
 * @return true if request should be blocked, false if allowed
 */
bool isRateLimited();

/**
 * @brief Get the reason for the last rate limit (if any)
 * @return String describing why the request was rate limited
 */
String getRateLimitReason();

// ========================================
// VALIDATION FUNCTIONS
// ========================================

/**
 * @brief Validate message content for printing
 * @param message The message to validate
 * @param maxLength Maximum allowed length (-1 for default)
 * @return ValidationResult with validation status and error message
 */
ValidationResult validateMessage(const String &message, int maxLength = -1);

/**
 * @brief Validate JSON payload
 * @param jsonString The JSON string to validate
 * @param requiredFields Array of required field names
 * @param fieldCount Number of required fields
 * @return ValidationResult with validation status and error message
 */
ValidationResult validateJSON(const String &jsonString, const char *requiredFields[], int fieldCount);

/**
 * @brief Validate MQTT topic format
 * @param topic The topic to validate
 * @return ValidationResult with validation status and error message
 */
ValidationResult validateMQTTTopic(const String &topic);

/**
 * @brief Validate HTTP parameter
 * @param param The parameter value to validate
 * @param paramName The parameter name for error messages
 * @param maxLength Maximum allowed length
 * @param allowEmpty Whether empty values are allowed
 * @return ValidationResult with validation status and error message
 */
ValidationResult validateParameter(const String &param, const String &paramName, int maxLength = 1000, bool allowEmpty = false);

/**
 * @brief Validate optional remote parameter (for MQTT sending)
 * @return ValidationResult with validation status and error message
 */
ValidationResult validateRemoteParameter();
ValidationResult validateRemoteParameter(AsyncWebServerRequest* request);

// ========================================
// UTILITY FUNCTIONS
// ========================================

/**
 * @brief URL decode a string (handle %XX encoding)
 * @param str String to decode
 * @return Decoded string
 */
String urlDecode(String str);

/**
 * @brief Send validation error response
 * @param result The validation result containing the error
 * @param statusCode HTTP status code to send (default 400)
 */
void sendValidationError(const ValidationResult &result, int statusCode = 400);

/**
 * @brief Send validation error response for async requests
 * @param request The async web server request
 * @param result The validation result containing the error
 * @param statusCode HTTP status code to send (default 400)
 */
void sendValidationError(AsyncWebServerRequest* request, const ValidationResult &result, int statusCode = 400);

/**
 * @brief Set maximum message characters for validation
 * @param maxChars Maximum allowed characters
 */
void setMaxCharacters(int maxChars);

#endif // VALIDATION_H
