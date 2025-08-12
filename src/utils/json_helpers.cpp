/**
 * @file json_helpers.cpp
 * @brief JSON response helper utilities implementation
 */

#include "json_helpers.h"
#include "../core/config.h"
#include "../web/validation.h"

void sendErrorResponse(WebServer &server, int httpCode, const String &errorMessage)
{
    DynamicJsonDocument response(256);
    response["success"] = false;
    response["error"] = errorMessage;

    String responseString;
    serializeJson(response, responseString);
    server.send(httpCode, "application/json", responseString);
}

void sendSuccessResponse(WebServer &server, const String &message)
{
    DynamicJsonDocument response(256);
    response["success"] = true;
    if (message.length() > 0)
    {
        response["message"] = message;
    }

    String responseString;
    serializeJson(response, responseString);
    server.send(200, "application/json", responseString);
}

void sendRateLimitResponse(WebServer &server)
{
    sendErrorResponse(server, 429, getRateLimitReason());
}

DynamicJsonDocument createErrorResponse(const String &errorMessage)
{
    DynamicJsonDocument response(256);
    response["success"] = false;
    response["error"] = errorMessage;
    return response;
}

DynamicJsonDocument createSuccessResponse(const String &message)
{
    DynamicJsonDocument response(256);
    response["success"] = true;
    if (message.length() > 0)
    {
        response["message"] = message;
    }
    return response;
}
