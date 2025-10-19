/**
 * @file logging.h
 * @brief Thread-safe serial logging system using LogManager for Scribe ESP32 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <config/config.h>
#include "LogManager.h"
#include "utils/time_utils.h"

/**
 * @file logging.h
 * @brief Centralized logging system for Scribe thermal printer
 *
 * Thread-safe serial output via LogManager singleton:
 * - FreeRTOS queue for message serialization
 * - Single writer task prevents concurrent Serial corruption
 * - ISR-safe logging support
 * - Non-blocking message enqueueing
 */

/**
 * @brief Get log level string from numeric level
 */
String getLogLevelString(int level);

/**
 * @brief Get safe device owner name (avoids recursion during init)
 */
const char* getSafeDeviceOwner();

// ============================================================================
// Thread-Safe Logging Macros - Serial Output via LogManager
// ============================================================================

#define LOG_VERBOSE(component, format, ...) \
    do { \
        if (LOG_LEVEL_VERBOSE <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            LogManager::instance().logf("[%s] [VERBOSE] V: [%s] [%s] " format "\n", \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_NOTICE(component, format, ...) \
    do { \
        if (LOG_LEVEL_NOTICE <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            LogManager::instance().logf("[%s] [NOTICE] I: [%s] [%s] " format "\n", \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_WARNING(component, format, ...) \
    do { \
        if (LOG_LEVEL_WARNING <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            LogManager::instance().logf("[%s] [WARNING] W: [%s] [%s] " format "\n", \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
        } \
    } while (0)

#define LOG_ERROR(component, format, ...) \
    do { \
        if (LOG_LEVEL_ERROR <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            LogManager::instance().logf("[%s] [ERROR] E: [%s] [%s] " format "\n", \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
        } \
    } while (0)

#endif // LOGGING_H
