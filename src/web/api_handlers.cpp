/**
 * @file api_handlers.cpp
 * @brief Implementation of API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_handlers.h"
#include "validation.h"
#include "../core/config.h"
#include "../core/config_loader.h"
#include "../core/config_utils.h"
#include "../core/logging.h"
#include "../core/mqtt_handler.h"
#include "../core/network.h"
#include "../core/printer_discovery.h"
#include "../hardware/hardware_buttons.h"
#include "../content/unbidden_ink.h"
#include <vector>
#include "../utils/json_helpers.h"
#include <WebServer.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <esp_system.h>

// External declarations
extern WebServer server;
extern PubSubClient mqttClient;

// ========================================
// API ENDPOINT HANDLERS
// ========================================

void handleStatus()
{
    // Get flash storage information
    size_t totalBytes = 0;
    size_t usedBytes = 0;
    totalBytes = LittleFS.totalBytes();
    usedBytes = LittleFS.usedBytes();

    DynamicJsonDocument doc(2048); // Large size for comprehensive data

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
    uint32_t appFree = ESP.getFreeSketchSpace();
    uint32_t appTotal = appUsed + appFree;
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

    // MQTT information
    const RuntimeConfig &runtimeConfig = getRuntimeConfig();
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
    if (settings.enabled)
    {
        unbiddenInk["start_hour"] = settings.startHour;
        unbiddenInk["end_hour"] = settings.endHour;
        unbiddenInk["frequency_minutes"] = settings.frequencyMinutes;
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
        button["gpio"] = buttonGPIOs[i];
        button["short_endpoint"] = runtimeConfig.buttonShortActions[i];
        button["long_endpoint"] = runtimeConfig.buttonLongActions[i];
    }

    // Logging configuration
    JsonObject logging = features.createNestedObject("logging");
    logging["level"] = logLevel;
    logging["level_name"] = getLogLevelString(logLevel);
    logging["serial_enabled"] = enableSerialLogging;
    logging["file_enabled"] = enableFileLogging;
    logging["mqtt_enabled"] = enableMQTTLogging;
    logging["betterstack_enabled"] = enableBetterStackLogging;
    if (enableMQTTLogging)
    {
        logging["mqtt_topic"] = mqttLogTopic;
    }
    if (enableFileLogging)
    {
        logging["file_name"] = logFileName;
        logging["max_file_size"] = maxLogFileSize;
    }

    // === CONFIGURATION LIMITS ===
    JsonObject config = doc.createNestedObject("configuration");
    config["max_message_chars"] = maxCharacters;
    config["max_prompt_chars"] = maxPromptCharacters;

    // Temperature (if available)
#ifdef SOC_TEMP_SENSOR_SUPPORTED
    float temp = temperatureRead();
    if (!isnan(temp))
    {
        hardware["temperature"] = temp;
    }
#endif

    // Serialize and send
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleButtons()
{
    if (isRateLimited())
    {
        sendRateLimitResponse(server);
        return;
    }

    String buttonConfig = getButtonConfigJson();
    LOG_VERBOSE("WEB", "Button configuration requested");
    server.send(200, "application/json", buttonConfig);
}

void handleMQTTSend()
{
    if (isRateLimited())
    {
        sendRateLimitResponse(server);
        return;
    }

    if (!mqttClient.connected())
    {
        sendErrorResponse(server, 503, "MQTT client not connected");
        return;
    }

    // Get and validate JSON body
    String body = server.arg("plain");
    if (body.length() == 0)
    {
        sendValidationError(ValidationResult(false, "No JSON body provided"));
        return;
    }

    // Validate JSON structure
    const char *requiredFields[] = {"topic", "message"};
    ValidationResult jsonValidation = validateJSON(body, requiredFields, 2);
    if (!jsonValidation.isValid)
    {
        sendValidationError(jsonValidation);
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
        sendValidationError(topicValidation);
        return;
    }

    // Validate message content
    ValidationResult messageValidation = validateMessage(message);
    if (!messageValidation.isValid)
    {
        sendValidationError(messageValidation);
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
        server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Message scribed via MQTT\"}");
    }
    else
    {
        LOG_ERROR("WEB", "Failed to send MQTT message to topic: %s", topic.c_str());
        server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to send MQTT message - broker error\"}");
    }
}

void handleConfigGet()
{
    if (isAPMode())
    {
        // DEBUG: handleConfigGet() called
    }

    // Check rate limiting
    if (isRateLimited())
    {
        if (isAPMode())
        {
            // DEBUG: handleConfigGet - rate limited
        }
        DynamicJsonDocument errorResponse(256);
        errorResponse["success"] = false;
        errorResponse["error"] = getRateLimitReason();

        String errorString;
        serializeJson(errorResponse, errorString);
        server.send(429, "application/json", errorString);
        return;
    }

    if (isAPMode())
    {
        // DEBUG: handleConfigGet - checking config file validity
    }
    // Ensure config.json exists (create from defaults if needed)
    if (!isConfigFileValid())
    {
        if (isAPMode())
        {
            // DEBUG: handleConfigGet - config file invalid, creating defaults
        }
        LOG_NOTICE("WEB", "config.json not found or invalid, creating from defaults");
        if (!createDefaultConfigFile())
        {
            LOG_ERROR("WEB", "Failed to create default config.json");
            sendErrorResponse(server, 500, "Failed to create configuration file.");
            return;
        }
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        LOG_ERROR("WEB", "config.json still not accessible after creation");
        sendErrorResponse(server, 500, "Configuration file access error.");
        return;
    }

    // Parse config.json
    DynamicJsonDocument userConfig(2048);
    DeserializationError error = deserializeJson(userConfig, configFile);
    configFile.close();

    if (error)
    {
        LOG_ERROR("WEB", "Failed to parse config.json: %s", error.c_str());
        sendErrorResponse(server, 500, "Invalid config.json format");
        return;
    }

    // Create response with config.json data (with defaults for missing keys)
    DynamicJsonDocument configDoc(2048);

    // Device configuration (with defaults)
    JsonObject device = configDoc.createNestedObject("device");
    if (userConfig.containsKey("device"))
    {
        device["owner"] = userConfig["device"]["owner"] | defaultDeviceOwner;
        device["timezone"] = userConfig["device"]["timezone"] | defaultTimezone;
    }
    else
    {
        device["owner"] = defaultDeviceOwner;
        device["timezone"] = defaultTimezone;
    }

    // WiFi configuration (empty if not configured in config.json)
    JsonObject wifi = configDoc.createNestedObject("wifi");
    if (userConfig.containsKey("wifi"))
    {
        wifi["ssid"] = userConfig["wifi"]["ssid"] | "";
        wifi["password"] = userConfig["wifi"]["password"] | "";
    }
    else
    {
        wifi["ssid"] = "";
        wifi["password"] = "";
    }

    // MQTT configuration
    if (userConfig.containsKey("mqtt"))
    {
        configDoc["mqtt"] = userConfig["mqtt"];
    }
    if (userConfig.containsKey("apis"))
    {
        // Only expose chatgptApiToken, not endpoints
        JsonObject apis = configDoc.createNestedObject("apis");
        if (userConfig["apis"].containsKey("chatgptApiToken"))
        {
            apis["chatgptApiToken"] = userConfig["apis"]["chatgptApiToken"];
        }
    }
    if (userConfig.containsKey("validation"))
    {
        configDoc["validation"] = userConfig["validation"];
    }
    if (userConfig.containsKey("webInterface"))
    {
        configDoc["webInterface"] = userConfig["webInterface"];
    }
    else
    {
        // Add default webInterface settings if not in config
        JsonObject webInterface = configDoc.createNestedObject("webInterface");
        webInterface["printerDiscoveryPollingInterval"] = defaultPrinterDiscoveryPollingInterval * 1000;
    }
    if (userConfig.containsKey("unbiddenInk"))
    {
        configDoc["unbiddenInk"] = userConfig["unbiddenInk"];
    }
    if (userConfig.containsKey("buttons"))
    {
        configDoc["buttons"] = userConfig["buttons"];
    }

    // Add local printer configuration
    JsonObject printer = configDoc.createNestedObject("printer");
    printer["name"] = getLocalPrinterName();
    printer["topic"] = getLocalPrinterTopic();
    printer["type"] = "local";

    // Note: Remote printers are now served via /api/printer-discovery endpoint

    // Add runtime status information for Unbidden Ink
    JsonObject status = configDoc.createNestedObject("status");
    JsonObject unbiddenInkStatus = status.createNestedObject("unbiddenInk");

    // Get current unbidden ink settings and next scheduled time
    // Reload settings from file to ensure we have the latest values
    loadUnbiddenInkSettings();
    UnbiddenInkSettings currentSettings = getCurrentUnbiddenInkSettings();
    if (currentSettings.enabled)
    {
        unsigned long nextTime = getNextUnbiddenInkTime();
        unsigned long currentTime = millis();

        if (nextTime > currentTime)
        {
            unsigned long minutesUntil = (nextTime - currentTime) / (60 * 1000);
            if (minutesUntil == 0)
            {
                unbiddenInkStatus["nextScheduled"] = "< 1 min";
            }
            else
            {
                unbiddenInkStatus["nextScheduled"] = String(minutesUntil) + (minutesUntil == 1 ? " min" : " mins");
            }
        }
        else
        {
            unbiddenInkStatus["nextScheduled"] = "-";
        }
    }
    else
    {
        unbiddenInkStatus["nextScheduled"] = "-";
    }

    String configString;
    serializeJson(configDoc, configString);

    LOG_VERBOSE("WEB", "Configuration from config.json, returning %d bytes", configString.length());
    server.send(200, "application/json", configString);
}

void handleConfigPost()
{
    if (isRateLimited())
    {
        sendRateLimitResponse(server);
        return;
    }

    // Get and validate JSON body
    String body = server.arg("plain");
    if (body.length() == 0)
    {
        sendValidationError(ValidationResult(false, "No JSON body provided"));
        return;
    }

    // Parse JSON to validate structure
    DynamicJsonDocument doc(largeJsonDocumentSize);
    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        sendValidationError(ValidationResult(false, "Invalid JSON format: " + String(error.c_str())));
        return;
    }

    // Validate required top-level sections exist
    const char *requiredSections[] = {"device", "wifi", "mqtt", "apis", "validation", "webInterface", "unbiddenInk", "buttons"};
    for (int i = 0; i < 8; i++)
    {
        if (!doc.containsKey(requiredSections[i]))
        {
            sendValidationError(ValidationResult(false, "Missing required section: " + String(requiredSections[i])));
            return;
        }
    }

    // Validate device configuration
    JsonObject device = doc["device"];
    if (!device.containsKey("owner") || !device.containsKey("timezone"))
    {
        sendValidationError(ValidationResult(false, "Missing required device configuration fields"));
        return;
    }

    String owner = device["owner"];
    String timezone = device["timezone"];
    if (owner.length() == 0 || timezone.length() == 0)
    {
        sendValidationError(ValidationResult(false, "Device owner and timezone cannot be empty"));
        return;
    }

    // Validate WiFi configuration
    JsonObject wifi = doc["wifi"];
    if (!wifi.containsKey("ssid") || !wifi.containsKey("password"))
    {
        sendValidationError(ValidationResult(false, "Missing required WiFi configuration fields"));
        return;
    }

    String ssid = wifi["ssid"];
    String password = wifi["password"];
    if (ssid.length() == 0)
    {
        sendValidationError(ValidationResult(false, "WiFi SSID cannot be empty"));
        return;
    }
    if (password.length() == 0)
    {
        sendValidationError(ValidationResult(false, "WiFi password cannot be empty"));
        return;
    } // Validate MQTT configuration
    JsonObject mqtt = doc["mqtt"];
    if (!mqtt.containsKey("server") || !mqtt.containsKey("port") ||
        !mqtt.containsKey("username") || !mqtt.containsKey("password"))
    {
        sendValidationError(ValidationResult(false, "Missing required MQTT configuration fields"));
        return;
    }

    int port = mqtt["port"];
    if (port < 1 || port > 65535)
    {
        sendValidationError(ValidationResult(false, "MQTT port must be between 1 and 65535"));
        return;
    }

    // Validate APIs configuration (only user-configurable fields)
    JsonObject apis = doc["apis"];
    if (!apis.containsKey("chatgptApiToken"))
    {
        sendValidationError(ValidationResult(false, "Missing required ChatGPT API token"));
        return;
    }

    // Validate validation configuration (only maxCharacters)
    JsonObject validation = doc["validation"];
    if (!validation.containsKey("maxCharacters"))
    {
        sendValidationError(ValidationResult(false, "Missing required maxCharacters field"));
        return;
    }

    int maxChars = validation["maxCharacters"];
    if (maxChars < 100 || maxChars > 5000)
    {
        sendValidationError(ValidationResult(false, "Max characters must be between 100 and 5000"));
        return;
    }

    // Validate webInterface configuration
    JsonObject webInterface = doc["webInterface"];
    if (!webInterface.containsKey("printerDiscoveryPollingInterval"))
    {
        sendValidationError(ValidationResult(false, "Missing required printerDiscoveryPollingInterval field"));
        return;
    }

    int pollingInterval = webInterface["printerDiscoveryPollingInterval"];
    if (pollingInterval < 5000 || pollingInterval > 300000)
    {
        sendValidationError(ValidationResult(false, "Printer discovery polling interval must be between 5000ms and 300000ms"));
        return;
    }

    // Validate unbidden ink configuration
    JsonObject unbiddenInk = doc["unbiddenInk"];
    if (!unbiddenInk.containsKey("enabled") || !unbiddenInk.containsKey("startHour") ||
        !unbiddenInk.containsKey("endHour") || !unbiddenInk.containsKey("frequencyMinutes"))
    {
        sendValidationError(ValidationResult(false, "Missing required Unbidden Ink configuration fields"));
        return;
    }

    bool enabled = unbiddenInk["enabled"];
    int startHour = unbiddenInk["startHour"];
    int endHour = unbiddenInk["endHour"];
    int frequency = unbiddenInk["frequencyMinutes"];

    if (startHour < 0 || startHour > 23 || endHour < 0 || endHour > 23)
    {
        sendValidationError(ValidationResult(false, "Hours must be between 0 and 23"));
        return;
    }

    if (startHour >= endHour)
    {
        sendValidationError(ValidationResult(false, "Start hour must be before end hour"));
        return;
    }

    if (frequency < 15 || frequency > 480)
    {
        sendValidationError(ValidationResult(false, "Frequency must be between 15 minutes and 8 hours"));
        return;
    }

    if (enabled && (!unbiddenInk.containsKey("prompt") || String((const char *)unbiddenInk["prompt"]).length() == 0))
    {
        sendValidationError(ValidationResult(false, "Prompt required when Unbidden Ink is enabled"));
        return;
    }

    // Validate button configuration (exactly 4 buttons)
    JsonObject buttons = doc["buttons"];
    const char *buttonKeys[] = {"button1", "button2", "button3", "button4"};
    const char *validActions[] = {"/api/joke", "/api/riddle", "/api/quote", "/api/quiz", "/api/print-test", "/api/unbidden-ink", ""};

    for (int i = 0; i < 4; i++)
    {
        if (!buttons.containsKey(buttonKeys[i]))
        {
            sendValidationError(ValidationResult(false, "Missing button configuration: " + String(buttonKeys[i])));
            return;
        }

        JsonObject button = buttons[buttonKeys[i]];
        if (!button.containsKey("shortAction") || !button.containsKey("longAction"))
        {
            sendValidationError(ValidationResult(false, "Missing shortAction or longAction for " + String(buttonKeys[i])));
            return;
        }

        String shortAction = button["shortAction"];
        String longAction = button["longAction"];

        // Validate shortAction (can now be empty or valid action)
        bool validShortAction = false;
        for (int j = 0; j < 7; j++) // Include empty string for short actions
        {
            if (shortAction == validActions[j])
            {
                validShortAction = true;
                break;
            }
        }
        if (!validShortAction)
        {
            sendValidationError(ValidationResult(false, "Invalid short action for " + String(buttonKeys[i]) + ": " + shortAction));
            return;
        }

        // Validate longAction (optional, can be empty or valid action)
        bool validLongAction = false;
        for (int j = 0; j < 7; j++) // Include empty string for long actions
        {
            if (longAction == validActions[j])
            {
                validLongAction = true;
                break;
            }
        }
        if (!validLongAction)
        {
            sendValidationError(ValidationResult(false, "Invalid long action for " + String(buttonKeys[i]) + ": " + longAction));
            return;
        }
    }

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        LOG_ERROR("WEB", "Failed to open config.json for writing");
        sendErrorResponse(server, 500, "Failed to save configuration");
        return;
    }

    serializeJson(doc, configFile);
    configFile.close();

    LOG_NOTICE("WEB", "Configuration saved successfully");

    // Reload runtime configuration to reflect changes immediately
    if (!loadRuntimeConfig())
    {
        LOG_WARNING("WEB", "Failed to reload runtime config after save");
    }
    else
    {
        LOG_VERBOSE("WEB", "Runtime configuration reloaded successfully");

        // Update MQTT subscription to new device owner topic
        updateMQTTSubscription();
    }

    sendSuccessResponse(server, "Configuration saved successfully");

    if (isAPMode())
    {
        LOG_NOTICE("WEB", "Device in AP mode - rebooting to connect to new WiFi configuration");
        delay(1000);
        ESP.restart();
    }
}

void handleDiscoveredPrinters()
{
    if (isRateLimited())
    {
        sendRateLimitResponse(server);
        return;
    }

    DynamicJsonDocument doc(2048);
    JsonArray printersArray = doc.createNestedArray("discovered_printers");

    std::vector<DiscoveredPrinter> discovered = getDiscoveredPrinters();
    for (const auto &printer : discovered)
    {
        if (printer.status == "online")
        {
            JsonObject printerObj = printersArray.createNestedObject();
            printerObj["printer_id"] = printer.printerId;
            printerObj["name"] = printer.name;
            printerObj["firmware_version"] = printer.firmwareVersion;
            printerObj["mdns"] = printer.mdns;
            printerObj["ip_address"] = printer.ipAddress;
            printerObj["status"] = printer.status;
            printerObj["last_power_on"] = printer.lastPowerOn;
            printerObj["timezone"] = printer.timezone;
        }
    }

    doc["count"] = printersArray.size();
    doc["our_printer_id"] = getPrinterId();

    String response;
    serializeJson(doc, response);

    LOG_VERBOSE("WEB", "Discovered printers requested, returning %d printers", printersArray.size());
    server.send(200, "application/json", response);
}
