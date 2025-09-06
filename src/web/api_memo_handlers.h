/**
 * @file api_memo_handlers.h
 * @brief Memo API endpoint handlers for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_MEMO_HANDLERS_H
#define API_MEMO_HANDLERS_H

#include <ESPAsyncWebServer.h>
#include <config/config.h>


/**
 * @brief Handle individual memo retrieval request  
 * @param request The HTTP request
 * 
 * Endpoint: GET /api/memo/{id}
 * Returns processed memo content with placeholders expanded (ready for printing)
 */
void handleMemoGet(AsyncWebServerRequest *request);

/**
 * @brief Handle memo update request
 * @param request The HTTP request
 * 
 * Endpoint: POST /api/memo/{id}
 * Body: JSON with "content" field
 * Updates specific memo content with validation
 */
void handleMemoUpdate(AsyncWebServerRequest *request);


/**
 * @brief Handle all memos update request  
 * @param request The HTTP request
 * 
 * Endpoint: POST /api/memos
 * Body: JSON with "memos" array
 * Updates all memo content with validation
 */
void handleMemosUpdate(AsyncWebServerRequest *request);

#endif // API_MEMO_HANDLERS_H