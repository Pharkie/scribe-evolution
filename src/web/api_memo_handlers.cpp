/**
 * @file api_memo_handlers.cpp
 * @brief Implementation of memo API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_memo_handlers.h"
#include "../content/memo_handler.h"
#include "../content/content_handlers.h"
#include "../core/nvs_keys.h"
#include "../core/config.h"
#include "../core/config_loader.h"
#include "../core/shared_types.h"
#include "../core/logging.h"
#include "../utils/json_helpers.h"
#include "../utils/time_utils.h"
#include "validation.h"
#include "web_server.h"
#include <Preferences.h>
#include <ArduinoJson.h>

// External message storage for printing
extern Message currentMessage;


void handleMemoGet(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "Memo GET requested from %s", request->client()->remoteIP().toString().c_str());

    // Get memo ID from path parameter
    String idParam = request->pathArg(0);
    int memoId = idParam.toInt();

    if (memoId < 1 || memoId > MEMO_COUNT)
    {
        sendErrorResponse(request, 400, "Invalid memo ID. Must be 1-4");
        return;
    }

    // Get memo content from centralized config system
    const RuntimeConfig &config = getRuntimeConfig();
    String memoContent = config.memos[memoId - 1];
    
    if (memoContent.isEmpty())
    {
        sendErrorResponse(request, 404, "Memo not found");
        return;
    }

    // Expand placeholders (this is the key change - now returns processed content)
    String expandedContent = processMemoPlaceholders(memoContent);

    // Use simple format like other content endpoints (joke, quiz, etc.)
    // Add heading using the standard formatContentWithHeader function for consistency
    String actionName = "MEMO " + String(memoId);
    String contentWithHeading = formatContentWithHeader(actionName, expandedContent, "");
    
    DynamicJsonDocument doc(1024);
    doc["content"] = contentWithHeading;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);

    LOG_VERBOSE("WEB", "Memo %d retrieved: %s", memoId, expandedContent.c_str());
}

void handleMemoUpdate(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "Memo UPDATE requested from %s", request->client()->remoteIP().toString().c_str());

    // Check request method
    if (request->method() != HTTP_POST)
    {
        sendErrorResponse(request, 405, "Method not allowed");
        return;
    }

    // Rate limiting check
    if (isRateLimited())
    {
        request->send(429, "text/plain", getRateLimitReason());
        return;
    }

    // Get memo ID from path parameter
    String idParam = request->pathArg(0);
    int memoId = idParam.toInt();

    if (memoId < 1 || memoId > MEMO_COUNT)
    {
        sendErrorResponse(request, 400, "Invalid memo ID. Must be 1-4");
        return;
    }

    // Parse JSON body
    if (!request->hasParam("body", true))
    {
        sendErrorResponse(request, 400, "Missing request body");
        return;
    }

    String jsonBody = request->getParam("body", true)->value();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonBody);

    if (error)
    {
        sendErrorResponse(request, 400, "Invalid JSON format");
        return;
    }

    if (!doc.containsKey("content"))
    {
        sendErrorResponse(request, 400, "Missing 'content' field");
        return;
    }

    String content = doc["content"].as<String>();

    // Validate memo content
    ValidationResult validation = validateMessage(content, MEMO_MAX_LENGTH);
    if (!validation.isValid)
    {
        sendValidationError(request, validation, 400);
        return;
    }

    // Save to NVS
    Preferences prefs;
    if (!prefs.begin("scribe-app", false)) // read-write
    {
        sendErrorResponse(request, 500, "Failed to access memo storage");
        return;
    }

    const char* memoKeys[] = {NVS_MEMO_1, NVS_MEMO_2, NVS_MEMO_3, NVS_MEMO_4};
    bool success = prefs.putString(memoKeys[memoId - 1], content);
    prefs.end();

    if (!success)
    {
        sendErrorResponse(request, 500, "Failed to save memo");
        return;
    }

    LOG_NOTICE("WEB", "Memo %d updated successfully", memoId);
    request->send(200);
}



void handleMemosUpdate(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "Memos BULK UPDATE requested from %s", request->client()->remoteIP().toString().c_str());

    // Check request method
    if (request->method() != HTTP_POST)
    {
        sendErrorResponse(request, 405, "Method not allowed");
        return;
    }

    // Rate limiting check
    if (isRateLimited())
    {
        request->send(429, "text/plain", getRateLimitReason());
        return;
    }

    // Parse JSON body
    if (!request->hasParam("body", true))
    {
        sendErrorResponse(request, 400, "Missing request body");
        return;
    }

    String jsonBody = request->getParam("body", true)->value();
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonBody);

    if (error)
    {
        sendErrorResponse(request, 400, "Invalid JSON format");
        return;
    }

    // Expect individual memo fields: memo1, memo2, memo3, memo4
    const char* memoFields[] = {"memo1", "memo2", "memo3", "memo4"};
    String memoContents[MEMO_COUNT];

    // Check that all memo fields are present
    for (int i = 0; i < MEMO_COUNT; i++)
    {
        if (!doc.containsKey(memoFields[i]))
        {
            sendErrorResponse(request, 400, "Missing " + String(memoFields[i]));
            return;
        }
        memoContents[i] = doc[memoFields[i]].as<String>();
    }

    // Validate all memos
    for (int i = 0; i < MEMO_COUNT; i++)
    {
        ValidationResult validation = validateMessage(memoContents[i], MEMO_MAX_LENGTH);
        if (!validation.isValid)
        {
            sendValidationError(request, validation, 400);
            return;
        }
    }

    // Save all memos to NVS
    Preferences prefs;
    if (!prefs.begin("scribe-app", false)) // read-write
    {
        sendErrorResponse(request, 500, "Failed to access memo storage");
        return;
    }

    const char* memoKeys[] = {NVS_MEMO_1, NVS_MEMO_2, NVS_MEMO_3, NVS_MEMO_4};
    bool allSuccess = true;

    for (int i = 0; i < MEMO_COUNT; i++)
    {
        if (!prefs.putString(memoKeys[i], memoContents[i]))
        {
            allSuccess = false;
            break;
        }
    }

    prefs.end();

    if (!allSuccess)
    {
        sendErrorResponse(request, 500, "Failed to save one or more memos");
        return;
    }

    LOG_NOTICE("WEB", "All memos updated successfully");
    request->send(200);
}