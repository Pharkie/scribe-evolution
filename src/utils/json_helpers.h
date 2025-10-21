/**
 * @file json_helpers.h
 * @brief JSON response helper utilities
 */

#ifndef JSON_HELPERS_H
#define JSON_HELPERS_H

#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>

void sendErrorResponse(AsyncWebServerRequest* request, int httpCode, const String &errorMessage);
void sendRateLimitResponse(AsyncWebServerRequest* request);
JsonDocument createErrorResponse(const String &errorMessage);

#endif
