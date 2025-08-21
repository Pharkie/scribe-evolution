/**
 * @file api_system_handlers.cpp
 * @brief System and diagnostics API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_system_handlers.h"
#include "api_handlers.h" // For shared utilities
#include "validation.h"
#include "web_server.h" // For addRegisteredRoutesToJson
#include "../core/config.h"
#include "../core/config_loader.h"
#include "../core/config_utils.h"
#include "../core/logging.h"
#include "../core/network.h"
#include "../core/mqtt_handler.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_ota_ops.h>

// External references
extern PubSubClient mqttClient;

// ========================================
// SYSTEM API HANDLERS
// ========================================

void handleDiagnostics(AsyncWebServerRequest *request)
{
    // Get flash storage information
    size_t totalBytes = 0;
    size_t usedBytes = 0;
    totalBytes = LittleFS.totalBytes();
    usedBytes = LittleFS.usedBytes();

    const RuntimeConfig &runtimeConfig = getRuntimeConfig();

    DynamicJsonDocument doc(4096);

    // === MICROCONTROLLER SECTION ===
    JsonObject microcontroller = doc.createNestedObject("microcontroller");

    // Hardware information
    microcontroller["chip_model"] = ESP.getChipModel();
    microcontroller["chip_revision"] = ESP.getChipRevision();
    microcontroller["cpu_frequency_mhz"] = ESP.getCpuFreqMHz();
    microcontroller["sdk_version"] = ESP.getSdkVersion();

    // Reset reason
    esp_reset_reason_t resetReason = esp_reset_reason();
    const char *resetReasonStr = "Unknown";
    switch (resetReason)
    {
    case ESP_RST_POWERON:
        resetReasonStr = "Power-on";
        break;
    case ESP_RST_EXT:
        resetReasonStr = "External reset";
        break;
    case ESP_RST_SW:
        resetReasonStr = "Software reset";
        break;
    case ESP_RST_PANIC:
        resetReasonStr = "Panic/exception";
        break;
    case ESP_RST_INT_WDT:
        resetReasonStr = "Interrupt watchdog";
        break;
    case ESP_RST_TASK_WDT:
        resetReasonStr = "Task watchdog";
        break;
    case ESP_RST_WDT:
        resetReasonStr = "Other watchdog";
        break;
    case ESP_RST_DEEPSLEEP:
        resetReasonStr = "Deep sleep";
        break;
    case ESP_RST_BROWNOUT:
        resetReasonStr = "Brownout";
        break;
    case ESP_RST_SDIO:
        resetReasonStr = "SDIO reset";
        break;
    default:
        resetReasonStr = "Unknown";
        break;
    }
    microcontroller["reset_reason"] = resetReasonStr;

    // Temperature (ESP32-C3 internal sensor)
    float temp = temperatureRead();
    LOG_VERBOSE("WEB", "Raw temperature reading: %.2f°C, isnan: %s, isfinite: %s",
                temp, isnan(temp) ? "true" : "false", isfinite(temp) ? "true" : "false");

    if (isfinite(temp) && temp > -100 && temp < 200) // Very lenient range for debugging
    {
        microcontroller["temperature"] = temp;
        LOG_VERBOSE("WEB", "Temperature added to JSON: %.2f°C", temp);
    }
    else
    {
        microcontroller["temperature"] = nullptr; // Explicitly set to null for JSON
        LOG_WARNING("WEB", "Invalid temperature reading filtered out: %.2f°C (isnan: %s, isfinite: %s)",
                    temp, isnan(temp) ? "true" : "false", isfinite(temp) ? "true" : "false");
    }

    // System status
    microcontroller["uptime_ms"] = millis();

    // Memory information
    JsonObject memory = microcontroller.createNestedObject("memory");
    memory["free_heap"] = ESP.getFreeHeap();
    memory["total_heap"] = ESP.getHeapSize();
    memory["used_heap"] = ESP.getHeapSize() - ESP.getFreeHeap();

    // Flash storage breakdown
    JsonObject flash = microcontroller.createNestedObject("flash");

    // Total flash chip size (4MB on ESP32-C3)
    uint32_t totalFlashSize = ESP.getFlashChipSize();
    flash["total_chip_size"] = totalFlashSize;

    // App partition (firmware)
    JsonObject app_partition = flash.createNestedObject("app_partition");
    uint32_t appUsed = ESP.getSketchSize();

    // Get accurate partition size using ESP-IDF APIs
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    uint32_t appTotal = 0;
    if (running_partition)
    {
        appTotal = running_partition->size;
    }
    else
    {
        // Fallback to Arduino API if ESP-IDF fails
        appTotal = ESP.getSketchSize() + ESP.getFreeSketchSpace();
    }

    uint32_t appFree = appTotal - appUsed;

    app_partition["used"] = appUsed;
    app_partition["free"] = appFree;
    app_partition["total"] = appTotal;
    app_partition["percent_of_total_flash"] = (appTotal * 100) / totalFlashSize;

    // File system (LittleFS)
    JsonObject filesystem = flash.createNestedObject("filesystem");
    filesystem["used"] = usedBytes;
    filesystem["free"] = totalBytes - usedBytes;
    filesystem["total"] = totalBytes;
    filesystem["percent_of_total_flash"] = (totalBytes * 100) / totalFlashSize;

    // === LOGGING CONFIGURATION ===
    JsonObject logging = doc.createNestedObject("logging");
    logging["level"] = logLevel;
    logging["level_name"] = getLogLevelString(logLevel);
    logging["serial_enabled"] = enableSerialLogging;
    logging["file_enabled"] = enableFileLogging;
    logging["mqtt_enabled"] = enableMQTTLogging;
    logging["betterstack_enabled"] = enableBetterStackLogging;

    // === PAGES AND ENDPOINTS ===
    JsonObject pages_and_endpoints = doc.createNestedObject("pages_and_endpoints");
    addRegisteredRoutesToJson(pages_and_endpoints);

    // Serialize and send
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleMQTTSend(AsyncWebServerRequest *request)
{
    if (isRateLimited())
    {
        sendRateLimitResponse(request);
        return;
    }

    if (!mqttClient.connected())
    {
        sendErrorResponse(request, 503, "MQTT client not connected");
        return;
    }

    // Get and validate JSON body
    extern String getRequestBody(AsyncWebServerRequest * request);
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "No JSON body provided"));
        return;
    }

    // Validate JSON structure
    const char *requiredFields[] = {"topic", "message"};
    ValidationResult jsonValidation = validateJSON(body, requiredFields, 2);
    if (!jsonValidation.isValid)
    {
        sendValidationError(request, jsonValidation);
        return;
    }

    // Parse the JSON (we know it's valid now)
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, body);

    String topic = doc["topic"].as<String>();
    String message = doc["message"].as<String>();

    // Validate MQTT topic
    ValidationResult topicValidation = validateMQTTTopic(topic);
    if (!topicValidation.isValid)
    {
        sendValidationError(request, topicValidation);
        return;
    }

    // Validate message content
    ValidationResult messageValidation = validateMessage(message);
    if (!messageValidation.isValid)
    {
        sendValidationError(request, messageValidation);
        return;
    }

    // Create the MQTT payload as JSON with proper escaping
    DynamicJsonDocument payloadDoc(4096);
    payloadDoc["message"] = message; // ArduinoJson handles escaping automatically

    String payload;
    serializeJson(payloadDoc, payload);

    // Publish to MQTT
    if (mqttClient.publish(topic.c_str(), payload.c_str()))
    {
        LOG_VERBOSE("WEB", "MQTT message sent to topic: %s (%d characters)", topic.c_str(), message.length());
        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Message scribed via MQTT\"}");
    }
    else
    {
        LOG_ERROR("WEB", "Failed to send MQTT message to topic: %s", topic.c_str());
        request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to send MQTT message - broker error\"}");
    }
}
