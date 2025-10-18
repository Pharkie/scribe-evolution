/**
 * @file json_helpers.cpp
 * @brief JSON response helper utilities implementation
 */

#include "json_helpers.h"
#include <config/config.h>
#include <web/validation.h>

void sendErrorResponse(AsyncWebServerRequest* request, int httpCode, const String &errorMessage)
{
    JsonDocument response;
    response["error"] = errorMessage;

    String responseString;
    serializeJson(response, responseString);
    request->send(httpCode, "application/json", responseString);
}


void sendRateLimitResponse(AsyncWebServerRequest* request)
{
    sendErrorResponse(request, 429, getRateLimitReason());
}

JsonDocument createErrorResponse(const String &errorMessage)
{
    JsonDocument response;
    response["error"] = errorMessage;
    return response;
}

