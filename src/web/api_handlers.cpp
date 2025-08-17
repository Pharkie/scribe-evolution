/**
 * @file api_handlers.cpp
 * @brief Core API endpoint handlers and shared utilities
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_handlers.h"
#include "validation.h"
#include "../utils/json_helpers.h"
#include "../core/config.h"
#include "../core/logging.h"

// ========================================
// SHARED API UTILITIES
// ========================================
// Note: Core response functions (sendErrorResponse, sendSuccessResponse, sendRateLimitResponse)
// are implemented in json_helpers.cpp and included via json_helpers.h
