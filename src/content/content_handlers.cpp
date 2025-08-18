/**
 * @file content_handlers.cpp
 * @brief Implementation of content generation request handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "content_handlers.h"
#include "../web/validation.h"
#include "../web/web_server.h"
#include "../core/config.h"
#include "../core/config_utils.h"
#include "../core/logging.h"
#include "../utils/time_utils.h"
#include "../utils/json_helpers.h"
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
    NEWS
};

/**
 * @brief Unified content generation handler
 * @param contentType The type of content to generate
 */
void handleContentGeneration(AsyncWebServerRequest *request, ContentType contentType)
{
    // Determine content type name for logging
    const char *typeName;
    switch (contentType)
    {
    case RIDDLE:
        typeName = "riddle";
        break;
    case JOKE:
        typeName = "joke";
        break;
    case QUOTE:
        typeName = "quote";
        break;
    case QUIZ:
        typeName = "quiz";
        break;
    case PRINT_TEST:
        typeName = "print test";
        break;
    case POKE:
        typeName = "poke";
        break;
    case USER_MESSAGE:
        typeName = "user message";
        break;
    case NEWS:
        typeName = "news";
        break;
    default:
        typeName = "unknown";
        break;
    }

    LOG_VERBOSE("WEB", "handle%s() called", typeName);

    // Note: Content generation endpoints are exempt from rate limiting
    // since they only generate content and don't perform actions.
    // Rate limiting is applied to the actual delivery endpoints (/print-local, /mqtt-send)

    // Get target parameter to determine if sender info should be included
    String body = getRequestBody(request);
    String target = "local-direct"; // Default

    if (body.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, body);
        if (!error && doc.containsKey("target"))
        {
            target = doc["target"].as<String>();
        }
    }

    // Determine if this is for MQTT (needs sender info) or local (no sender)
    bool isForMQTT = (target != "local-direct");
    String sender = isForMQTT ? String(getDeviceOwnerKey()) : "";

    // Generate content based on type
    String content;
    String actionName;
    switch (contentType)
    {
    case RIDDLE:
        content = generateRiddleContent();
        actionName = "RIDDLE";
        break;
    case JOKE:
        content = generateJokeContent();
        actionName = "JOKE";
        break;
    case QUOTE:
        content = generateQuoteContent();
        actionName = "QUOTE";
        break;
    case QUIZ:
        content = generateQuizContent();
        actionName = "QUIZ";
        break;
    case PRINT_TEST:
    {
        String testContent = loadPrintTestContent();
        content = testContent + "\n\n";
        actionName = "TEST PRINT";
    }
    break;
    case POKE:
        content = generatePokeContent();
        actionName = "POKE";
        break;
    case USER_MESSAGE:
    {
        // Get and validate JSON body for user message input
        if (body.length() == 0)
        {
            sendValidationError(request, ValidationResult(false, "No JSON body provided"));
            return;
        }

        // Parse JSON (already parsed above, but parse again for user message field)
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

        String userMessage = doc["message"].as<String>();

        // Validate message content
        ValidationResult messageValidation = validateMessage(userMessage);
        if (!messageValidation.isValid)
        {
            LOG_WARNING("WEB", "User message validation failed: %s", messageValidation.errorMessage.c_str());
            sendValidationError(request, messageValidation);
            return;
        }

        // Use the raw user message directly
        content = userMessage;
        actionName = "MESSAGE";
    }
    break;
    case NEWS:
        content = generateNewsContent();
        actionName = "NEWS";
        break;
    }

    if (content.length() > 0 || actionName == "POKE")
    {
        // Format content with header and appropriate sender info
        String formattedContent = formatContentWithHeader(actionName, content, sender);

        // Return JSON with formatted content
        DynamicJsonDocument doc(2048);
        doc["content"] = formattedContent;

        String response;
        serializeJson(doc, response);

        request->send(200, "application/json", response);
        LOG_VERBOSE("WEB", "%s content generated successfully for target: %s", typeName, target.c_str());
    }
    else
    {
        DynamicJsonDocument errorResponse(256);
        errorResponse["error"] = String("Failed to generate ") + typeName + " content";

        String errorString;
        serializeJson(errorResponse, errorString);
        request->send(500, "application/json", errorString);
        LOG_ERROR("WEB", "Failed to generate %s content", typeName);
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

    // Check if there's a custom prompt in the request body
    String customPrompt = "";
    extern String getRequestBody(AsyncWebServerRequest * request);
    String body = getRequestBody(request);
    if (body.length() > 0)
    {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, body);

        if (!error && doc.containsKey("prompt"))
        {
            customPrompt = doc["prompt"].as<String>();
            customPrompt.trim();
            LOG_VERBOSE("WEB", "Using custom prompt from request: %s", customPrompt.c_str());
        }
    }

    // Generate unbidden ink content (with optional custom prompt)
    String content = generateUnbiddenInkContent(customPrompt);

    if (content.length() > 0)
    {
        // Format content with header to match other endpoints
        String formattedContent = formatContentWithHeader("UNBIDDEN INK", content, "");

        // Return JSON response in the same format as other content endpoints
        DynamicJsonDocument responseDoc(2048);
        responseDoc["content"] = formattedContent;

        String response;
        serializeJson(responseDoc, response);
        request->send(200, "application/json", response);

        LOG_VERBOSE("WEB", "Unbidden Ink content generated and returned");
    }
    else
    {
        // Return JSON error response in the same format as other content endpoints
        DynamicJsonDocument errorDoc(256);
        errorDoc["error"] = "Failed to generate Unbidden Ink content";

        String errorResponse;
        serializeJson(errorDoc, errorResponse);
        request->send(500, "application/json", errorResponse);
        LOG_ERROR("WEB", "Failed to generate Unbidden Ink content");
    }
}

bool generateAndQueueUnbiddenInk()
{
    LOG_VERBOSE("UNBIDDENINK", "generateAndQueueUnbiddenInk() called");

    // Generate unbidden ink content (no custom prompt for timer-based calls)
    String content = generateUnbiddenInkContent("");

    if (content.length() > 0)
    {
        // Format with header (always local, so no sender info)
        String formattedContent = formatContentWithHeader("UNBIDDEN INK", content, "");

        // Set up message for local printing (Unbidden Ink is always local-direct)
        currentMessage.message = formattedContent;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;

        LOG_VERBOSE("UNBIDDENINK", "Unbidden Ink content queued for local direct printing");
        return true;
    }
    else
    {
        LOG_ERROR("UNBIDDENINK", "Failed to generate Unbidden Ink content");
        return false;
    }
}

// ========================================
// INTERNAL CONTENT GENERATION FUNCTIONS
// ========================================

bool generateAndQueueRiddle()
{
    String content = generateRiddleContent();
    if (content.length() > 0)
    {
        String formattedContent = formatContentWithHeader("RIDDLE", content, "");
        currentMessage.message = formattedContent;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;
        LOG_VERBOSE("BUTTON", "Riddle content queued for local printing");
        return true;
    }
    return false;
}

bool generateAndQueueJoke()
{
    String content = generateJokeContent();
    if (content.length() > 0)
    {
        String formattedContent = formatContentWithHeader("JOKE", content, "");
        currentMessage.message = formattedContent;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;
        LOG_VERBOSE("BUTTON", "Joke content queued for local printing");
        return true;
    }
    return false;
}

bool generateAndQueueQuote()
{
    String content = generateQuoteContent();
    if (content.length() > 0)
    {
        String formattedContent = formatContentWithHeader("QUOTE", content, "");
        currentMessage.message = formattedContent;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;
        LOG_VERBOSE("BUTTON", "Quote content queued for local printing");
        return true;
    }
    return false;
}

bool generateAndQueueQuiz()
{
    String content = generateQuizContent();
    if (content.length() > 0)
    {
        String formattedContent = formatContentWithHeader("QUIZ", content, "");
        currentMessage.message = formattedContent;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;
        LOG_VERBOSE("BUTTON", "Quiz content queued for local printing");
        return true;
    }
    return false;
}

bool generateAndQueuePrintTest()
{
    String content = loadPrintTestContent();
    if (content.length() > 0)
    {
        String formattedContent = formatContentWithHeader("PRINT TEST", content, "");
        currentMessage.message = formattedContent;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;
        LOG_VERBOSE("BUTTON", "Print test content queued for local printing");
        return true;
    }
    return false;
}

bool generateAndQueueNews()
{
    String content = generateNewsContent();
    if (content.length() > 0)
    {
        String formattedContent = formatContentWithHeader("NEWS", content, "");
        currentMessage.message = formattedContent;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;
        LOG_VERBOSE("BUTTON", "News content queued for local printing");
        return true;
    }
    return false;
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

        request->send(200, "application/json", "{\"success\":true,\"message\":\"Message processed successfully\"}");
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
        String payload;
        serializeJson(payloadDoc, payload);

        // Send via MQTT
        if (mqttClient.publish(source.c_str(), payload.c_str()))
        {
            LOG_VERBOSE("WEB", "Custom message successfully sent via MQTT");
            sendSuccessResponse(request, "Message processed successfully");
        }
        else
        {
            LOG_ERROR("WEB", "Failed to send custom message via MQTT");
            request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to process message\"}");
        }
    }
}

// ========================================
// UTILITY FUNCTIONS
// ========================================

/**
 * @brief Format content with action header and optional sender info
 * @param action The action name (JOKE, RIDDLE, MESSAGE, etc.)
 * @param content The raw content
 * @param sender Optional sender name (empty for local actions)
 * @return Formatted string with header
 */
String formatContentWithHeader(const String &action, const String &content, const String &sender)
{
    String header = action;
    if (sender.length() > 0)
    {
        header += " from " + sender;
    }
    return header + "\n\n" + content;
}

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
