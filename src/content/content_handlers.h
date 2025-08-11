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
#include <PubSubClient.h>
#include "../core/shared_types.h"

// External variable reference
extern Message currentMessage;

// ========================================
// CONTENT GENERATION HANDLERS
// ========================================

/**
 * @brief Handle riddle content generation request
 */
void handleRiddle();

/**
 * @brief Handle joke content generation request
 */
void handleJoke();

/**
 * @brief Handle quote content generation request
 */
void handleQuote();

/**
 * @brief Handle quiz content generation request
 */
void handleQuiz();

/**
 * @brief Handle poke content generation request
 */
void handlePoke();

/**
 * @brief Handle Unbidden Ink content generation request
 */
void handleUnbiddenInk();

/**
 * @brief Handle print test request
 */
void handlePrintTest();

/**
 * @brief Handle user message content generation (adds MESSAGE prefix)
 */
void handleUserMessage();

/**
 * @brief Handle content printing (generic handler for any pre-formatted content)
 * @details Unified endpoint for printing content locally or routing via MQTT.
 *          Content should already have action headers (MESSAGE, JOKE, etc.)
 */
void handlePrintContent();

// ========================================
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
String formatContentWithHeader(const String &action, const String &content, const String &sender);

/**
 * @brief Load print test content from filesystem
 * @return String containing the test content
 */
String loadPrintTestContent();

#endif // CONTENT_HANDLERS_H
