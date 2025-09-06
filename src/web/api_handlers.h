/**
 * @file api_handlers.h
 * @brief Core API endpoint handlers and shared utilities
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <core/shared_types.h>
#include <utils/json_helpers.h>
#include "validation.h"

// Import specialized handler modules
#include "api_system_handlers.h"
#include "api_config_handlers.h"

#ifdef ENABLE_LEDS
#include "api_led_handlers.h"
#endif

// ========================================
// SHARED API UTILITIES
// ========================================
// Note: Response utility functions are defined in json_helpers.h

#endif // API_HANDLERS_H
