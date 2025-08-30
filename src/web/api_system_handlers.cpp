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
#include <esp_task_wdt.h>

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

    // Pages and endpoints moved to separate /api/routes endpoint

    // Serialize and send
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleRoutes(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "handleRoutes() called - listing pages and API endpoints");

    DynamicJsonDocument doc(8192); // Large buffer for all routes
    JsonObject routes = doc.to<JsonObject>();

    // === PAGES AND ENDPOINTS ===
    addRegisteredRoutesToJson(routes);

    // Serialize and send
    String response;
    serializeJson(doc, response);
    
    AsyncWebServerResponse *res = request->beginResponse(200, "application/json", response);
    res->addHeader("Access-Control-Allow-Origin", "*");
    request->send(res);
    
    LOG_VERBOSE("WEB", "Routes data sent (%zu bytes)", response.length());
}

void handleMQTTSend(AsyncWebServerRequest *request)
{
    if (isRateLimited())
    {
        sendRateLimitResponse(request);
        return;
    }

    if (!isMQTTEnabled())
    {
        sendErrorResponse(request, 503, "MQTT is disabled");
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
    
    // Add sender information (device owner)
    const RuntimeConfig &config = getRuntimeConfig();
    if (config.deviceOwner.length() > 0)
    {
        payloadDoc["sender"] = config.deviceOwner;
    }

    String payload;
    serializeJson(payloadDoc, payload);

    // Publish to MQTT
    if (mqttClient.publish(topic.c_str(), payload.c_str()))
    {
        LOG_VERBOSE("WEB", "MQTT message sent to topic: %s (%d characters)", topic.c_str(), message.length());
        request->send(200, "application/json", "{\"message\":\"Message scribed via MQTT\"}");
    }
    else
    {
        LOG_ERROR("WEB", "Failed to send MQTT message to topic: %s", topic.c_str());
        sendErrorResponse(request, 500, "Failed to send MQTT message - broker error");
    }
}

void handleWiFiScan(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "WiFi scan requested");

    // Only allow GET method
    if (request->method() != HTTP_GET)
    {
        sendErrorResponse(request, 405, "Method not allowed");
        return;
    }

    // Debug current WiFi mode and status
    wifi_mode_t mode = WiFi.getMode();
    wl_status_t status = WiFi.status();
    LOG_NOTICE("WEB", "WiFi scan debug - Mode: %d, Status: %d, IP: %s", mode, status, WiFi.localIP().toString().c_str());
    
    // Check if we're in AP mode - scanning might not work properly
    if (mode == WIFI_AP)
    {
        LOG_WARNING("WEB", "WiFi scan requested while in AP mode - may fail");
    }

    // Start async WiFi scan - fail fast if it doesn't work
    LOG_NOTICE("WEB", "Starting async WiFi scan");
    
    // Start async scan - returns immediately
    int scanResult = WiFi.scanNetworks(true); // true = async mode
    LOG_NOTICE("WEB", "WiFi.scanNetworks(async=true) returned: %d", scanResult);
    
    // Fail fast if async scan initiation failed
    if (scanResult == WIFI_SCAN_FAILED) {
        LOG_ERROR("WEB", "Async WiFi scan failed to start - Mode: %d, Status: %d", mode, status);
        sendErrorResponse(request, 500, "WiFi scanning not available - async scan initialization failed");
        return;
    }
    
    // Wait for scan completion with watchdog feeding
    int networkCount = WIFI_SCAN_RUNNING;
    unsigned long scanStartTime = millis();
    const unsigned long PARTIAL_RESULT_TIME = 10000; // Return partial results after 10s
    const unsigned long MAX_SCAN_TIME = 15000; // Hard timeout at 15s
    bool partialResults = false;
    
    while (networkCount == WIFI_SCAN_RUNNING) {
        networkCount = WiFi.scanComplete();
        unsigned long elapsed = millis() - scanStartTime;
        
        // Return partial results after 10 seconds
        if (elapsed > PARTIAL_RESULT_TIME && networkCount == WIFI_SCAN_RUNNING) {
            LOG_NOTICE("WEB", "WiFi scan taking longer than %ds, checking for partial results", PARTIAL_RESULT_TIME / 1000);
            networkCount = WiFi.scanComplete(); // Get whatever we have so far
            if (networkCount == WIFI_SCAN_RUNNING) {
                networkCount = 0; // If still running, return empty results rather than error
            }
            partialResults = true;
            break;
        }
        
        // Hard timeout
        if (elapsed > MAX_SCAN_TIME) {
            LOG_WARNING("WEB", "WiFi scan hard timeout after %d seconds", MAX_SCAN_TIME / 1000);
            networkCount = WiFi.scanComplete(); // Try to get partial results
            if (networkCount == WIFI_SCAN_RUNNING) {
                WiFi.scanDelete(); // Clean up if still running
                sendErrorResponse(request, 500, "WiFi scan timeout - no results available");
                return;
            }
            partialResults = true;
            break;
        }
        
        // Feed watchdog and yield to other tasks
        esp_task_wdt_reset();
        delay(500); // Poll every 500ms - balance responsiveness vs efficiency
    }

    if (networkCount == WIFI_SCAN_FAILED)
    {
        LOG_ERROR("WEB", "Async WiFi scan failed - Mode: %d, Status: %d", mode, status);
        
        String errorMsg;
        if (mode == WIFI_AP) {
            errorMsg = "WiFi scan failed - scanning not supported in AP mode";
        } else {
            errorMsg = "WiFi scan failed - check WiFi radio state";
        }
        
        sendErrorResponse(request, 500, errorMsg);
        return;
    }

    if (networkCount == 0)
    {
        LOG_WARNING("WEB", "No networks found - this may be due to AP mode limitations");
    }

    // Create JSON response with scanned networks
    DynamicJsonDocument doc(2048);
    doc["count"] = networkCount;
    doc["partial"] = partialResults;

    JsonArray networks = doc.createNestedArray("networks");

    for (int i = 0; i < networkCount; i++)
    {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["channel"] = WiFi.channel(i);
        network["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

        // Convert encryption type to readable string
        String encType = "Unknown";
        switch (WiFi.encryptionType(i))
        {
        case WIFI_AUTH_OPEN:
            encType = "Open";
            break;
        case WIFI_AUTH_WEP:
            encType = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            encType = "WPA";
            break;
        case WIFI_AUTH_WPA2_PSK:
            encType = "WPA2";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            encType = "WPA/WPA2";
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            encType = "WPA2 Enterprise";
            break;
        case WIFI_AUTH_WPA3_PSK:
            encType = "WPA3";
            break;
        default:
            encType = "Unknown";
            break;
        }
        network["encryption"] = encType;
    }

    // Convert signal strength to descriptive labels for UI display
    for (int i = 0; i < networkCount; i++)
    {
        JsonObject network = networks[i];
        int rssi = network["rssi"];

        // Convert RSSI to descriptive strength labels
        String signalStrength;
        if (rssi >= -50)
        {
            signalStrength = "Strong";
        }
        else if (rssi >= -60)
        {
            signalStrength = "Good";
        }
        else if (rssi >= -70)
        {
            signalStrength = "Fair";
        }
        else
        {
            signalStrength = "Weak";
        }

        network["signal_strength"] = signalStrength;
    }

    String response;
    serializeJson(doc, response);

    if (partialResults) {
        LOG_NOTICE("WEB", "WiFi scan returned partial results - found %d networks", networkCount);
    } else {
        LOG_VERBOSE("WEB", "WiFi scan completed - found %d networks", networkCount);
    }
    request->send(200, "application/json", response);

    // Clean up scan results to free memory
    WiFi.scanDelete();
}
