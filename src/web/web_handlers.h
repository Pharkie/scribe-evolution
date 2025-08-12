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

// ========================================
// STATIC FILE HANDLERS
// ========================================

/**
 * @brief Handle 404 not found requests
 */
void handleNotFound();

// ========================================
// UTILITY FUNCTIONS
// ========================================

/**
 * @brief Helper function to serve files from LittleFS filesystem
 * @param path The file path to serve (e.g., "/html/index.html")
 * @param contentType The MIME content type (e.g., "text/html")
 * @return true if file was served successfully, false otherwise
 */
bool serveFileFromLittleFS(const String &path, const String &contentType);

#endif // WEB_HANDLERS_H
