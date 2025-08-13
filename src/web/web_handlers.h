/**
 * @file web_handlers.h
 * @brief Basic web request handlers (static files, config, etc.)
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef WEB_HANDLERS_H
#define WEB_HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// ========================================
// STATIC FILE HANDLERS
// ========================================

/**
 * @brief Handle 404 not found requests
 */
void handleNotFound(AsyncWebServerRequest *request);

// ========================================
// UTILITY FUNCTIONS
// ========================================

#endif // WEB_HANDLERS_H
