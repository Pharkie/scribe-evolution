/**
 * @file api_nvs_handlers.h
 * @brief NVS debugging and diagnostic API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_NVS_HANDLERS_H
#define API_NVS_HANDLERS_H

#include <ESPAsyncWebServer.h>

/**
 * @brief Handle NVS dump request - provides complete raw NVS storage dump
 *
 * Endpoint: GET /api/nvs-dump
 *
 * Returns comprehensive JSON containing:
 * - All known NVS keys with values, types, and descriptions
 * - Validation status indicators (✅ valid, ⚠️ corrected, ❌ invalid/missing)
 * - Secret value redaction for sensitive data
 * - Summary statistics (total, valid, corrected, invalid keys)
 *
 * @param request The HTTP request object
 */
void handleNVSDump(AsyncWebServerRequest *request);

#endif // API_NVS_HANDLERS_H
