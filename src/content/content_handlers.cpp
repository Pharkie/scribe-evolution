/**
 * @file content_handlers.cpp
 * @brief Implementation of content generation request handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "content_handlers.h"
#include "memo_handler.h"
#include "../web/validation.h"
#include "../web/web_server.h"
#include "../core/config.h"
#include "../core/config_loader.h"
#include "../core/config_utils.h"
#include "../core/nvs_keys.h"
#include "../core/logging.h"
#include "../utils/time_utils.h"
#include "../utils/json_helpers.h"
#include "../utils/content_actions.h"
#include "content_generators.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>

// External declarations
extern AsyncWebServer server;
extern PubSubClient mqttClient;
extern String getFormattedDateTime();
extern String formatCustomDate(String customDate);

// ========================================
// CONTENT GENERATION HANDLERS
// ========================================

// Content type enumeration for unified handler
enum ContentType
{
    RIDDLE,
    JOKE,
    QUOTE,
    QUIZ,
    PRINT_TEST,
    POKE,
    USER_MESSAGE,
    NEWS,
    MEMO
};

/**
 * @brief Unified content generation handler using shared business logic
 * @param contentType The type of content to generate
 */
void handleContentGeneration(AsyncWebServerRequest *request, ContentType contentType)
{
    // Convert ContentType to ContentActionType
    ContentActionType actionType;
    const char *typeName;
    switch (contentType)
    {
    case RIDDLE:
        actionType = ContentActionType::RIDDLE;
        typeName = "riddle";
        break;
    case JOKE:
        actionType = ContentActionType::JOKE;
        typeName = "joke";
        break;
    case QUOTE:
        actionType = ContentActionType::QUOTE;
        typeName = "quote";
        break;
    case QUIZ:
        actionType = ContentActionType::QUIZ;
        typeName = "quiz";
        break;
    case PRINT_TEST:
        actionType = ContentActionType::PRINT_TEST;
        typeName = "print test";
        break;
    case POKE:
        actionType = ContentActionType::POKE;
        typeName = "poke";
        break;
    case USER_MESSAGE:
        actionType = ContentActionType::USER_MESSAGE;
        typeName = "user message";
        break;
    case NEWS:
        actionType = ContentActionType::NEWS;
        typeName = "news";
        break;
    default:
        sendValidationError(request, ValidationResult(false, "Unknown content type"));
        return;
    }

    LOG_VERBOSE("WEB", "handle%s() called", typeName);

    // Note: Content generation endpoints are exempt from rate limiting
    // since they only generate content and don't perform actions.
    // Rate limiting is applied to the actual delivery endpoints (/print-local, /print-mqtt)

    // Get target parameter and custom data from query parameters
    String target = "local-direct"; // Default
    String customData = "";
    
    // Get target from query parameter
    if (request->hasParam("target"))
    {
        target = request->getParam("target")->value();
    }
    
    // Handle user message custom data from query parameter
    if (contentType == USER_MESSAGE)
    {
        if (!request->hasParam("message"))
        {
            sendValidationError(request, ValidationResult(false, "Missing required query parameter 'message'"));
            return;
        }
        
        customData = request->getParam("message")->value();
        
        // Validate message content
        ValidationResult messageValidation = validateMessage(customData);
        if (!messageValidation.isValid)
        {
            LOG_WARNING("WEB", "User message validation failed: %s", messageValidation.errorMessage.c_str());
            sendValidationError(request, messageValidation);
            return;
        }
    }

    // Determine if this is for MQTT (needs sender info) or local (no sender)
    bool isForMQTT = (target != "local-direct");
    String sender = "";
    
    if (isForMQTT)
    {
        if (actionType == ContentActionType::USER_MESSAGE)
        {
            // For MQTT USER_MESSAGE, use device owner name for display
            const RuntimeConfig &config = getRuntimeConfig();
            sender = config.deviceOwner;
        }
        else
        {
            sender = String(getDeviceOwnerKey());  // MQTT routing key for other content types
        }
    }
    // For local messages, sender stays empty (no "from" in header)

    // Execute content action using shared business logic
    ContentActionResult result = executeContentAction(actionType, customData, sender);

    if (result.success)
    {
        // Always return structured data (header + body separately)
        DynamicJsonDocument doc(2048);
        doc["header"] = result.header;
        doc["body"] = result.body;

        String response;
        serializeJson(doc, response);

        request->send(200, "application/json", response);
        LOG_VERBOSE("WEB", "%s content generated successfully for target: %s", typeName, target.c_str());
    }
    else
    {
        DynamicJsonDocument errorResponse(256);
        errorResponse["error"] = result.errorMessage.length() > 0 ? result.errorMessage : 
                                 String("Failed to generate ") + typeName + " content";

        String errorString;
        serializeJson(errorResponse, errorString);
        request->send(500, "application/json", errorString);
        LOG_ERROR("WEB", "Failed to generate %s content: %s", typeName, result.errorMessage.c_str());
    }
}

// Individual handler functions (simple wrappers)
void handleRiddle(AsyncWebServerRequest *request) { handleContentGeneration(request, RIDDLE); }
void handleJoke(AsyncWebServerRequest *request) { handleContentGeneration(request, JOKE); }
void handleQuote(AsyncWebServerRequest *request) { handleContentGeneration(request, QUOTE); }
void handleQuiz(AsyncWebServerRequest *request) { handleContentGeneration(request, QUIZ); }
void handlePrintTest(AsyncWebServerRequest *request) { handleContentGeneration(request, PRINT_TEST); }
void handlePoke(AsyncWebServerRequest *request) { handleContentGeneration(request, POKE); }
void handleNews(AsyncWebServerRequest *request) { handleContentGeneration(request, NEWS); }
void handleUserMessage(AsyncWebServerRequest *request) { handleContentGeneration(request, USER_MESSAGE); }

void handleUnbiddenInk(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "handleUnbiddenInk() called");

    // Check if there's a custom prompt in query parameters
    String customPrompt = "";
    if (request->hasParam("prompt"))
    {
        customPrompt = request->getParam("prompt")->value();
        customPrompt.trim();
        LOG_VERBOSE("WEB", "Using custom prompt from query parameter: %s", customPrompt.c_str());
    }

    // Execute content action using shared business logic
    ContentActionResult result = executeContentAction(ContentActionType::UNBIDDEN_INK, customPrompt, "");

    if (result.success)
    {
        // Return JSON response in structured format (header + body separately)
        DynamicJsonDocument responseDoc(2048);
        responseDoc["header"] = result.header;
        responseDoc["body"] = result.body;

        String response;
        serializeJson(responseDoc, response);
        request->send(200, "application/json", response);

        LOG_VERBOSE("WEB", "Unbidden Ink content generated and returned");
    }
    else
    {
        // Return JSON error response in the same format as other content endpoints
        DynamicJsonDocument errorDoc(256);
        errorDoc["error"] = result.errorMessage.length() > 0 ? result.errorMessage : 
                           "Failed to generate Unbidden Ink content";

        String errorResponse;
        serializeJson(errorDoc, errorResponse);
        request->send(500, "application/json", errorResponse);
        LOG_ERROR("WEB", "Failed to generate Unbidden Ink content: %s", result.errorMessage.c_str());
    }
}

bool generateAndQueueUnbiddenInk()
{
    LOG_VERBOSE("UNBIDDENINK", "generateAndQueueUnbiddenInk() called");
    return executeAndQueueContent(ContentActionType::UNBIDDEN_INK);
}

// ========================================
// INTERNAL CONTENT GENERATION FUNCTIONS
// ========================================

bool generateAndQueueRiddle()
{
    return executeAndQueueContent(ContentActionType::RIDDLE);
}

bool generateAndQueueJoke()
{
    return executeAndQueueContent(ContentActionType::JOKE);
}

bool generateAndQueueQuote()
{
    return executeAndQueueContent(ContentActionType::QUOTE);
}

bool generateAndQueueQuiz()
{
    return executeAndQueueContent(ContentActionType::QUIZ);
}

bool generateAndQueuePrintTest()
{
    return executeAndQueueContent(ContentActionType::PRINT_TEST);
}

bool generateAndQueueNews()
{
    return executeAndQueueContent(ContentActionType::NEWS);
}

void handlePrintContent(AsyncWebServerRequest *request)
{
    if (isRateLimited())
    {
        sendRateLimitResponse(request);
        return;
    }

    // Get and validate JSON body
    extern String getRequestBody(AsyncWebServerRequest * request);
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "No JSON body provided"));
        return;
    }

    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        sendValidationError(request, ValidationResult(false, "Invalid JSON format: " + String(error.c_str())));
        return;
    }

    // Validate required message field
    if (!doc.containsKey("message"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required field 'message' in JSON"));
        return;
    }

    String message = doc["message"].as<String>();
    String source = doc.containsKey("source") ? doc["source"].as<String>() : "local-direct";

    // Debug: Log message details
    LOG_VERBOSE("WEB", "Received message: length=%d, content: '%.50s'", message.length(), message.c_str());

    // Validate message content
    ValidationResult messageValidation = validateMessage(message);
    if (!messageValidation.isValid)
    {
        LOG_WARNING("WEB", "Message validation failed: %s", messageValidation.errorMessage.c_str());
        sendValidationError(request, messageValidation);
        return;
    }

    // Set up message data - content should already be formatted with action headers
    currentMessage.message = message;
    currentMessage.timestamp = getFormattedDateTime();

    // Handle routing based on source
    bool isLocalDirect = (strcmp(source.c_str(), "local-direct") == 0);

    if (isLocalDirect)
    {
        // Local direct printing: queue for local printer
        currentMessage.shouldPrintLocally = true;
        LOG_VERBOSE("WEB", "Custom message queued for local direct printing");

        request->send(200);
    }
    else
    {
        // MQTT: send via MQTT, don't print locally
        currentMessage.shouldPrintLocally = false;
        LOG_VERBOSE("WEB", "Custom message will be sent via MQTT to topic: %s", source.c_str());

        // Check MQTT connection
        if (!mqttClient.connected())
        {
            sendErrorResponse(request, 503, "MQTT client not connected");
            return;
        }

        // Create JSON payload for MQTT
        DynamicJsonDocument payloadDoc(jsonDocumentSize);
        payloadDoc["message"] = currentMessage.message;
        payloadDoc["timestamp"] = currentMessage.timestamp;
        
        // Add sender information (device owner)
        const RuntimeConfig &config = getRuntimeConfig();
        if (config.deviceOwner.length() > 0)
        {
            payloadDoc["sender"] = config.deviceOwner;
        }
        String payload;
        serializeJson(payloadDoc, payload);

        // Send via MQTT
        if (mqttClient.publish(source.c_str(), payload.c_str()))
        {
            LOG_VERBOSE("WEB", "Custom message successfully sent via MQTT");
            request->send(200);
        }
        else
        {
            LOG_ERROR("WEB", "Failed to send custom message via MQTT");
            sendErrorResponse(request, 500, "Failed to process message");
        }
    }
}

// ========================================
// UTILITY FUNCTIONS
// ========================================


String loadPrintTestContent()
{
    File file = LittleFS.open("/resources/character-test.txt", "r");
    if (!file)
    {
        return "ASCII: Hello World 123!@#\n\nFailed to load print test file";
    }

    String content = file.readString();
    file.close();
    return content;
}

String generateMemoContent(int memoId)
{
    if (memoId < 1 || memoId > MEMO_COUNT)
    {
        LOG_ERROR("CONTENT", "Invalid memo ID: %d", memoId);
        return "";
    }

    // Get memo content from centralized config system
    const RuntimeConfig &config = getRuntimeConfig();
    String memoContent = config.memos[memoId - 1];
    
    if (memoContent.isEmpty())
    {
        LOG_ERROR("CONTENT", "Memo %d is empty", memoId);
        return "";
    }

    // Return raw template - placeholders will be expanded at print time
    return memoContent;
}

bool generateAndQueueMemo(int memoId)
{
    if (memoId < 1 || memoId > MEMO_COUNT)
    {
        LOG_ERROR("CONTENT", "Invalid memo ID: %d", memoId);
        return false;
    }

    // Get memo content from centralized config system
    const RuntimeConfig &config = getRuntimeConfig();
    String memoContent = config.memos[memoId - 1];
    
    if (memoContent.isEmpty())
    {
        LOG_ERROR("CONTENT", "Memo %d is empty", memoId);
        return false;
    }

    // Queue raw template for printing - placeholders will be expanded at print time
    currentMessage.message = memoContent;
    currentMessage.timestamp = getFormattedDateTime();
    currentMessage.hasMessage = true;

    LOG_NOTICE("CONTENT", "Memo %d queued for printing: %s", memoId, memoContent.c_str());
    return true;
}
