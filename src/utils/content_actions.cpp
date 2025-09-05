/**
 * @file content_actions.cpp
 * @brief Implementation of shared business logic for content generation actions
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "content_actions.h"
#include "../content/content_generators.h"
#include "../content/content_handlers.h"
#include "../content/memo_handler.h"
#include "../utils/time_utils.h"
#include "../core/shared_types.h"
#include "../core/logging.h"
#include <ArduinoJson.h>

// External variable reference
extern Message currentMessage;

ContentActionResult executeContentAction(ContentActionType actionType,
                                         const String &customData,
                                         const String &sender)
{
    return executeContentActionWithTimeout(actionType, customData, sender, 5000);
}

ContentActionResult executeContentActionWithTimeout(ContentActionType actionType,
                                                    const String &customData,
                                                    const String &sender,
                                                    int timeoutMs)
{
    LOG_VERBOSE("CONTENT_ACTION", "Executing content action: %s (timeout: %dms)",
                actionTypeToString(actionType).c_str(), timeoutMs);

    String content;
    String actionName = actionTypeToString(actionType);

    // Generate raw content based on action type with timeout
    switch (actionType)
    {
    case ContentActionType::JOKE:
        content = generateJokeContent(timeoutMs);
        break;

    case ContentActionType::RIDDLE:
        content = generateRiddleContent();
        break;

    case ContentActionType::QUOTE:
        content = generateQuoteContent(timeoutMs);
        break;

    case ContentActionType::QUIZ:
        content = generateQuizContent(timeoutMs);
        break;

    case ContentActionType::NEWS:
        content = generateNewsContent(timeoutMs);
        break;

    case ContentActionType::PRINT_TEST:
        content = loadPrintTestContent();
        if (content.length() > 0)
        {
            content += "\n\n";
            actionName = "TEST PRINT";
        }
        break;

    case ContentActionType::POKE:
        content = generatePokeContent();
        break;

    case ContentActionType::USER_MESSAGE:
        if (customData.length() == 0)
        {
            return ContentActionResult(false, "", "", "No message content provided");
        }
        content = customData;
        actionName = "MESSAGE";
        break;

    case ContentActionType::UNBIDDEN_INK:
        content = generateUnbiddenInkContent(customData);
        actionName = "UNBIDDEN INK";
        break;

    case ContentActionType::MEMO1:
        content = generateMemoContent(1);
        actionName = "MEMO 1";
        break;

    case ContentActionType::MEMO2:
        content = generateMemoContent(2);
        actionName = "MEMO 2";
        break;

    case ContentActionType::MEMO3:
        content = generateMemoContent(3);
        actionName = "MEMO 3";
        break;

    case ContentActionType::MEMO4:
        content = generateMemoContent(4);
        actionName = "MEMO 4";
        break;

    default:
        return ContentActionResult(false, "", "", "Unknown content action type");
    }

    // Check if content generation failed
    if (content.length() == 0)
    {
        String actionString = actionTypeToString(actionType);
        actionString.toLowerCase();
        String errorMsg = "Failed to generate " + actionString + " content";
        LOG_ERROR("CONTENT_ACTION", "%s", errorMsg.c_str());
        return ContentActionResult(false, "", "", errorMsg);
    }

    LOG_VERBOSE("CONTENT_ACTION", "Successfully generated %s content (%d chars)",
                actionName.c_str(), content.length());

    return ContentActionResult(true, actionName, content, "");
}

bool queueContentForPrinting(const ContentActionResult &result)
{
    if (!result.success || result.header.length() == 0 || result.body.length() == 0)
    {
        LOG_ERROR("CONTENT_ACTION", "Cannot queue invalid content for printing");
        return false;
    }

    // Expand memo placeholders at print time (if this is a memo)
    String finalBody = result.body;
    if (result.header.startsWith("MEMO"))
    {
        finalBody = processMemoPlaceholders(result.body);
        LOG_VERBOSE("CONTENT_ACTION", "Expanded memo placeholders for: %s", result.header.c_str());
    }

    // Format content for local printing (header + processed body)
    String formattedContent = result.header + "\n\n" + finalBody;
    currentMessage.message = formattedContent;
    currentMessage.timestamp = getFormattedDateTime();
    currentMessage.shouldPrintLocally = true;

    LOG_VERBOSE("CONTENT_ACTION", "Content queued for local printing (%d chars)",
                formattedContent.length());
    return true;
}

bool executeAndQueueContent(ContentActionType actionType, const String &customData)
{
    ContentActionResult result = executeContentAction(actionType, customData);

    if (!result.success)
    {
        LOG_ERROR("CONTENT_ACTION", "Content action failed: %s", result.errorMessage.c_str());
        return false;
    }

    return queueContentForPrinting(result);
}

ContentActionType endpointToActionType(const String &endpoint)
{
    if (endpoint == "/api/joke")
        return ContentActionType::JOKE;
    if (endpoint == "/api/riddle")
        return ContentActionType::RIDDLE;
    if (endpoint == "/api/quote")
        return ContentActionType::QUOTE;
    if (endpoint == "/api/quiz")
        return ContentActionType::QUIZ;
    if (endpoint == "/api/news")
        return ContentActionType::NEWS;
    if (endpoint == "/api/character-test")
        return ContentActionType::PRINT_TEST;
    if (endpoint == "/api/poke")
        return ContentActionType::POKE;
    if (endpoint == "/api/user-message")
        return ContentActionType::USER_MESSAGE;
    if (endpoint == "/api/unbidden-ink")
        return ContentActionType::UNBIDDEN_INK;

    // Return a default that will be handled as unknown
    return ContentActionType::JOKE; // This will be validated by caller
}

String actionTypeToString(ContentActionType actionType)
{
    switch (actionType)
    {
    case ContentActionType::JOKE:
        return "JOKE";
    case ContentActionType::RIDDLE:
        return "RIDDLE";
    case ContentActionType::QUOTE:
        return "QUOTE";
    case ContentActionType::QUIZ:
        return "QUIZ";
    case ContentActionType::NEWS:
        return "NEWS";
    case ContentActionType::PRINT_TEST:
        return "PRINT TEST";
    case ContentActionType::POKE:
        return "POKE";
    case ContentActionType::USER_MESSAGE:
        return "MESSAGE";
    case ContentActionType::UNBIDDEN_INK:
        return "UNBIDDEN INK";
    case ContentActionType::MEMO1:
        return "MEMO1";
    case ContentActionType::MEMO2:
        return "MEMO2";
    case ContentActionType::MEMO3:
        return "MEMO3";
    case ContentActionType::MEMO4:
        return "MEMO4";
    default:
        return "UNKNOWN";
    }
}

ContentActionType stringToActionType(const String &actionString)
{
    String action = actionString;
    action.toUpperCase(); // Ensure consistent case
    action.trim();        // Remove any whitespace

    if (action == "JOKE")
        return ContentActionType::JOKE;
    if (action == "RIDDLE")
        return ContentActionType::RIDDLE;
    if (action == "QUOTE")
        return ContentActionType::QUOTE;
    if (action == "QUIZ")
        return ContentActionType::QUIZ;
    if (action == "NEWS")
        return ContentActionType::NEWS;
    if (action == "PRINT_TEST" || action == "PRINTTEST" || action == "PRINT TEST")
        return ContentActionType::PRINT_TEST;
    if (action == "POKE")
        return ContentActionType::POKE;
    if (action == "USER_MESSAGE" || action == "MESSAGE")
        return ContentActionType::USER_MESSAGE;
    if (action == "UNBIDDEN_INK" || action == "UNBIDDEN INK")
        return ContentActionType::UNBIDDEN_INK;
    if (action == "MEMO1")
        return ContentActionType::MEMO1;
    if (action == "MEMO2")
        return ContentActionType::MEMO2;
    if (action == "MEMO3")
        return ContentActionType::MEMO3;
    if (action == "MEMO4")
        return ContentActionType::MEMO4;

    // Return default for unknown actions
    return ContentActionType::JOKE;
}