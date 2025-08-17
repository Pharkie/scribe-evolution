/**
 * @file api_handlers.cpp
 * @brief Core API endpoint handlers and shared utilities
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_handlers.h"
#include "validation.h"
#include "../core/config.h"
#include "../core/logging.h"
#include <ArduinoJson.h>

// ========================================
// SHARED API UTILITIES
// ========================================

void sendErrorResponse(AsyncWebServerRequest *request, int code, const String &message)
{
    DynamicJsonDocument errorResponse(256);
    errorResponse["success"] = false;
    errorResponse["error"] = message;

    String errorString;
    serializeJson(errorResponse, errorString);
    request->send(code, "application/json", errorString);
}

void sendSuccessResponse(AsyncWebServerRequest *request, const String &message)
{
    DynamicJsonDocument successResponse(256);
    successResponse["success"] = true;
    successResponse["message"] = message;

    String successString;
    serializeJson(successResponse, successString);
    request->send(200, "application/json", successString);
}

void sendRateLimitResponse(AsyncWebServerRequest *request)
{
    DynamicJsonDocument errorResponse(256);
    errorResponse["success"] = false;
    errorResponse["error"] = getRateLimitReason();

    String errorString;
    serializeJson(errorResponse, errorString);
    request->send(429, "application/json", errorString);
}

void sendValidationError(AsyncWebServerRequest *request, const ValidationResult &result)
{
    DynamicJsonDocument errorResponse(512);
    errorResponse["success"] = false;
    errorResponse["error"] = "Validation failed";
    errorResponse["details"] = result.errorMessage;

    String errorString;
    serializeJson(errorResponse, errorString);
    request->send(400, "application/json", errorString);
}
