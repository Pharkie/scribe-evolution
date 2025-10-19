/**
 * @file logging.h
 * @brief Unified logging system using LogManager for Scribe ESP32 Thermal Printer
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

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <config/config.h>
#include "LogManager.h"
#include "utils/time_utils.h"

/**
 * @file logging.h
 * @brief Centralized logging system for Scribe thermal printer
 *
 * Provides configurable logging to multiple outputs:
 * - Serial console (via LogManager single-writer queue)
 * - LittleFS file
 * - MQTT topic
 * - BetterStack telemetry
 */

// External MQTT client reference
extern PubSubClient mqttClient;

/**
 * @brief Log message to file
 */
void logToFile(const char *message);

/**
 * @brief Log message to MQTT topic with optional component metadata
 */
void logToMQTT(const char *message, const char *level, const char *component = "");

/**
 * @brief Log message to BetterStack with component metadata
 */
void logToBetterStack(const char *message, const char *level, const char *component = "");

void rotateLogFile();

/**
 * @brief Get log level string from numeric level
 */
String getLogLevelString(int level);

/**
 * @brief Get safe device owner name (avoids recursion during init)
 */
const char* getSafeDeviceOwner();

// ============================================================================
// Unified Logging Macros - Direct LogManager Integration
// ============================================================================

#define LOG_VERBOSE(component, format, ...) \
    do { \
        if (LOG_LEVEL_VERBOSE <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            char _log_buf[512]; \
            snprintf(_log_buf, sizeof(_log_buf), "[%s] [VERBOSE] V: [%s] [%s] " format, \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
            LogManager::instance().logf("%s\n", _log_buf); \
            if (enableFileLogging) logToFile(_log_buf); \
            if (enableMqttLogging && mqttClient.connected()) logToMQTT(_log_buf, "VERBOSE", component); \
            if (enableBetterStackLogging && WiFi.status() == WL_CONNECTED) logToBetterStack(_log_buf, "VERBOSE", component); \
        } \
    } while (0)

#define LOG_NOTICE(component, format, ...) \
    do { \
        if (LOG_LEVEL_NOTICE <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            char _log_buf[512]; \
            snprintf(_log_buf, sizeof(_log_buf), "[%s] [NOTICE] I: [%s] [%s] " format, \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
            LogManager::instance().logf("%s\n", _log_buf); \
            if (enableFileLogging) logToFile(_log_buf); \
            if (enableMqttLogging && mqttClient.connected()) logToMQTT(_log_buf, "NOTICE", component); \
            if (enableBetterStackLogging && WiFi.status() == WL_CONNECTED) logToBetterStack(_log_buf, "NOTICE", component); \
        } \
    } while (0)

#define LOG_WARNING(component, format, ...) \
    do { \
        if (LOG_LEVEL_WARNING <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            char _log_buf[512]; \
            snprintf(_log_buf, sizeof(_log_buf), "[%s] [WARNING] W: [%s] [%s] " format, \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
            LogManager::instance().logf("%s\n", _log_buf); \
            if (enableFileLogging) logToFile(_log_buf); \
            if (enableMqttLogging && mqttClient.connected()) logToMQTT(_log_buf, "WARNING", component); \
            if (enableBetterStackLogging && WiFi.status() == WL_CONNECTED) logToBetterStack(_log_buf, "WARNING", component); \
        } \
    } while (0)

#define LOG_ERROR(component, format, ...) \
    do { \
        if (LOG_LEVEL_ERROR <= logLevel) { \
            String _timestamp = getFormattedDateTime(); \
            char _log_buf[512]; \
            snprintf(_log_buf, sizeof(_log_buf), "[%s] [ERROR] E: [%s] [%s] " format, \
                     _timestamp.c_str(), getSafeDeviceOwner(), component, ##__VA_ARGS__); \
            LogManager::instance().logf("%s\n", _log_buf); \
            if (enableFileLogging) logToFile(_log_buf); \
            if (enableMqttLogging && mqttClient.connected()) logToMQTT(_log_buf, "ERROR", component); \
            if (enableBetterStackLogging && WiFi.status() == WL_CONNECTED) logToBetterStack(_log_buf, "ERROR", component); \
        } \
    } while (0)

#endif // LOGGING_H
