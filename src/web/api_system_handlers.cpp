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
#include <config/config.h>
#include <core/config_loader.h>
#include <core/config_utils.h>
#include <core/logging.h>
#include <core/network.h>
#include <core/mqtt_handler.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
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

    JsonDocument doc;

    // === MICROCONTROLLER SECTION ===
    JsonObject microcontroller = doc["microcontroller"].to<JsonObject>();

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
    JsonObject memory = microcontroller["memory"].to<JsonObject>();
    memory["free_heap"] = ESP.getFreeHeap();
    memory["total_heap"] = ESP.getHeapSize();
    memory["used_heap"] = ESP.getHeapSize() - ESP.getFreeHeap();

    // Flash storage breakdown
    JsonObject flash = microcontroller["flash"].to<JsonObject>();

    // Total flash chip size (4MB on ESP32-C3)
    uint32_t totalFlashSize = ESP.getFlashChipSize();
    flash["total_chip_size"] = totalFlashSize;

    // App partition (firmware)
    JsonObject app_partition = flash["app_partition"].to<JsonObject>();
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
    JsonObject filesystem = flash["filesystem"].to<JsonObject>();
    filesystem["used"] = usedBytes;
    filesystem["free"] = totalBytes - usedBytes;
    filesystem["total"] = totalBytes;
    filesystem["percent_of_total_flash"] = (totalBytes * 100) / totalFlashSize;

    // === LOGGING CONFIGURATION ===
    JsonObject logging = doc["logging"].to<JsonObject>();
    logging["level"] = logLevel;
    logging["level_name"] = getLogLevelString(logLevel);

    // Pages and endpoints moved to separate /api/routes endpoint

    // Serialize and send
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void handleRoutes(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "handleRoutes() called - listing pages and API endpoints");

    // Use heap allocation for large JSON documents to prevent stack overflow
    std::unique_ptr<JsonDocument> doc(new JsonDocument());
    JsonObject routes = doc->to<JsonObject>();

    // === PAGES AND ENDPOINTS ===
    addRegisteredRoutesToJson(routes);

    // Serialize and send
    String response;
    serializeJson(*doc, response);
    
    AsyncWebServerResponse *res = request->beginResponse(200, "application/json", response);
    res->addHeader("Access-Control-Allow-Origin", "*");
    request->send(res);
    
    LOG_VERBOSE("WEB", "Routes data sent (%zu bytes)", response.length());
}

void handlePrintMQTT(AsyncWebServerRequest *request)
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
    
    if (!MQTTManager::instance().isConnected())
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

    // Validate JSON structure - only support structured format (header+body)
    const char *requiredFields[] = {"topic", "header", "body"};
    ValidationResult jsonValidation = validateJSON(body, requiredFields, 3);
    if (!jsonValidation.isValid)
    {
        sendValidationError(request, jsonValidation);
        return;
    }

    // Parse the JSON (we know it's valid now)
    JsonDocument doc;
    deserializeJson(doc, body);

    String topic = doc["topic"].as<String>();
    
    // Validate MQTT topic
    ValidationResult topicValidation = validateMQTTTopic(topic);
    if (!topicValidation.isValid)
    {
        sendValidationError(request, topicValidation);
        return;
    }

    String header = doc["header"].as<String>();
    String bodyContent = doc["body"].as<String>();
    
    // Validate content
    ValidationResult headerValidation = validateMessage(header);
    ValidationResult bodyValidation = validateMessage(bodyContent);
    if (!headerValidation.isValid) {
        sendValidationError(request, headerValidation);
        return;
    }
    if (!bodyValidation.isValid) {
        sendValidationError(request, bodyValidation);
        return;
    }
    
    // Use centralized MQTT publishing function
    bool success = publishMQTTMessage(topic, header, bodyContent);
    
    if (success) {
        LOG_VERBOSE("WEB", "MQTT message sent via centralized function to topic: %s", topic.c_str());
        request->send(200);
    } else {
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

    // Feed watchdog before scan (scan can take ~1-2s with tuned params)
    esp_task_wdt_reset();
    
    // Tuned synchronous scan to reduce disruption in AP-STA mode:
    // - passive=true (less active probing)
    // - max_ms_per_chan=120 to keep scan quick across channels
    LOG_VERBOSE("WEB", "Starting WiFi scan (passive, 120ms/channel)");
    int networkCount = WiFi.scanNetworks(false, false, true, 120);
    LOG_VERBOSE("WEB", "WiFi scan completed, found %d networks", networkCount);
    
    // Feed watchdog after scan completes
    esp_task_wdt_reset();

    if (networkCount == WIFI_SCAN_FAILED)
    {
        LOG_ERROR("WEB", "WiFi scan failed");
        sendErrorResponse(request, 500, "WiFi scan failed");
        return;
    }

    if (networkCount == 0)
    {
        LOG_WARNING("WEB", "No networks found");
    }

    // Create JSON response with scanned networks
    JsonDocument doc;
    doc["count"] = networkCount;

    JsonArray networks = doc["networks"].to<JsonArray>();

    for (int i = 0; i < networkCount; i++)
    {
        JsonObject network = networks.add<JsonObject>();
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

    LOG_VERBOSE("WEB", "WiFi scan response sent - found %d networks", networkCount);
    request->send(200, "application/json", response);

    // Clean up scan results to free memory
    WiFi.scanDelete();
}

// ========================================
// WiFi Test Endpoint (blocking, AP mode provisioning)
// ========================================

static SemaphoreHandle_t wifiTestMutex = nullptr;
static volatile int lastStaDisconnectReason = 0; // from WiFi event, if available

void handleTestWiFi(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "WiFi test requested");

    if (request->method() != HTTP_POST)
    {
        sendErrorResponse(request, 405, "Method not allowed");
        return;
    }

    // Lazily init mutex
    if (wifiTestMutex == nullptr)
    {
        wifiTestMutex = xSemaphoreCreateMutex();
    }

    // Try to take the mutex without waiting
    if (wifiTestMutex && xSemaphoreTake(wifiTestMutex, 0) != pdTRUE)
    {
        sendErrorResponse(request, 409, "Test already running");
        return;
    }

    // Ensure we release the mutex on all paths
    auto releaseMutex = [&]() {
        if (wifiTestMutex)
        {
            xSemaphoreGive(wifiTestMutex);
        }
    };

    // Parse JSON body
    extern String getRequestBody(AsyncWebServerRequest * request);
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        releaseMutex();
        sendErrorResponse(request, 422, "No JSON body provided");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err)
    {
        releaseMutex();
        sendErrorResponse(request, 422, "Invalid JSON payload");
        return;
    }

    String ssid = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();
    if (ssid.length() == 0)
    {
        releaseMutex();
        sendErrorResponse(request, 422, "Invalid payload: ssid is required");
        return;
    }

    // Basic password length validation using config constraint if available
    if (password.length() > maxWifiPasswordLength)
    {
        releaseMutex();
        sendErrorResponse(request, 422, "Invalid payload: password too long");
        return;
    }

    // Install temporary WiFi event handler to classify failures
    lastStaDisconnectReason = 0;
    WiFiEventId_t evtId = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
                                       {
                                           if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
                                           {
                                               lastStaDisconnectReason = info.wifi_sta_disconnected.reason;
                                           }
                                       });

    // Ensure STA test runs while AP stays up (AP_STA expected in AP fallback)
    // Begin connection attempt
    WiFi.begin(ssid.c_str(), password.c_str());

    const unsigned long maxDurationMs = 6500; // keep below watchdog
    const unsigned long start = millis();

    while ((millis() - start) < maxDurationMs && WiFi.status() != WL_CONNECTED)
    {
        esp_task_wdt_reset();
        delay(75);
        yield();
    }

    bool connected = (WiFi.status() == WL_CONNECTED);
    int rssi = 0;
    if (connected)
    {
        rssi = WiFi.RSSI();
        // Disconnect STA only; preserve AP
        WiFi.disconnect();
    }
    else
    {
        // If still not connected, also disconnect to reset state
        WiFi.disconnect();
    }

    // Remove event handler
    WiFi.removeEvent(evtId);

    if (connected)
    {
        JsonDocument resp;
        resp["success"] = true;
        resp["rssi"] = rssi;
        String out;
        serializeJson(resp, out);
        releaseMutex();
        request->send(200, "application/json", out);
        return;
    }

    // Classify error: timeout vs. event reasons
    const char *message = "Association timeout";
    int statusCode = 408;
    if (lastStaDisconnectReason != 0)
    {
        // Map a few common reasons
        switch (lastStaDisconnectReason)
        {
        case 201: // WIFI_REASON_NO_AP_FOUND
            message = "No AP found";
            statusCode = 400;
            break;
        case 202: // WIFI_REASON_AUTH_FAIL (common value; may vary by core)
        case 15:  // WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT
            message = "Authentication failed";
            statusCode = 400;
            break;
        default:
            message = "Network error";
            statusCode = 400;
            break;
        }
    }

    JsonDocument resp;
    resp["success"] = false;
    resp["message"] = message;
    String out;
    serializeJson(resp, out);
    releaseMutex();
    request->send(statusCode, "application/json", out);
}
