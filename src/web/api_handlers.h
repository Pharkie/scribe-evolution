/**
 * @file api_handlers.h
 * @brief Core API endpoint handlers and shared utilities
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "../core/shared_types.h"

// Import specialized handler modules
#include "api_system_handlers.h"
#include "api_config_handlers.h"

#ifdef ENABLE_LEDS
#include "api_led_handlers.h"
#endif

// ========================================
// SHARED API UTILITIES
// ========================================

/**
 * @brief Send standardized error response
 * @param request HTTP request object
 * @param code HTTP status code
 * @param message Error message
 */
void sendErrorResponse(AsyncWebServerRequest *request, int code, const String &message);

/**
 * @brief Send standardized success response
 * @param request HTTP request object
 * @param message Success message
 */
void sendSuccessResponse(AsyncWebServerRequest *request, const String &message);

/**
 * @brief Send rate limit response
 * @param request HTTP request object
 */
void sendRateLimitResponse(AsyncWebServerRequest *request);

/**
 * @brief Send validation error response
 * @param request HTTP request object
 * @param result Validation result containing error details
 */
void sendValidationError(AsyncWebServerRequest *request, const ValidationResult &result);

#endif // API_HANDLERS_H
