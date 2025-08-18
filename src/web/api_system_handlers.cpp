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
#include "../content/unbidden_ink.h"
#include "../hardware/hardware_buttons.h"
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

    // === DEVICE INFORMATION ===
    JsonObject device = doc.createNestedObject("device");
    device["owner"] = String(getDeviceOwnerKey());
    device["hostname"] = String(getMdnsHostname());
    device["timezone"] = String(getTimezone());

    // === HARDWARE INFORMATION ===
    JsonObject hardware = doc.createNestedObject("hardware");
    hardware["chip_model"] = ESP.getChipModel();
    hardware["chip_revision"] = ESP.getChipRevision();
    hardware["cpu_frequency_mhz"] = ESP.getCpuFreqMHz();
    hardware["sdk_version"] = ESP.getSdkVersion();

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
    hardware["reset_reason"] = resetReasonStr;

    // Temperature (ESP32-C3 internal sensor)
    float temp = temperatureRead();
    LOG_VERBOSE("WEB", "Raw temperature reading: %.2f°C, isnan: %s, isfinite: %s",
                temp, isnan(temp) ? "true" : "false", isfinite(temp) ? "true" : "false");

    if (isfinite(temp) && temp > -100 && temp < 200) // Very lenient range for debugging
    {
        hardware["temperature"] = temp;
        LOG_VERBOSE("WEB", "Temperature added to JSON: %.2f°C", temp);
    }
    else
    {
        hardware["temperature"] = nullptr; // Explicitly set to null for JSON
        LOG_WARNING("WEB", "Invalid temperature reading filtered out: %.2f°C (isnan: %s, isfinite: %s)",
                    temp, isnan(temp) ? "true" : "false", isfinite(temp) ? "true" : "false");
    }

    // === SYSTEM STATUS ===
    JsonObject system = doc.createNestedObject("system");
    system["uptime_ms"] = millis();

    // Memory information
    JsonObject memory = system.createNestedObject("memory");
    memory["free_heap"] = ESP.getFreeHeap();
    memory["total_heap"] = ESP.getHeapSize();
    memory["used_heap"] = ESP.getHeapSize() - ESP.getFreeHeap();

    // Flash storage breakdown
    JsonObject flash = system.createNestedObject("flash");

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

    // === NETWORK STATUS ===
    JsonObject network = doc.createNestedObject("network");

    // WiFi information
    JsonObject wifi = network.createNestedObject("wifi");
    wifi["connected"] = (WiFi.status() == WL_CONNECTED);
    wifi["ssid"] = WiFi.SSID();
    wifi["ip_address"] = WiFi.localIP().toString();
    wifi["mac_address"] = WiFi.macAddress();
    wifi["signal_strength_dbm"] = WiFi.RSSI();
    wifi["gateway"] = WiFi.gatewayIP().toString();
    wifi["dns"] = WiFi.dnsIP().toString();
    wifi["connect_timeout_ms"] = runtimeConfig.wifiConnectTimeoutMs;

    // MQTT information
    JsonObject mqtt = network.createNestedObject("mqtt");
    mqtt["connected"] = mqttClient.connected();
    mqtt["server"] = runtimeConfig.mqttServer;
    mqtt["port"] = runtimeConfig.mqttPort;
    mqtt["topic"] = String(getLocalPrinterTopic());

    // === FEATURES STATUS ===
    JsonObject features = doc.createNestedObject("features");

    // Unbidden Ink status
    JsonObject unbiddenInk = features.createNestedObject("unbidden_ink");
    // Reload settings from file to ensure we have the latest values
    loadUnbiddenInkSettings();
    UnbiddenInkSettings settings = getCurrentUnbiddenInkSettings();
    unbiddenInk["enabled"] = settings.enabled;
    // Always include configuration values regardless of enabled state
    unbiddenInk["start_hour"] = settings.startHour;
    unbiddenInk["end_hour"] = settings.endHour;
    unbiddenInk["frequency_minutes"] = settings.frequencyMinutes;
    // Only include runtime data when enabled
    if (settings.enabled)
    {
        unbiddenInk["next_message_time"] = getNextUnbiddenInkTime();
    }

    // Hardware buttons configuration
    JsonObject buttons = features.createNestedObject("hardware_buttons");
    buttons["num_buttons"] = numHardwareButtons;
    buttons["debounce_ms"] = buttonDebounceMs;
    buttons["long_press_ms"] = buttonLongPressMs;
    buttons["active_low"] = buttonActiveLow;
    buttons["min_interval_ms"] = buttonMinInterval;
    buttons["max_per_minute"] = buttonMaxPerMinute;
    JsonArray buttonArray = buttons.createNestedArray("buttons");
    // Use existing runtime configuration for button actions
    for (int i = 0; i < numHardwareButtons; i++)
    {
        JsonObject button = buttonArray.createNestedObject();
        button["gpio"] = defaultButtons[i].gpio;
        button["short_endpoint"] = runtimeConfig.buttonShortActions[i];
        button["long_endpoint"] = runtimeConfig.buttonLongActions[i];
        button["short_mqtt_topic"] = runtimeConfig.buttonShortMqttTopics[i];
        button["long_mqtt_topic"] = runtimeConfig.buttonLongMqttTopics[i];
    }

    // Logging configuration
    JsonObject logging = features.createNestedObject("logging");
    logging["level"] = logLevel;
    logging["level_name"] = getLogLevelString(logLevel);
    logging["serial_enabled"] = enableSerialLogging;
    logging["file_enabled"] = enableFileLogging;
    logging["mqtt_enabled"] = enableMQTTLogging;
    logging["betterstack_enabled"] = enableBetterStackLogging;

    // === AVAILABLE ENDPOINTS ===
    JsonObject endpoints = doc.createNestedObject("endpoints");
    addRegisteredRoutesToJson(endpoints);

    // === CONFIGURATION LIMITS ===
    JsonObject config = doc.createNestedObject("configuration");
    config["max_message_chars"] = runtimeConfig.maxCharacters;
    config["max_prompt_chars"] = maxPromptCharacters;

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
