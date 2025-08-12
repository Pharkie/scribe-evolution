/**
 * @file json_helpers.h
 * @brief JSON response helper utilities
 */

#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#include <ArduinoJson.h>
#include <WebServer.h>

void sendErrorResponse(WebServer &server, int httpCode, const String &errorMessage);
void sendSuccessResponse(WebServer &server, const String &message = "");
void sendRateLimitResponse(WebServer &server);
DynamicJsonDocument createErrorResponse(const String &errorMessage);
DynamicJsonDocument createSuccessResponse(const String &message = "");

#endif
