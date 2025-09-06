/**
 * @file validation.cpp
 * @brief Implementation of input validation utilities for web server
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "validation.h"
#include <config/config.h>
#include <core/logging.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// Static variables for rate limiting
static unsigned long lastRequestTime = 0;
static unsigned long requestCount = 0;
static unsigned long rateLimitWindow = 0;

// Local storage for max message chars
static int localMaxMessageChars = maxCharacters;

// ========================================
// RATE LIMITING
// ========================================

// Static variable to store the last rate limit reason
static String lastRateLimitReason = "";

bool isRateLimited()
{
    // Disable rate limiting completely in AP mode for initial setup
    extern bool isAPMode(); 
    if (isAPMode()) {
        return false;
    }

    unsigned long currentTime = millis();

    // Basic timing rate limit (prevent rapid-fire requests)
    unsigned long timeSinceLastRequest = currentTime - lastRequestTime;
    if (timeSinceLastRequest < minRequestIntervalMs)
    {
        lastRateLimitReason = "Too many requests too quickly. Only " + String(timeSinceLastRequest) + "ms since last request (minimum " + String(minRequestIntervalMs) + "ms required).";
        LOG_WARNING("WEB", "Rate limit triggered: only %lums since last request (min: %lums)",
                    timeSinceLastRequest, minRequestIntervalMs);
        return true;
    }

    // Reset rate limit window every minute
    if (currentTime - rateLimitWindow > rateLimitWindowMs)
    {
        rateLimitWindow = currentTime;
        requestCount = 0;
    }

    // Check requests per minute
    requestCount++;
    if (requestCount > maxRequestsPerMinute)
    {
        lastRateLimitReason = "Too many requests per minute. Maximum " + String(maxRequestsPerMinute) + " requests allowed per minute.";
        LOG_WARNING("WEB", "Rate limit exceeded: %lu requests in current window", requestCount);
        return true;
    }

    lastRequestTime = currentTime;
    LOG_VERBOSE("WEB", "Rate limit OK: %lums since last, request #%lu in window",
                timeSinceLastRequest, requestCount);
    return false;
}

String getRateLimitReason()
{
    return lastRateLimitReason;
}

// ========================================
// VALIDATION FUNCTIONS
// ========================================

ValidationResult validateMessage(const String &message, int maxLength)
{
    if (maxLength == -1)
    {
        maxLength = localMaxMessageChars;
    }

    // Check if message is empty
    if (message.length() == 0)
    {
        return ValidationResult(false, "Message cannot be empty");
    }

    // Check message length
    if (message.length() > maxLength)
    {
        return ValidationResult(false, "Message too long. Maximum " + String(maxLength) + " characters allowed, got " + String(message.length()));
    }

    // Check for null bytes WITHIN the message content (not the terminator)
    for (unsigned int i = 0; i < message.length(); i++)
    {
        if (message.charAt(i) == '\0')
        {
            LOG_WARNING("WEB", "Found null byte at position %d in message content", i);
            return ValidationResult(false, "Message contains null bytes which are not allowed");
        }
    }

    // Check for excessive control characters (except common ones like \n, \r, \t)
    int controlCharCount = 0;
    for (unsigned int i = 0; i < message.length(); i++)
    {
        char c = message.charAt(i);
        if (c < 32 && c != '\n' && c != '\r' && c != '\t')
        {
            controlCharCount++;
        }
    }

    // Allow some control characters but not too many (might indicate binary data)
    if (controlCharCount > message.length() / maxControlCharPercent)
    {
        return ValidationResult(false, "Message contains too many control characters");
    }

    // Check for potential script injection attempts (enhanced XSS protection)
    String messageLower = message;
    messageLower.toLowerCase();
    
    // Enhanced list of dangerous patterns
    const char* xssPatterns[] = {
        "<script", "javascript:", "onload=", "onerror=",
        "<iframe", "<object", "<embed", "<link",
        "onclick=", "onmouseover=", "onfocus=", "onblur=",
        "eval(", "expression(", "vbscript:", "data:",
        "<svg", "<form", "formaction=", "srcdoc="
    };
    
    int patternCount = sizeof(xssPatterns) / sizeof(xssPatterns[0]);
    for (int i = 0; i < patternCount; i++)
    {
        if (messageLower.indexOf(xssPatterns[i]) != -1)
        {
            return ValidationResult(false, "Message contains potentially malicious content");
        }
    }

    return ValidationResult(true);
}

ValidationResult validateMemo(const String &memo, int maxLength)
{
    if (maxLength == -1)
    {
        maxLength = MEMO_MAX_LENGTH; // Use memo specific max length
    }

    // Unlike validateMessage(), memos CAN be empty
    // Only check length if memo is not empty
    if (memo.length() > 0 && memo.length() > maxLength)
    {
        return ValidationResult(false, "Memo too long (max " + String(maxLength) + " characters)");
    }

    return ValidationResult(true);
}

ValidationResult validateJSON(const String &jsonString, const char *requiredFields[], int fieldCount)
{
    if (jsonString.length() == 0)
    {
        return ValidationResult(false, "JSON payload is empty");
    }

    if (jsonString.length() > maxJsonPayloadSize)
    {
        return ValidationResult(false, "JSON payload too large (max " + String(maxJsonPayloadSize / 1024) + "KB)");
    }

    // Parse JSON
    DynamicJsonDocument doc(maxJsonPayloadSize / 2);
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error)
    {
        return ValidationResult(false, "Invalid JSON format: " + String(error.c_str()));
    }

    // Check required fields
    for (int i = 0; i < fieldCount; i++)
    {
        if (!doc.containsKey(requiredFields[i]))
        {
            return ValidationResult(false, "Missing required field: " + String(requiredFields[i]));
        }
    }

    return ValidationResult(true);
}

ValidationResult validateMQTTTopic(const String &topic)
{
    if (topic.length() == 0)
    {
        return ValidationResult(false, "MQTT topic cannot be empty");
    }

    if (topic.length() > maxMqttTopicLength)
    {
        return ValidationResult(false, "MQTT topic too long (max " + String(maxMqttTopicLength) + " characters)");
    }

    // Check for valid MQTT topic characters
    for (unsigned int i = 0; i < topic.length(); i++)
    {
        char c = topic.charAt(i);
        if (c < 32 || c > 126)
        {
            return ValidationResult(false, "MQTT topic contains invalid characters");
        }
    }

    // Check for MQTT wildcards in publish topics (not allowed)
    if (topic.indexOf('+') != -1 || topic.indexOf('#') != -1)
    {
        return ValidationResult(false, "MQTT topic cannot contain wildcards (+, #) for publishing");
    }

    return ValidationResult(true);
}

ValidationResult validateParameter(const String &param, const String &paramName, int maxLength, bool allowEmpty)
{
    if (!allowEmpty && param.length() == 0)
    {
        return ValidationResult(false, "Parameter '" + paramName + "' cannot be empty");
    }

    if (param.length() > maxLength)
    {
        return ValidationResult(false, "Parameter '" + paramName + "' too long (max " + String(maxLength) + " characters)");
    }

    // Check for path traversal attempts
    if (param.indexOf("..") != -1 || param.indexOf("./") != -1 || 
        param.indexOf("\\") != -1 || param.indexOf("//") != -1)
    {
        return ValidationResult(false, "Parameter '" + paramName + "' contains invalid path characters");
    }

    return ValidationResult(true);
}

ValidationResult validateRemoteParameter(AsyncWebServerRequest *request)
{
    if (request->hasParam("remote"))
    {
        String remote = request->getParam("remote")->value();
        ValidationResult paramValidation = validateParameter(remote, "remote", maxRemoteParameterLength, false);
        if (!paramValidation.isValid)
        {
            return paramValidation;
        }
    }
    return ValidationResult(true);
}

// ========================================
// UTILITY FUNCTIONS
// ========================================

String urlDecode(String str)
{
    String decoded = "";
    char temp[] = "00";
    unsigned int len = str.length();

    for (unsigned int i = 0; i < len; i++)
    {
        char decodedChar;
        if (str[i] == '%')
        {
            if (i + 2 < len)
            {
                temp[0] = str[i + 1];
                temp[1] = str[i + 2];
                decodedChar = (char)strtol(temp, NULL, 16);
                decoded += decodedChar;
                i += 2;
            }
            else
            {
                decoded += str[i]; // Invalid encoding, keep as-is
            }
        }
        else
        {
            decoded += str[i];
        }
    }
    return decoded;
}

void sendValidationError(AsyncWebServerRequest *request, const ValidationResult &result, int statusCode)
{
    LOG_WARNING("WEB", "Validation error: %s", result.errorMessage.c_str());

    // Return JSON error response
    DynamicJsonDocument errorResponse(512);
    errorResponse["error"] = result.errorMessage;

    String errorString;
    serializeJson(errorResponse, errorString);
    request->send(statusCode, "application/json", errorString);
}

void setMaxCharacters(int maxChars)
{
    localMaxMessageChars = maxChars;
}
