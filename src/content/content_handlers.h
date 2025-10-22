/**
 * @file content_handlers.h
 * @brief Content generation request handlers (riddle, joke, quote, etc.)
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef CONTENT_HANDLERS_H
#define CONTENT_HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <core/shared_types.h>
#include <web/api_memo_handlers.h>  // For MEMO_COUNT definition

// External variable reference
extern Message currentMessage;

// ========================================
// CONTENT GENERATION HANDLERS
// ========================================

/**
 * @brief Handle riddle content generation request
 */
void handleRiddle(AsyncWebServerRequest* request);

/**
 * @brief Handle joke content generation request
 */
void handleJoke(AsyncWebServerRequest* request);

/**
 * @brief Handle quote content generation request
 */
void handleQuote(AsyncWebServerRequest* request);

/**
 * @brief Handle quiz content generation request
 */
void handleQuiz(AsyncWebServerRequest* request);

/**
 * @brief Handle poke content generation request
 */
void handlePoke(AsyncWebServerRequest* request);

/**
 * @brief Handle news content generation request
 */
void handleNews(AsyncWebServerRequest* request);

/**
 * @brief Handle Unbidden Ink content generation request
 */
void handleUnbiddenInk(AsyncWebServerRequest* request);

/**
 * @brief Generate and queue Unbidden Ink content for internal calls (no request)
 * @return true if content was generated and queued successfully
 */
bool generateAndQueueUnbiddenInk();

// ========================================
// INTERNAL CONTENT GENERATION FUNCTIONS
// ========================================
// These functions are for internal calls (hardware buttons, timers, etc.)
// They don't require request parameters and directly queue content for printing

bool generateAndQueueRiddle();
bool generateAndQueueJoke();
bool generateAndQueueQuote();
bool generateAndQueueQuiz();
bool generateAndQueueNews();

/**
 * @brief Generate memo content without queuing
 * @param memoId The memo ID (1-4) to generate content for
 * @return The expanded memo content or empty string if failed
 */
String generateMemoContent(int memoId);

/**
 * @brief Generate and queue memo content for internal calls (hardware buttons)
 * @param memoId The memo ID (1-4) to generate and queue
 * @return true if content was generated and queued successfully
 */
bool generateAndQueueMemo(int memoId);


/**
 * @brief Handle user message content generation (adds MESSAGE prefix)
 */
void handleUserMessage(AsyncWebServerRequest* request);

/**
 * @brief Unified content generation handler (handles all content types via path parameter)
 * @details Replaces individual handlers: riddle, joke, quote, quiz, news, poke, unbidden-ink, user-message
 * Route: /api/content/:type where type = riddle|joke|quote|quiz|news|poke|unbidden-ink|user-message
 */
void handleContent(AsyncWebServerRequest* request);

/**
 * @brief Handle local content printing (generic handler for any pre-formatted content)
 * @details Endpoint for printing content locally only. For MQTT operations, use /api/print-mqtt.
 *          Content should already have action headers (MESSAGE, JOKE, etc.)
 */
void handlePrintLocal(AsyncWebServerRequest* request);

// ========================================
// ========================================
// UTILITY FUNCTIONS
// ========================================



#endif // CONTENT_HANDLERS_H
