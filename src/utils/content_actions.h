/**
 * @file content_actions.h
 * @brief Shared business logic for content generation actions
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file provides unified content generation functionality following the DRY principle.
 * Both web API handlers and hardware button handlers use these shared utilities.
 */

#ifndef CONTENT_ACTIONS_H
#define CONTENT_ACTIONS_H

#include <Arduino.h>

/**
 * @brief Result of a content action operation
 */
struct ContentActionResult
{
    bool success;        // Whether the action succeeded
    String header;       // Content header (e.g., "JOKE", "RIDDLE", "MEMO 1")
    String body;         // Content body (the actual joke, riddle text, etc.)
    String errorMessage; // Error message if failed

    ContentActionResult() : success(false), header(""), body(""), errorMessage("") {}
    ContentActionResult(bool s, const String &h, const String &b, const String &e = "")
        : success(s), header(h), body(b), errorMessage(e) {}
    
    // Backward compatibility: get formatted content
    String getFormattedContent() const {
        if (!success || header.length() == 0 || body.length() == 0) {
            return "";
        }
        return header + "\n\n" + body;
    }
};

/**
 * @brief Supported content action types
 */
enum class ContentActionType
{
    JOKE,
    RIDDLE,
    QUOTE,
    QUIZ,
    NEWS,
    PRINT_TEST,
    POKE,
    USER_MESSAGE,
    UNBIDDEN_INK,
    MEMO1,
    MEMO2,
    MEMO3,
    MEMO4
};

/**
 * @brief Execute a content action and return formatted result
 * @param actionType The type of content to generate
 * @param customData Optional custom data (e.g., for user messages)
 * @param sender Optional sender name (for MQTT formatting)
 * @return ContentActionResult with success status and formatted content
 */
ContentActionResult executeContentAction(ContentActionType actionType,
                                         const String &customData = "",
                                         const String &sender = "");

/**
 * @brief Execute a content action with custom timeout
 * @param actionType The type of content to generate
 * @param customData Optional custom data (e.g., for user messages)
 * @param sender Optional sender name (for MQTT formatting)
 * @param timeoutMs Custom timeout in milliseconds for HTTP operations
 * @return ContentActionResult with success status and formatted content
 */
ContentActionResult executeContentActionWithTimeout(ContentActionType actionType,
                                                    const String &customData = "",
                                                    const String &sender = "",
                                                    int timeoutMs = 5000);

/**
 * @brief Queue content for local printing (sets currentMessage)
 * @param result The content action result to queue
 * @return true if content was queued successfully
 */
bool queueContentForPrinting(const ContentActionResult &result);

/**
 * @brief Execute content action and queue for immediate printing
 * @param actionType The type of content to generate
 * @param customData Optional custom data (e.g., for user messages)
 * @return true if content was generated and queued successfully
 */
bool executeAndQueueContent(ContentActionType actionType, const String &customData = "");

/**
 * @brief Convert endpoint path to content action type
 * @param endpoint The API endpoint path (e.g., "/api/joke")
 * @return ContentActionType or nullopt if endpoint not recognized
 */
ContentActionType endpointToActionType(const String &endpoint);

/**
 * @brief Convert content action type to action name string
 * @param actionType The content action type
 * @return String action name (e.g., "JOKE", "RIDDLE")
 */
String actionTypeToString(ContentActionType actionType);

/**
 * @brief Convert action type string to content action type enum
 * @param actionString The action string (e.g., "JOKE", "RIDDLE")
 * @return ContentActionType or JOKE as default if not recognized
 */
ContentActionType stringToActionType(const String &actionString);

#endif // CONTENT_ACTIONS_H