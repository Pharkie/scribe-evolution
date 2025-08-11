/**
 * @file logging.h
 * @brief Multi-output logging system for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 * Based on the original Project Scribe by UrbanCircles.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "config.h"

/**
 * @file logging.h
 * @brief Centralized logging system for Scribe thermal printer
 *
 * Provides configurable logging to multiple outputs:
 * - Serial console
 * - LittleFS file
 * - MQTT topic
 * - BetterStack telemetry
 */

// External MQTT client reference
extern PubSubClient mqttClient;

/**
 * @brief Initialize the logging system with configured outputs
 */
void setupLogging();

/**
 * @brief ESP32-style component logging functions with structured logging support
 */
void structuredLog(const char *component, int level, const char *format, ...);

#define LOG_NOTICE(component, format, ...) structuredLog(component, LOG_LEVEL_NOTICE, format, ##__VA_ARGS__)
#define LOG_ERROR(component, format, ...) structuredLog(component, LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#define LOG_WARNING(component, format, ...) structuredLog(component, LOG_LEVEL_WARNING, format, ##__VA_ARGS__)
#define LOG_INFO(component, format, ...) structuredLog(component, LOG_LEVEL_NOTICE, format, ##__VA_ARGS__)
#define LOG_VERBOSE(component, format, ...) structuredLog(component, LOG_LEVEL_VERBOSE, format, ##__VA_ARGS__)

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

// Custom print class for multi-output logging
class MultiOutputPrint : public Print
{
public:
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
};

#endif // LOGGING_H
