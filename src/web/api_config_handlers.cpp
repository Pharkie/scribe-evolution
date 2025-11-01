/**
 * @file api_config_handlers.cpp
 * @brief Configuration API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_config_handlers.h"
#include "api_handlers.h" // For shared utilities
#include "validation.h"
#include <config/config.h>
#include <core/nvs_keys.h>
#include <core/config_loader.h>
#include <core/config_utils.h>
#include <core/led_config_loader.h>
#include <core/logging.h>
#include <core/printer_discovery.h>
#include <utils/time_utils.h>
#include <core/network.h>
#include <core/mqtt_handler.h>


// Utility function to mask secrets for API responses
String maskSecret(const String &secret)
{
    if (secret.length() == 0)
    {
        return "";
    }
    if (secret.length() <= 4)
    {
        return "●●●●";
    }
    if (secret.length() <= 8)
    {
        return "●●●●●●●●";
    }
    // For longer secrets, show first 2 and last 2 characters
    return secret.substring(0, 2) + "●●●●●●●●" + secret.substring(secret.length() - 2);
}
#include <content/unbidden_ink.h>
#include <hardware/hardware_buttons.h>
#if ENABLE_LEDS
#include <leds/LedEffects.h>
#include <FastLED.h>
#endif
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <utils/api_client.h>
#include <config/system_constants.h>

#if ENABLE_LEDS
#include <leds/LedEffects.h>
// Note: ledEffects() is a singleton accessor function, not an extern object
#endif

// ========================================
// CONFIGURATION API HANDLERS
// ========================================

void handleConfigGet(AsyncWebServerRequest *request)
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
        JsonDocument errorResponse;
        errorResponse["error"] = getRateLimitReason();

        String errorString;
        serializeJson(errorResponse, errorString);
        request->send(429, "application/json", errorString);
        return;
    }

    if (isAPMode())
    {
        // DEBUG: handleConfigGet - getting runtime config
    }

    // Get current runtime configuration (from NVS or defaults)
    const RuntimeConfig &config = getRuntimeConfig();

    // Feed watchdog to prevent timeout during JSON construction
    delay(1);

    // Create response with runtime configuration data
    JsonDocument configDoc;

    // Device configuration - main section matching settings.html
    JsonObject device = configDoc["device"].to<JsonObject>();
    device["owner"] = config.deviceOwner;
    device["timezone"] = config.timezone;

    // Move maxCharacters from validation to device section
    device["maxCharacters"] = config.maxCharacters;

    // Add runtime device information
    device["firmwareVersion"] = getFirmwareVersion();
    device["chipModel"] = ESP.getChipModel();
    device["bootTime"] = getDeviceBootTime();
    device["mdns"] = String(getMdnsHostname()) + ".local";
    device["ipAddress"] = WiFi.localIP().toString();
    device["printerName"] = getLocalPrinterName();
    device["mqttTopic"] = getLocalPrinterTopic();
    device["type"] = "local";

    // Hardware GPIO configuration
    device["printerTxPin"] = config.printerTxPin;
    device["printerRxPin"] = config.printerRxPin;
    device["printerDtrPin"] = config.printerDtrPin;

    // WiFi configuration - nested under device to match settings structure
    JsonObject wifi = device["wifi"].to<JsonObject>();

    // In AP mode, encourage fresh setup by showing generic placeholders
    if (isAPMode())
    {
        wifi["ssid"] = "AP_MODE";
        wifi["password"] = ""; // Blank to encourage manual entry
    }
    else
    {
        wifi["ssid"] = config.wifiSSID;
        wifi["password"] = maskSecret(config.wifiPassword);
    }

    // Include fallback AP details for client use - always available regardless of current mode
    wifi["fallbackApSsid"] = fallbackAPSSID;
    wifi["fallbackApPassword"] = fallbackAPPassword;
    // Always provide mDNS hostname as it's consistent and preferred
    wifi["fallbackApMdns"] = String(getMdnsHostname()) + ".local";

    // WiFi status information
    JsonObject wifiStatus = wifi["status"].to<JsonObject>();
    wifiStatus["connected"] = (WiFi.status() == WL_CONNECTED);
    wifiStatus["apStaMode"] = isAPMode(); // Indicate if device is in AP-STA setup mode
    wifiStatus["ipAddress"] = WiFi.localIP().toString();
    wifiStatus["macAddress"] = WiFi.macAddress();
    wifiStatus["gateway"] = WiFi.gatewayIP().toString();
    wifiStatus["dns"] = WiFi.dnsIP().toString();

    // Format signal strength
    int rssi = WiFi.RSSI();
    String signalStrength = String(rssi) + " dBm";
    if (rssi >= -50)
    {
        signalStrength += " (Strong)";
    }
    else if (rssi >= -60)
    {
        signalStrength += " (Good)";
    }
    else if (rssi >= -70)
    {
        signalStrength += " (Fair)";
    }
    else
    {
        signalStrength += " (Weak)";
    }
    wifiStatus["signalStrength"] = signalStrength;

    // Feed watchdog after WiFi processing
    delay(1);

    // MQTT configuration - top-level section matching settings.html
    JsonObject mqtt = configDoc["mqtt"].to<JsonObject>();
    mqtt["enabled"] = config.mqttEnabled;
    mqtt["server"] = config.mqttServer;
    mqtt["port"] = config.mqttPort;
    mqtt["username"] = config.mqttUsername;
    mqtt["password"] = maskSecret(config.mqttPassword);
    // Skip MQTT connection check in AP mode to avoid potential blocking
    mqtt["connected"] = (isAPMode() || !config.mqttEnabled) ? false : MQTTManager::instance().isConnected();

    // Unbidden Ink configuration - top-level section matching settings.html
    JsonObject unbiddenInk = configDoc["unbiddenInk"].to<JsonObject>();
    unbiddenInk["enabled"] = config.unbiddenInkEnabled;
    unbiddenInk["startHour"] = config.unbiddenInkStartHour;
    unbiddenInk["endHour"] = config.unbiddenInkEndHour;
    unbiddenInk["frequencyMinutes"] = config.unbiddenInkFrequencyMinutes;
    unbiddenInk["prompt"] = config.unbiddenInkPrompt;
    unbiddenInk["chatgptApiToken"] = maskSecret(config.chatgptApiToken);

    // Add prompt presets for quick selection
    JsonObject prompts = unbiddenInk["promptPresets"].to<JsonObject>();
    prompts["creative"] = unbiddenInkPromptCreative;
    prompts["wisdom"] = unbiddenInkPromptWisdom;
    prompts["humor"] = unbiddenInkPromptHumor;
    prompts["doctorwho"] = unbiddenInkPromptDoctorWho;

    // Add runtime status for Unbidden Ink
    if (config.unbiddenInkEnabled)
    {
        unsigned long nextTime = getNextUnbiddenInkTime();
        unsigned long currentTime = millis();

        if (nextTime > currentTime)
        {
            unsigned long minutesUntil = (nextTime - currentTime) / (60 * 1000);
            if (minutesUntil == 0)
            {
                unbiddenInk["nextScheduled"] = "< 1 min";
            }
            else
            {
                unbiddenInk["nextScheduled"] = String(minutesUntil) + (minutesUntil == 1 ? " min" : " mins");
            }
        }
        else
        {
            unbiddenInk["nextScheduled"] = "-";
        }
    }
    else
    {
        unbiddenInk["nextScheduled"] = "-";
    }

    // Memos are now handled by separate /api/memos endpoint

    // Buttons configuration - top-level section matching settings.html
    JsonObject buttons = configDoc["buttons"].to<JsonObject>();

    // Hardware button status information
    buttons["count"] = numHardwareButtons;
    buttons["debounceTime"] = buttonDebounceMs;
    buttons["longPressTime"] = buttonLongPressMs;
    buttons["activeLow"] = buttonActiveLow;
    buttons["minInterval"] = buttonMinInterval;
    buttons["maxPerMinute"] = buttonMaxPerMinute;

    // Button action configuration
    for (int i = 0; i < numHardwareButtons; i++)
    {
        String buttonKey = "button" + String(i + 1);
        JsonObject button = buttons[buttonKey].to<JsonObject>();

        // Add GPIO pin information for each button
        button["gpio"] = config.buttonGpios[i];

        button["shortAction"] = config.buttonShortActions[i];
        button["shortMqttTopic"] = config.buttonShortMqttTopics[i];
        button["longAction"] = config.buttonLongActions[i];
        button["longMqttTopic"] = config.buttonLongMqttTopics[i];

        // Add LED effect configuration
        button["shortLedEffect"] = config.buttonShortLedEffects[i];
        button["longLedEffect"] = config.buttonLongLedEffects[i];
    }

#if ENABLE_LEDS
    // LEDs configuration - top-level section matching settings.html
    JsonObject leds = configDoc["leds"].to<JsonObject>();
    leds["enabled"] = true; // LED support is compiled in
    leds["pin"] = config.ledPin;
    leds["count"] = config.ledCount;
    leds["brightness"] = config.ledBrightness;
    leds["refreshRate"] = config.ledRefreshRate;

    // Add effectDefaults structure for frontend LED playground (10-100 scale)
    JsonObject effectDefaults = leds["effectDefaults"].to<JsonObject>();

    // Chase Single defaults
    JsonObject chaseSingle = effectDefaults["chase_single"].to<JsonObject>();
    chaseSingle["speed"] = 50;
    chaseSingle["intensity"] = 50;
    chaseSingle["cycles"] = DEFAULT_LED_EFFECT_CYCLES;
    JsonArray chaseSingleColors = chaseSingle["colors"].to<JsonArray>();
    chaseSingleColors.add(config.ledEffects.chaseSingle.defaultColor);

    // Chase Multi defaults
    JsonObject chaseMulti = effectDefaults["chase_multi"].to<JsonObject>();
    chaseMulti["speed"] = 50;
    chaseMulti["intensity"] = 50;
    chaseMulti["cycles"] = DEFAULT_LED_EFFECT_CYCLES;
    JsonArray chaseMultiColors = chaseMulti["colors"].to<JsonArray>();
    chaseMultiColors.add(config.ledEffects.chaseMulti.color1);
    chaseMultiColors.add(config.ledEffects.chaseMulti.color2);
    chaseMultiColors.add(config.ledEffects.chaseMulti.color3);

    // Matrix defaults
    JsonObject matrix = effectDefaults["matrix"].to<JsonObject>();
    matrix["speed"] = 50;
    matrix["intensity"] = 50;
    matrix["cycles"] = DEFAULT_LED_EFFECT_CYCLES;
    JsonArray matrixColors = matrix["colors"].to<JsonArray>();
    matrixColors.add(config.ledEffects.matrix.defaultColor);

    // Twinkle defaults
    JsonObject twinkle = effectDefaults["twinkle"].to<JsonObject>();
    twinkle["speed"] = 50;
    twinkle["intensity"] = 50;
    twinkle["cycles"] = DEFAULT_LED_EFFECT_CYCLES;
    JsonArray twinkleColors = twinkle["colors"].to<JsonArray>();
    twinkleColors.add(config.ledEffects.twinkle.defaultColor);

    // Pulse defaults
    JsonObject pulse = effectDefaults["pulse"].to<JsonObject>();
    pulse["speed"] = 50;
    pulse["intensity"] = 50;
    pulse["cycles"] = DEFAULT_LED_EFFECT_CYCLES;
    JsonArray pulseColors = pulse["colors"].to<JsonArray>();
    pulseColors.add(config.ledEffects.pulse.defaultColor);

    // Rainbow defaults
    JsonObject rainbow = effectDefaults["rainbow"].to<JsonObject>();
    rainbow["speed"] = 50;
    rainbow["intensity"] = 50;
    rainbow["cycles"] = DEFAULT_LED_EFFECT_CYCLES;
    JsonArray rainbowColors = rainbow["colors"].to<JsonArray>();
    rainbowColors.add("#ff0000"); // Rainbow doesn't use colors but needs array
#else
    // LEDs disabled at compile time - provide minimal config to inform frontend
    JsonObject leds = configDoc["leds"].to<JsonObject>();
    leds["enabled"] = false; // LED support is NOT compiled in
#endif

    // GPIO information for frontend validation
    JsonObject gpio = configDoc["gpio"].to<JsonObject>();
    JsonArray availablePins = gpio["availablePins"].to<JsonArray>();
    JsonArray safePins = gpio["safePins"].to<JsonArray>();
    JsonObject pinDescriptions = gpio["pinDescriptions"].to<JsonObject>();

    // Add all GPIO information for current board
    for (int i = 0; i < BOARD_GPIO_MAP_SIZE; i++)
    {
        int pin = BOARD_GPIO_MAP[i].pin;
        availablePins.add(pin);
        pinDescriptions[String(pin)] = BOARD_GPIO_MAP[i].description;

        if (isSafeGPIO(pin))
        {
            safePins.add(pin);
        }
    }

    // Check if JSON document has overflowed before serialization
    if (configDoc.overflowed())
    {
        LOG_ERROR("CONFIG", "JSON document overflow detected!");
        sendErrorResponse(request, 500, "Configuration too large for buffer");
        return;
    }

    // Feed watchdog before JSON serialization
    delay(1);

    String configString;
    size_t jsonSize = serializeJson(configDoc, configString);

    LOG_VERBOSE("CONFIG", "JSON serialized: %d bytes", jsonSize);

    if (jsonSize == 0)
    {
        LOG_ERROR("WEB", "Failed to serialize config JSON");
        JsonDocument errorDoc;
        errorDoc["error"] = "JSON serialization failed";
        String errorString;
        serializeJson(errorDoc, errorString);
        request->send(500, "application/json", errorString);
        return;
    }

    LOG_VERBOSE("WEB", "Configuration from NVS, returning %d bytes", configString.length());
    request->send(200, "application/json", configString);
}

#include "config_field_registry.h"

void handleConfigPost(AsyncWebServerRequest *request)
{
    if (isRateLimited())
    {
        sendRateLimitResponse(request);
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

    // Parse JSON to validate structure
    JsonDocument doc;

    LOG_VERBOSE("WEB", "Config POST body length: %d", body.length());
    LOG_VERBOSE("WEB", "Config POST body (first 200 chars): %s", body.substring(0, 200).c_str());

    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        LOG_ERROR("WEB", "JSON deserialization failed: %s", error.c_str());
        LOG_ERROR("WEB", "JSON body length: %d", body.length());
        sendValidationError(request, ValidationResult(false, "Invalid JSON format: " + String(error.c_str())));
        return;
    }

    // Load current configuration for partial updates
    RuntimeConfig currentConfig = getRuntimeConfig();
    RuntimeConfig newConfig = currentConfig;
    
    // Track timezone changes for immediate update
    String currentTimezone = currentConfig.timezone;

    // Data-driven configuration processing - handles ALL fields generically
    String errorMsg;
    if (!processJsonObject("", doc.as<JsonObject>(), newConfig, errorMsg))
    {
        sendValidationError(request, ValidationResult(false, errorMsg));
        return;
    }

    // Debug: Check MQTT password before and after processing
    LOG_VERBOSE("WEB", "MQTT Debug - Current password length: %d", newConfig.mqttPassword.length());
    LOG_VERBOSE("WEB", "MQTT Debug - NewConfig password length after processing: %d", newConfig.mqttPassword.length());

    // MQTT password fix: If frontend didn't send password, preserve existing one
    if (doc["mqtt"].is<JsonObject>())
    {
        JsonObject mqttObj = doc["mqtt"];
        if (!mqttObj["password"].is<const char*>())
        {
            LOG_VERBOSE("WEB", "MQTT password not in request, preserving existing stored password (length: %d)", currentConfig.mqttPassword.length());
            newConfig.mqttPassword = currentConfig.mqttPassword;
        }
        else
        {
            LOG_NOTICE("WEB", "MQTT password provided in request");
        }
    }

    // Non-user configurable APIs remain as constants (always set regardless of sections present)
    newConfig.jokeAPI = jokeAPI;
    newConfig.quoteAPI = quoteAPI;
    newConfig.triviaAPI = triviaAPI;
    newConfig.newsAPI = newsAPI;
        // configDoc["betterStackToken"] = config.betterStackToken; // Removed
        // configDoc["betterStackEndpoint"] = config.betterStackEndpoint; // Removed
    newConfig.chatgptApiEndpoint = chatgptApiEndpoint;

    // maxCharacters remains hardcoded from config.h
    newConfig.maxCharacters = maxCharacters;

#if ENABLE_LEDS
    // Load default LED effects configuration (TODO: make this data-driven too)
    newConfig.ledEffects = getDefaultLedEffectsConfig();
#endif

    // Check change states before saving (using currentConfig as the original state)
    bool mqttStateChanged = (currentConfig.mqttEnabled != newConfig.mqttEnabled);
    
    // Check if MQTT settings changed (only matters if MQTT is enabled)
    bool mqttSettingsChanged = false;
    if (doc["mqtt"].is<JsonObject>() && newConfig.mqttEnabled) {
        mqttSettingsChanged = (
            currentConfig.mqttServer != newConfig.mqttServer ||
            currentConfig.mqttPort != newConfig.mqttPort ||
            currentConfig.mqttUsername != newConfig.mqttUsername ||
            currentConfig.mqttPassword != newConfig.mqttPassword
        );
    }
    
    // Check UnbiddenInk state changes
    bool unbiddenStateChanged = (currentConfig.unbiddenInkEnabled != newConfig.unbiddenInkEnabled);
    
    // Check if UnbiddenInk settings changed (only matters if UnbiddenInk is enabled)
    bool unbiddenSettingsChanged = false;
    if (doc["unbiddenInk"].is<JsonObject>() && newConfig.unbiddenInkEnabled) {
        unbiddenSettingsChanged = (
            currentConfig.unbiddenInkStartHour != newConfig.unbiddenInkStartHour ||
            currentConfig.unbiddenInkEndHour != newConfig.unbiddenInkEndHour ||
            currentConfig.unbiddenInkFrequencyMinutes != newConfig.unbiddenInkFrequencyMinutes ||
            currentConfig.unbiddenInkPrompt != newConfig.unbiddenInkPrompt ||
            currentConfig.chatgptApiToken != newConfig.chatgptApiToken
        );
    }
    
    // Check if WiFi credentials changed
    bool wifiCredentialsChanged = false;
    if (doc["wifi"].is<JsonObject>())
    {
        JsonObject wifiObj = doc["wifi"];

        // Check if SSID changed
        if (wifiObj["ssid"].is<const char*>() &&
            newConfig.wifiSSID != currentConfig.wifiSSID)
        {
            wifiCredentialsChanged = true;
            LOG_NOTICE("WEB", "WiFi SSID changed from '%s' to '%s'",
                       currentConfig.wifiSSID.c_str(),
                       newConfig.wifiSSID.c_str());
        }

        // Check if password changed (only if provided)
        if (wifiObj["password"].is<const char*>() &&
            newConfig.wifiPassword != currentConfig.wifiPassword)
        {
            wifiCredentialsChanged = true;
            LOG_NOTICE("WEB", "WiFi password changed");
        }
    }

    // FIRST: Save to NVS for persistence (fail-safe - don't update runtime if this fails)
    if (!saveNVSConfig(newConfig))
    {
        LOG_ERROR("WEB", "Failed to save configuration to NVS");
        sendErrorResponse(request, 500, "Failed to save configuration");
        return;
    }

    // ONLY THEN: Update global runtime configuration (after successful NVS save)
    setRuntimeConfig(newConfig);
    
    // Handle timezone changes - update immediately without requiring reboot
    bool timezoneChanged = (currentTimezone != newConfig.timezone);
    if (timezoneChanged) {
        LOG_NOTICE("WEB", "Timezone changed from %s to %s - updating immediately", 
                   currentTimezone.c_str(), newConfig.timezone.c_str());
        bool timezoneUpdated = updateTimezone(newConfig.timezone);
        if (timezoneUpdated) {
            LOG_NOTICE("WEB", "Timezone successfully updated to %s", newConfig.timezone.c_str());
        } else {
            LOG_WARNING("WEB", "Failed to update timezone to %s - will retry on next reboot", 
                       newConfig.timezone.c_str());
        }
    }

    // Handle dynamic MQTT start/stop
    if (mqttStateChanged)
    {
        if (newConfig.mqttEnabled)
        {
            LOG_NOTICE("WEB", "MQTT enabled - starting client");
            startMQTTClient(true);  // true = immediate connection
        }
        else
        {
            LOG_NOTICE("WEB", "MQTT disabled - stopping client");
            stopMQTTClient();
        }
    }
    else if (mqttSettingsChanged)
    {
        // MQTT settings changed but was already enabled - restart cleanly
        LOG_NOTICE("WEB", "MQTT settings updated - restarting client");
        stopMQTTClient();
        delay(100);  // Brief cleanup delay
        startMQTTClient(true);  // true = immediate reconnection
    }

    // Handle dynamic UnbiddenInk start/stop
    if (unbiddenStateChanged)
    {
        if (newConfig.unbiddenInkEnabled)
        {
            LOG_NOTICE("WEB", "UnbiddenInk enabled - starting scheduler");
            startUnbiddenInk(true);  // true = immediate scheduling for feedback
        }
        else
        {
            LOG_NOTICE("WEB", "UnbiddenInk disabled - stopping scheduler");
            stopUnbiddenInk();
        }
    }
    else if (unbiddenSettingsChanged)
    {
        // UnbiddenInk settings changed but was already enabled - restart cleanly
        LOG_NOTICE("WEB", "UnbiddenInk settings updated - restarting scheduler");
        restartUnbiddenInk();
    }

#if ENABLE_LEDS
    // Reinitialize LED system with new configuration
    if (ledEffects().reinitialize(newConfig.ledPin, newConfig.ledCount, newConfig.ledBrightness,
                                newConfig.ledRefreshRate, newConfig.ledEffects))
    {
        LOG_VERBOSE("WEB", "LED system reinitialized with new configuration");

        // Trigger green chase single effect as visual confirmation of successful save
        ledEffects().startEffectCycles("chase_single", 1, CRGB::Green);
        LOG_VERBOSE("WEB", "LED confirmation effect triggered for config save");
    }
    else
    {
        LOG_WARNING("WEB", "Failed to reinitialize LED system with new configuration");
    }
#endif

    // Handle WiFi credential changes requiring restart
    if (wifiCredentialsChanged && !isAPMode())
    {
        LOG_NOTICE("WEB", "WiFi credentials changed - device will restart to apply new settings");
        
        // Send response with restart flag
        JsonDocument response;
        response["restart"] = true;
        response["reason"] = "wifi_change";
        String jsonResponse;
        serializeJson(response, jsonResponse);
        request->send(200, "application/json", jsonResponse);
        
        // Schedule restart after response is sent
        delay(2000);  // Give frontend time to show overlay
        LOG_NOTICE("WEB", "Restarting to connect to new WiFi network: %s", 
                   newConfig.wifiSSID.c_str());
        ESP.restart();
        return;
    }

    // Handle AP mode restart (existing logic)
    if (isAPMode())
    {
        LOG_NOTICE("WEB", "Device in AP-STA mode - rebooting to connect to new WiFi configuration");
        request->send(200);
        delay(1000);
        ESP.restart();
        return;
    }

    // Normal success response (no restart needed)
    request->send(200);
}

// ========================================
// MEMOS API ENDPOINTS
// ========================================

void handleMemosGet(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "handleMemosGet() called");

    // Create JSON response with all memos
    JsonDocument memosDoc;

    // Get memo values from centralized config system
    const RuntimeConfig &config = getRuntimeConfig();

    memosDoc["memo1"] = config.memos[0];
    memosDoc["memo2"] = config.memos[1];
    memosDoc["memo3"] = config.memos[2];
    memosDoc["memo4"] = config.memos[3];

    // Check for overflow
    if (memosDoc.overflowed())
    {
        LOG_ERROR("MEMOS", "Memos JSON document overflow detected!");
        sendErrorResponse(request, 500, "Memos too large for buffer");
        return;
    }

    // Serialize and send
    String memosString;
    size_t jsonSize = serializeJson(memosDoc, memosString);

    if (jsonSize == 0)
    {
        LOG_ERROR("WEB", "Failed to serialize memos JSON");
        sendErrorResponse(request, 500, "Failed to serialize memos");
        return;
    }

    LOG_VERBOSE("WEB", "Memos sent to client (%zu bytes)", jsonSize);
    request->send(200, "application/json", memosString);
}

void handleMemosPost(AsyncWebServerRequest *request)
{
    extern String getRequestBody(AsyncWebServerRequest * request);

    LOG_VERBOSE("WEB", "handleMemosPost() called");

    // Get request body
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        sendErrorResponse(request, 400, "No JSON body provided");
        return;
    }

    // Parse JSON
    JsonDocument memosDoc;
    DeserializationError error = deserializeJson(memosDoc, body);
    if (error)
    {
        LOG_ERROR("WEB", "Failed to parse memos JSON: %s", error.c_str());
        sendErrorResponse(request, 400, "Invalid JSON format");
        return;
    }

    // Get current config and update memo fields
    RuntimeConfig currentConfig = getRuntimeConfig();
    const char *memoNames[] = {"memo1", "memo2", "memo3", "memo4"};

    // Update each memo with validation
    for (int i = 0; i < 4; i++)
    {
        if (memosDoc[memoNames[i]].is<const char*>())
        {
            String memoContent = memosDoc[memoNames[i]].as<String>();

            // Validate memo length
            if (memoContent.length() > MEMO_MAX_LENGTH)
            {
                sendErrorResponse(request, 400,
                                  String("Memo ") + String(i + 1) + " exceeds maximum length of " + String(MEMO_MAX_LENGTH) + " characters");
                return;
            }

            currentConfig.memos[i] = memoContent;
            LOG_VERBOSE("WEB", "Updated memo %d (%d characters)", i + 1, memoContent.length());
        }
    }

    // Save updated config to NVS through centralized system
    if (!saveNVSConfig(currentConfig))
    {
        LOG_ERROR("WEB", "Failed to save memo configuration to NVS");
        sendErrorResponse(request, 500, "Failed to save memo configuration");
        return;
    }

    // Update runtime config
    setRuntimeConfig(currentConfig);

    request->send(200);
    LOG_NOTICE("WEB", "All memos saved successfully");
}

void handleSetupPost(AsyncWebServerRequest *request)
{
    extern String getRequestBody(AsyncWebServerRequest * request);

    LOG_VERBOSE("WEB", "handleSetupPost() called - initial device setup");

    // Get request body
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        LOG_ERROR("WEB", "Setup request body is empty");
        sendErrorResponse(request, 400, "Request body is empty");
        return;
    }

    // Parse JSON - minimal buffer since setup only has device config
    JsonDocument doc;

    LOG_VERBOSE("WEB", "Setup POST body: %s", body.c_str());

    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        LOG_ERROR("WEB", "Setup JSON deserialization failed: %s", error.c_str());
        sendValidationError(request, ValidationResult(false, "Invalid JSON format: " + String(error.c_str())));
        return;
    }

    // Minimal validation - only require device section for setup
    if (!doc["device"].is<JsonObject>())
    {
        sendValidationError(request, ValidationResult(false, "Missing required section: device"));
        return;
    }

    // Load current configuration to preserve existing settings
    RuntimeConfig currentConfig = getRuntimeConfig();
    RuntimeConfig newConfig = currentConfig; // Start with current values

    // Track timezone changes for immediate update
    String currentTimezone = currentConfig.timezone;

    // Validate and extract device configuration
    JsonObject device = doc["device"];
    if (!device["owner"].is<const char*>() || !device["timezone"].is<const char*>())
    {
        sendValidationError(request, ValidationResult(false, "Missing required device configuration fields (owner, timezone)"));
        return;
    }

    // Validate WiFi configuration (nested under device)
    if (!device["wifi"].is<JsonObject>() || !device["wifi"]["ssid"].is<const char*>() || !device["wifi"]["password"].is<const char*>())
    {
        sendValidationError(request, ValidationResult(false, "Missing required WiFi configuration (ssid, password)"));
        return;
    }

    String owner = device["owner"];
    String timezone = device["timezone"];
    String ssid = device["wifi"]["ssid"];
    String password = device["wifi"]["password"];

    // Validate non-empty values
    if (owner.length() == 0 || timezone.length() == 0 || ssid.length() == 0 || password.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "Device owner, timezone, WiFi SSID, and password cannot be empty"));
        return;
    }

    // Update only the setup-configurable fields
    newConfig.deviceOwner = owner;
    newConfig.timezone = timezone;
    newConfig.wifiSSID = ssid;
    newConfig.wifiPassword = password;

    // Parse optional printer GPIO pins (preserve defaults if not provided)
    if (device["printerTxPin"].is<int>())
    {
        int printerTxPin = device["printerTxPin"];
        if (!isValidGPIO(printerTxPin) || !isSafeGPIO(printerTxPin))
        {
            sendValidationError(request, ValidationResult(false, "Invalid printer TX GPIO pin"));
            return;
        }
        newConfig.printerTxPin = printerTxPin;
    }

    if (device["printerRxPin"].is<int>())
    {
        int printerRxPin = device["printerRxPin"];
        // Allow -1 for disabled RX
        if (printerRxPin != -1 && (!isValidGPIO(printerRxPin) || !isSafeGPIO(printerRxPin)))
        {
            sendValidationError(request, ValidationResult(false, "Invalid printer RX GPIO pin"));
            return;
        }
        newConfig.printerRxPin = printerRxPin;
    }

    if (device["printerDtrPin"].is<int>())
    {
        int printerDtrPin = device["printerDtrPin"];
        // Allow -1 for disabled DTR
        if (printerDtrPin != -1 && (!isValidGPIO(printerDtrPin) || !isSafeGPIO(printerDtrPin)))
        {
            sendValidationError(request, ValidationResult(false, "Invalid printer DTR GPIO pin"));
            return;
        }
        newConfig.printerDtrPin = printerDtrPin;
    }

    // Save the updated configuration
    setRuntimeConfig(newConfig);

    // Save to NVS for persistence
    if (!saveNVSConfig(newConfig))
    {
        LOG_ERROR("WEB", "Failed to save setup configuration to NVS");
        sendErrorResponse(request, 500, "Failed to save configuration");
        return;
    }

    // Handle timezone changes - update immediately without requiring reboot
    bool timezoneChanged = (currentTimezone != newConfig.timezone);
    if (timezoneChanged) {
        LOG_NOTICE("WEB", "Setup: Timezone changed from %s to %s - updating immediately", 
                   currentTimezone.c_str(), newConfig.timezone.c_str());
        bool timezoneUpdated = updateTimezone(newConfig.timezone);
        if (timezoneUpdated) {
            LOG_NOTICE("WEB", "Setup: Timezone successfully updated to %s", newConfig.timezone.c_str());
        } else {
            LOG_WARNING("WEB", "Setup: Failed to update timezone to %s - will retry on next reboot", 
                       newConfig.timezone.c_str());
        }
    }

    LOG_NOTICE("WEB", "Setup configuration saved successfully");

    request->send(200);

    // In AP-STA mode, reboot after short delay to connect to new WiFi
    if (isAPMode())
    {
        LOG_NOTICE("WEB", "Device in AP-STA mode - rebooting to connect to new WiFi configuration");
        // Use async callback instead of blocking delay
        static WiFiEventId_t eventId = WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
                                                    { ESP.restart(); }, ARDUINO_EVENT_WIFI_AP_STOP);

        // Schedule restart in next loop iteration
        WiFi.mode(WIFI_STA); // This will trigger AP stop and restart
    }
}

void handleSetupGet(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "handleSetupGet() called - AP-STA mode setup configuration request");

    // Create minimal configuration document for setup - only what's needed for initial configuration
    JsonDocument setupDoc;

    // Device section with minimal defaults
    JsonObject device = setupDoc["device"].to<JsonObject>();
    device["owner"] = "";                 // Always blank in setup mode
    device["timezone"] = defaultTimezone; // Default from config.h

    // WiFi section with blanks for user input
    JsonObject wifi = device["wifi"].to<JsonObject>();
    wifi["ssid"] = "";
    wifi["password"] = "";

    // Optional printer settings with current runtime config defaults
    const RuntimeConfig &config = getRuntimeConfig();
    device["printerTxPin"] = config.printerTxPin;
    device["printerRxPin"] = config.printerRxPin;
    device["printerDtrPin"] = config.printerDtrPin;

    // Minimal GPIO info for the printer pin selector
    JsonObject gpio = setupDoc["gpio"].to<JsonObject>();
    JsonArray safePins = gpio["safePins"].to<JsonArray>();
    JsonObject pinDescriptions = gpio["pinDescriptions"].to<JsonObject>();

    // Only include safe pins for setup
    for (int i = 0; i < BOARD_GPIO_MAP_SIZE; i++)
    {
        int pin = BOARD_GPIO_MAP[i].pin;
        if (isSafeGPIO(pin))
        {
            safePins.add(pin);
            pinDescriptions[String(pin)] = BOARD_GPIO_MAP[i].description;
        }
    }

    String response;
    serializeJson(setupDoc, response);

    AsyncWebServerResponse *res = request->beginResponse(200, "application/json", response);
    res->addHeader("Access-Control-Allow-Origin", "*");
    request->send(res);

    LOG_VERBOSE("WEB", "Setup configuration sent (minimal for AP-STA mode)");
}

void handleTestMQTT(AsyncWebServerRequest *request)
{
    extern String getRequestBody(AsyncWebServerRequest * request);

    LOG_VERBOSE("WEB", "handleTestMQTT() called - testing MQTT connection");

    // Get request body
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        LOG_ERROR("WEB", "MQTT test request body is empty");
        sendErrorResponse(request, 400, "Request body is empty");
        return;
    }

    // Parse JSON - small buffer for test data
    JsonDocument doc;

    LOG_VERBOSE("WEB", "MQTT test POST body: %s", body.c_str());

    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        LOG_ERROR("WEB", "MQTT test JSON deserialization failed: %s", error.c_str());
        sendValidationError(request, ValidationResult(false, "Invalid JSON format: " + String(error.c_str())));
        return;
    }

    // Extract test parameters
    if (!doc["server"].is<const char*>() || !doc["port"].is<int>() || !doc["username"].is<const char*>())
    {
        sendValidationError(request, ValidationResult(false, "Missing required MQTT test fields (server, port, username)"));
        return;
    }

    String server = doc["server"];
    int port = doc["port"];
    String username = doc["username"];
    String password = doc["password"] | "";

    // If no password provided in test request, use stored password from config
    if (password.length() == 0)
    {
        const RuntimeConfig &config = getRuntimeConfig();
        password = config.mqttPassword;
    }

    if (server.length() == 0 || port < 1 || port > 65535)
    {
        sendValidationError(request, ValidationResult(false, "Invalid MQTT test parameters"));
        return;
    }

    // Build test credentials struct
    MQTTTestCredentials testCreds;
    testCreds.server = server;
    testCreds.port = port;
    testCreds.username = username;
    testCreds.password = password;

    // Use MQTTManager singleton to test connection
    // This is thread-safe and properly handles state save/restore
    String errorMsg;
    bool success = MQTTManager::instance().testConnection(testCreds, errorMsg);

    // Build response
    JsonDocument response;

    if (success)
    {
        response["success"] = true;
        response["message"] = "Successfully connected to MQTT broker";

        String responseStr;
        serializeJson(response, responseStr);

        AsyncWebServerResponse *res = request->beginResponse(200, "application/json", responseStr);
        res->addHeader("Access-Control-Allow-Origin", "*");
        request->send(res);
    }
    else
    {
        LOG_WARNING("WEB", "MQTT test connection failed: %s", errorMsg.c_str());
        sendErrorResponse(request, 400, errorMsg);
    }
}

void handleTestChatGPT(AsyncWebServerRequest *request)
{
    extern String getRequestBody(AsyncWebServerRequest * request);

    LOG_VERBOSE("WEB", "handleTestChatGPT() called - testing ChatGPT API token");

    // Read body
    String body = getRequestBody(request);
    if (body.length() == 0)
    {
        sendErrorResponse(request, 400, "Request body is empty");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err)
    {
        sendValidationError(request, ValidationResult(false, "Invalid JSON format: " + String(err.c_str())));
        return;
    }

    if (!doc["token"].is<const char *>())
    {
        sendValidationError(request, ValidationResult(false, "Missing required field 'token'"));
        return;
    }

    String token = doc["token"].as<String>();
    token.trim();
    if (token.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "API token cannot be blank"));
        return;
    }

    // Build bearer and call models endpoint
    String bearer = String("Bearer ") + token;
    String response = fetchFromAPIWithBearer(chatgptApiTestEndpoint, bearer, apiUserAgent, 6000);

    JsonDocument out;
    if (response.length() > 0)
    {
        out["success"] = true;
        String resp;
        serializeJson(out, resp);
        request->send(200, "application/json", resp);
    }
    else
    {
        out["success"] = false;
        out["error"] = "Invalid API key or network error";
        String resp;
        serializeJson(out, resp);
        request->send(401, "application/json", resp);
    }
}
