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
#include "../core/config.h"
#include "../core/nvs_keys.h"
#include "../core/config_loader.h"
#include "../core/config_utils.h"
#include "../core/led_config_loader.h"
#include "../core/logging.h"
#include "../core/printer_discovery.h"
#include "../utils/time_utils.h"
#include "../core/network.h"
#include "../core/mqtt_handler.h"

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
#include "../content/unbidden_ink.h"
#include "../hardware/hardware_buttons.h"
#if ENABLE_LEDS
#include "../leds/LedEffects.h"
#include <FastLED.h>
#endif
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>

// External references
extern PubSubClient mqttClient;

#if ENABLE_LEDS
#include "../leds/LedEffects.h"
extern LedEffects ledEffects;
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
        DynamicJsonDocument errorResponse(256);
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
    DynamicJsonDocument configDoc(largeJsonDocumentSize);

    // Device configuration - main section matching settings.html
    JsonObject device = configDoc.createNestedObject("device");
    device["owner"] = config.deviceOwner;
    device["timezone"] = config.timezone;

    // Move maxCharacters from validation to device section
    device["maxCharacters"] = config.maxCharacters;

    // Add runtime device information
    device["firmware_version"] = getFirmwareVersion();
    device["boot_time"] = getDeviceBootTime();
    device["mdns"] = String(getMdnsHostname()) + ".local";
    device["ip_address"] = WiFi.localIP().toString();
    device["printer_name"] = getLocalPrinterName();
    device["mqtt_topic"] = getLocalPrinterTopic();
    device["type"] = "local";
    
    // Hardware GPIO configuration
    device["printerTxPin"] = config.printerTxPin;

    // WiFi configuration - nested under device to match settings structure
    JsonObject wifi = device.createNestedObject("wifi");
    
    // In AP mode, encourage fresh setup by showing generic placeholders
    if (isAPMode()) {
        wifi["ssid"] = "AP_MODE";
        wifi["password"] = ""; // Blank to encourage manual entry
    } else {
        wifi["ssid"] = config.wifiSSID;
        wifi["password"] = maskSecret(config.wifiPassword);
    }
    wifi["connect_timeout"] = config.wifiConnectTimeoutMs;

    // Include fallback AP details for client use - always available regardless of current mode
    wifi["fallback_ap_ssid"] = fallbackAPSSID;
    wifi["fallback_ap_password"] = fallbackAPPassword;
    // Always provide mDNS hostname as it's consistent and preferred
    wifi["fallback_ap_mdns"] = String(getMdnsHostname()) + ".local";

    // WiFi status information
    JsonObject wifiStatus = wifi.createNestedObject("status");
    wifiStatus["connected"] = (WiFi.status() == WL_CONNECTED);
    wifiStatus["ap_mode"] = isAPMode(); // Indicate if device is in AP setup mode
    wifiStatus["ip_address"] = WiFi.localIP().toString();
    wifiStatus["mac_address"] = WiFi.macAddress();
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
    wifiStatus["signal_strength"] = signalStrength;

    // Feed watchdog after WiFi processing
    delay(1);

    // MQTT configuration - top-level section matching settings.html
    JsonObject mqtt = configDoc.createNestedObject("mqtt");
    mqtt["server"] = config.mqttServer;
    mqtt["port"] = config.mqttPort;
    mqtt["username"] = config.mqttUsername;
    mqtt["password"] = maskSecret(config.mqttPassword);
    // Skip MQTT connection check in AP mode to avoid potential blocking
    mqtt["connected"] = isAPMode() ? false : mqttClient.connected();

    // Unbidden Ink configuration - top-level section matching settings.html
    JsonObject unbiddenInk = configDoc.createNestedObject("unbiddenInk");
    unbiddenInk["enabled"] = config.unbiddenInkEnabled;
    unbiddenInk["startHour"] = config.unbiddenInkStartHour;
    unbiddenInk["endHour"] = config.unbiddenInkEndHour;
    unbiddenInk["frequencyMinutes"] = config.unbiddenInkFrequencyMinutes;
    unbiddenInk["prompt"] = config.unbiddenInkPrompt;
    unbiddenInk["chatgptApiToken"] = maskSecret(config.chatgptApiToken);

    // Add prompt presets for quick selection
    JsonObject prompts = unbiddenInk.createNestedObject("promptPresets");
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
    JsonObject buttons = configDoc.createNestedObject("buttons");

    // Hardware button status information
    buttons["count"] = numHardwareButtons;
    buttons["debounce_time"] = buttonDebounceMs;
    buttons["long_press_time"] = buttonLongPressMs;
    buttons["active_low"] = buttonActiveLow;
    buttons["min_interval"] = buttonMinInterval;
    buttons["max_per_minute"] = buttonMaxPerMinute;

    // Button action configuration
    for (int i = 0; i < numHardwareButtons; i++)
    {
        String buttonKey = "button" + String(i + 1);
        JsonObject button = buttons.createNestedObject(buttonKey);

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
    JsonObject leds = configDoc.createNestedObject("leds");
    leds["enabled"] = true;  // LED support is compiled in
    leds["pin"] = config.ledPin;
    leds["count"] = config.ledCount;
    leds["brightness"] = config.ledBrightness;
    leds["refreshRate"] = config.ledRefreshRate;

    // Add effectDefaults structure for frontend LED playground (10-100 scale)
    JsonObject effectDefaults = leds.createNestedObject("effectDefaults");
    
    // Chase Single defaults
    JsonObject chaseSingle = effectDefaults.createNestedObject("chase_single");
    chaseSingle["speed"] = 50;
    chaseSingle["intensity"] = 50;
    chaseSingle["cycles"] = 3;
    JsonArray chaseSingleColors = chaseSingle.createNestedArray("colors");
    chaseSingleColors.add(config.ledEffects.chaseSingle.defaultColor);
    
    // Chase Multi defaults  
    JsonObject chaseMulti = effectDefaults.createNestedObject("chase_multi");
    chaseMulti["speed"] = 50;
    chaseMulti["intensity"] = 50;
    chaseMulti["cycles"] = 3;
    JsonArray chaseMultiColors = chaseMulti.createNestedArray("colors");
    chaseMultiColors.add(config.ledEffects.chaseMulti.color1);
    chaseMultiColors.add(config.ledEffects.chaseMulti.color2);
    chaseMultiColors.add(config.ledEffects.chaseMulti.color3);
    
    // Matrix defaults
    JsonObject matrix = effectDefaults.createNestedObject("matrix");
    matrix["speed"] = 50;
    matrix["intensity"] = 50;
    matrix["cycles"] = 3;
    JsonArray matrixColors = matrix.createNestedArray("colors");
    matrixColors.add(config.ledEffects.matrix.defaultColor);
    
    // Twinkle defaults
    JsonObject twinkle = effectDefaults.createNestedObject("twinkle");
    twinkle["speed"] = 50;
    twinkle["intensity"] = 50;
    twinkle["cycles"] = 3;
    JsonArray twinkleColors = twinkle.createNestedArray("colors");
    twinkleColors.add(config.ledEffects.twinkle.defaultColor);
    
    // Pulse defaults
    JsonObject pulse = effectDefaults.createNestedObject("pulse");
    pulse["speed"] = 50;
    pulse["intensity"] = 50;
    pulse["cycles"] = 3;
    JsonArray pulseColors = pulse.createNestedArray("colors");
    pulseColors.add(config.ledEffects.pulse.defaultColor);
    
    // Rainbow defaults  
    JsonObject rainbow = effectDefaults.createNestedObject("rainbow");
    rainbow["speed"] = 50;
    rainbow["intensity"] = 50;
    rainbow["cycles"] = 3;
    JsonArray rainbowColors = rainbow.createNestedArray("colors");
    rainbowColors.add("#ff0000"); // Rainbow doesn't use colors but needs array
#else
    // LEDs disabled at compile time - provide minimal config to inform frontend
    JsonObject leds = configDoc.createNestedObject("leds");
    leds["enabled"] = false;  // LED support is NOT compiled in
#endif

    // GPIO information for frontend validation
    JsonObject gpio = configDoc.createNestedObject("gpio");
    JsonArray availablePins = gpio.createNestedArray("availablePins");
    JsonArray safePins = gpio.createNestedArray("safePins");
    JsonObject pinDescriptions = gpio.createNestedObject("pinDescriptions");
    
    LOG_VERBOSE("CONFIG", "Adding GPIO info, ESP32C3_GPIO_COUNT: %d", ESP32C3_GPIO_COUNT);
    
    // Add all ESP32-C3 GPIO information
    for (int i = 0; i < ESP32C3_GPIO_COUNT; i++) {
        int pin = ESP32C3_GPIO_MAP[i].pin;
        availablePins.add(pin);
        pinDescriptions[String(pin)] = ESP32C3_GPIO_MAP[i].description;
        
        if (isSafeGPIO(pin)) {
            safePins.add(pin);
            LOG_VERBOSE("CONFIG", "Added safe GPIO pin: %d", pin);
        }
    }
    
    LOG_VERBOSE("CONFIG", "GPIO info complete - %d available pins, %d safe pins", availablePins.size(), safePins.size());

    // Check if JSON document has overflowed before serialization
    if (configDoc.overflowed()) {
        LOG_ERROR("CONFIG", "JSON document overflow detected! Size: %d bytes, Capacity: %d bytes", 
                  configDoc.memoryUsage(), configDoc.capacity());
        sendErrorResponse(request, 500, "Configuration too large for buffer - increase largeJsonDocumentSize");
        return;
    }

    // Feed watchdog before JSON serialization
    delay(1);

    String configString;
    size_t jsonSize = serializeJson(configDoc, configString);
    
    LOG_VERBOSE("CONFIG", "JSON serialized: %d bytes, buffer usage: %d/%d bytes", 
                jsonSize, configDoc.memoryUsage(), configDoc.capacity());

    if (jsonSize == 0)
    {
        LOG_ERROR("WEB", "Failed to serialize config JSON");
        DynamicJsonDocument errorDoc(256);
        errorDoc["error"] = "JSON serialization failed";
        String errorString;
        serializeJson(errorDoc, errorString);
        request->send(500, "application/json", errorString);
        return;
    }

    LOG_VERBOSE("WEB", "Configuration from NVS, returning %d bytes", configString.length());
    request->send(200, "application/json", configString);
}

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
    DynamicJsonDocument doc(largeJsonDocumentSize);
    
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

    // Validate required top-level sections exist (memos now handled separately)
    const char *requiredSections[] = {"device", "mqtt", "unbiddenInk", "buttons", "leds"};
    for (int i = 0; i < 5; i++)
    {
        if (!doc.containsKey(requiredSections[i]))
        {
            sendValidationError(request, ValidationResult(false, "Missing required section: " + String(requiredSections[i])));
            return;
        }
    }

    // Create new runtime configuration from JSON
    RuntimeConfig newConfig;

    // Validate and extract device configuration
    JsonObject device = doc["device"];
    if (!device.containsKey("owner") || !device.containsKey("timezone"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required device configuration fields"));
        return;
    }

    String owner = device["owner"];
    String timezone = device["timezone"];
    if (owner.length() == 0 || timezone.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "Device owner and timezone cannot be empty"));
        return;
    }
    newConfig.deviceOwner = owner;
    newConfig.timezone = timezone;
    
    // Parse printer TX GPIO configuration
    if (device.containsKey("printerTxPin")) {
        int printerTxPin = device["printerTxPin"];
        if (!isValidGPIO(printerTxPin) || !isSafeGPIO(printerTxPin)) {
            sendValidationError(request, ValidationResult(false, "Invalid printer TX GPIO pin"));
            return;
        }
        newConfig.printerTxPin = printerTxPin;
    } else {
        newConfig.printerTxPin = defaultPrinterTxPin;
    }

    // maxCharacters is now in device section, but remains hardcoded from config.h
    newConfig.maxCharacters = maxCharacters;

    // Validate WiFi configuration (nested under device)
    JsonObject wifi = device["wifi"];
    if (!wifi.containsKey("ssid"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required WiFi SSID"));
        return;
    }

    String ssid = wifi["ssid"];
    if (ssid.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "WiFi SSID cannot be empty"));
        return;
    }
    newConfig.wifiSSID = ssid;

    // Handle WiFi password - preserve existing if not provided (masked field)
    if (wifi.containsKey("password"))
    {
        String password = wifi["password"];
        if (password.length() == 0)
        {
            sendValidationError(request, ValidationResult(false, "WiFi password cannot be empty when provided"));
            return;
        }
        newConfig.wifiPassword = password;
    }
    else
    {
        // Preserve existing WiFi password when not provided (masked field not changed)
        const RuntimeConfig &currentConfig = getRuntimeConfig();
        newConfig.wifiPassword = currentConfig.wifiPassword;
        LOG_VERBOSE("WEB", "WiFi password not provided, preserving existing value");
    }

    newConfig.wifiConnectTimeoutMs = wifi["connect_timeout"] | wifiConnectTimeoutMs;

    // MQTT configuration (top-level section)
    JsonObject mqtt = doc["mqtt"];
    if (!mqtt.containsKey("server") || !mqtt.containsKey("port") || !mqtt.containsKey("username"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required MQTT configuration fields"));
        return;
    }

    int port = mqtt["port"];
    if (port < 1 || port > 65535)
    {
        sendValidationError(request, ValidationResult(false, "MQTT port must be between 1 and 65535"));
        return;
    }
    newConfig.mqttServer = mqtt["server"].as<const char *>();
    newConfig.mqttPort = port;
    newConfig.mqttUsername = mqtt["username"].as<const char *>();

    // Handle MQTT password - preserve existing if not provided (masked field)
    if (mqtt.containsKey("password"))
    {
        newConfig.mqttPassword = mqtt["password"].as<const char *>();
    }
    else
    {
        // Preserve existing MQTT password when not provided (masked field not changed)
        const RuntimeConfig &currentConfig = getRuntimeConfig();
        newConfig.mqttPassword = currentConfig.mqttPassword;
        LOG_VERBOSE("WEB", "MQTT password not provided, preserving existing value");
    }

    // Validate unbidden ink configuration
    JsonObject unbiddenInk = doc["unbiddenInk"];
    if (!unbiddenInk.containsKey("enabled"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required Unbidden Ink 'enabled' field"));
        return;
    }

    bool enabled = unbiddenInk["enabled"];

    // Only validate time fields if unbidden ink is enabled
    if (enabled)
    {
        if (!unbiddenInk.containsKey("startHour") || !unbiddenInk.containsKey("endHour") ||
            !unbiddenInk.containsKey("frequencyMinutes"))
        {
            sendValidationError(request, ValidationResult(false, "Missing required Unbidden Ink time configuration fields"));
            return;
        }

        int startHour = unbiddenInk["startHour"];
        int endHour = unbiddenInk["endHour"];
        int frequency = unbiddenInk["frequencyMinutes"];

        if (startHour < 0 || startHour > 24 || endHour < 0 || endHour > 24)
        {
            sendValidationError(request, ValidationResult(false, "Hours must be between 0 and 24"));
            return;
        }

        if (startHour >= endHour)
        {
            sendValidationError(request, ValidationResult(false, "Start hour must be before end hour"));
            return;
        }

        if (frequency < minUnbiddenInkFrequencyMinutes || frequency > maxUnbiddenInkFrequencyMinutes)
        {
            sendValidationError(request, ValidationResult(false, "Frequency must be between 15 minutes and 8 hours"));
            return;
        }

        if (!unbiddenInk.containsKey("prompt") || String((const char *)unbiddenInk["prompt"]).length() == 0)
        {
            sendValidationError(request, ValidationResult(false, "Prompt required when Unbidden Ink is enabled"));
            return;
        }
    }

    // Apply validated unbidden ink settings to newConfig
    newConfig.unbiddenInkEnabled = enabled;
    if (enabled)
    {
        // Only save time values if they were validated (when enabled=true)
        newConfig.unbiddenInkStartHour = unbiddenInk["startHour"];
        newConfig.unbiddenInkEndHour = unbiddenInk["endHour"];
        newConfig.unbiddenInkFrequencyMinutes = unbiddenInk["frequencyMinutes"];
        newConfig.unbiddenInkPrompt = String((const char *)unbiddenInk["prompt"]);
    }
    else
    {
        // When disabled, preserve existing values from current config to avoid garbage
        RuntimeConfig currentConfig = getRuntimeConfig();
        newConfig.unbiddenInkStartHour = currentConfig.unbiddenInkStartHour;
        newConfig.unbiddenInkEndHour = currentConfig.unbiddenInkEndHour;
        newConfig.unbiddenInkFrequencyMinutes = currentConfig.unbiddenInkFrequencyMinutes;
        newConfig.unbiddenInkPrompt = currentConfig.unbiddenInkPrompt;
    }

    // Handle ChatGPT API token - preserve existing if not provided (masked field)
    if (unbiddenInk.containsKey("chatgptApiToken"))
    {
        newConfig.chatgptApiToken = unbiddenInk["chatgptApiToken"].as<const char *>();
    }
    else
    {
        // Preserve existing ChatGPT API token when not provided (masked field not changed)
        const RuntimeConfig &currentConfig = getRuntimeConfig();
        newConfig.chatgptApiToken = currentConfig.chatgptApiToken;
        LOG_VERBOSE("WEB", "ChatGPT API token not provided, preserving existing value");
    }

    // Non-user configurable APIs remain as constants
    newConfig.jokeAPI = jokeAPI;
    newConfig.quoteAPI = quoteAPI;
    newConfig.triviaAPI = triviaAPI;
    newConfig.betterStackToken = betterStackToken;
    newConfig.betterStackEndpoint = betterStackEndpoint;
    newConfig.chatgptApiEndpoint = chatgptApiEndpoint;

    // Memos are now handled by separate /api/memos endpoint

    // Validate button configuration (exactly 4 buttons)
    JsonObject buttons = doc["buttons"];
    const char *buttonKeys[] = {"button1", "button2", "button3", "button4"};
    const char *validActions[] = {"JOKE", "RIDDLE", "QUOTE", "QUIZ", "NEWS", "CHARACTER_TEST", "UNBIDDEN_INK", "MEMO1", "MEMO2", "MEMO3", "MEMO4", ""};

    for (int i = 0; i < 4; i++)
    {
        if (!buttons.containsKey(buttonKeys[i]))
        {
            sendValidationError(request, ValidationResult(false, "Missing button configuration: " + String(buttonKeys[i])));
            return;
        }

        JsonObject button = buttons[buttonKeys[i]];
        if (!button.containsKey("shortAction") || !button.containsKey("longAction"))
        {
            sendValidationError(request, ValidationResult(false, "Missing shortAction or longAction for " + String(buttonKeys[i])));
            return;
        }
        
        // Parse and validate button GPIO configuration
        if (button.containsKey("gpio")) {
            int buttonGpio = button["gpio"];
            if (!isValidGPIO(buttonGpio) || !isSafeGPIO(buttonGpio)) {
                sendValidationError(request, ValidationResult(false, "Invalid GPIO pin for " + String(buttonKeys[i])));
                return;
            }
            newConfig.buttonGpios[i] = buttonGpio;
        } else {
            newConfig.buttonGpios[i] = defaultButtons[i].gpio;
        }

        String shortAction = button["shortAction"];
        String longAction = button["longAction"];
        String shortLedEffect = button["shortLedEffect"] | "chase_single"; // Default to chase_single
        String longLedEffect = button["longLedEffect"] | "chase_single";   // Default to chase_single

        // MQTT topics are optional - if present, they should be strings
        if (button.containsKey("shortMqttTopic") && !button["shortMqttTopic"].is<String>())
        {
            sendValidationError(request, ValidationResult(false, "shortMqttTopic must be a string for " + String(buttonKeys[i])));
            return;
        }

        if (button.containsKey("longMqttTopic") && !button["longMqttTopic"].is<String>())
        {
            sendValidationError(request, ValidationResult(false, "longMqttTopic must be a string for " + String(buttonKeys[i])));
            return;
        }

        // Validate shortAction (can now be empty or valid action)
        bool validShortAction = false;
        for (int j = 0; j < 12; j++) // 12 elements in validActions array
        {
            if (shortAction == validActions[j])
            {
                validShortAction = true;
                break;
            }
        }
        if (!validShortAction)
        {
            sendValidationError(request, ValidationResult(false, "Invalid short action for " + String(buttonKeys[i]) + ": " + shortAction));
            return;
        }

        // Validate longAction (optional, can be empty or valid action)
        bool validLongAction = false;
        for (int j = 0; j < 12; j++) // 12 elements in validActions array
        {
            if (longAction == validActions[j])
            {
                validLongAction = true;
                break;
            }
        }
        if (!validLongAction)
        {
            sendValidationError(request, ValidationResult(false, "Invalid long action for " + String(buttonKeys[i]) + ": " + longAction));
            return;
        }

        // Validate LED effects (must be valid effect names or "none")
        const char *validLedEffects[] = {"chase_single", "chase_multi", "rainbow", "twinkle", "pulse", "matrix", "none"};
        bool validShortLedEffect = false;
        bool validLongLedEffect = false;

        for (int j = 0; j < 7; j++)
        {
            if (shortLedEffect == validLedEffects[j])
                validShortLedEffect = true;
            if (longLedEffect == validLedEffects[j])
                validLongLedEffect = true;
        }

        if (!validShortLedEffect)
        {
            sendValidationError(request, ValidationResult(false, "Invalid short LED effect for " + String(buttonKeys[i]) + ": " + shortLedEffect));
            return;
        }

        if (!validLongLedEffect)
        {
            sendValidationError(request, ValidationResult(false, "Invalid long LED effect for " + String(buttonKeys[i]) + ": " + longLedEffect));
            return;
        }

        // Store validated button configuration
        newConfig.buttonShortActions[i] = shortAction;
        newConfig.buttonLongActions[i] = longAction;
        newConfig.buttonShortMqttTopics[i] = button["shortMqttTopic"] | "";
        newConfig.buttonLongMqttTopics[i] = button["longMqttTopic"] | "";

        // Store validated LED effects
        newConfig.buttonShortLedEffects[i] = shortLedEffect;
        newConfig.buttonLongLedEffects[i] = longLedEffect;
    }

#if ENABLE_LEDS
    // Validate LED configuration with new autonomous structure
    JsonObject leds = doc["leds"];
    if (!leds.containsKey("pin") || !leds.containsKey("count") ||
        !leds.containsKey("brightness") || !leds.containsKey("refreshRate"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required LED hardware configuration fields"));
        return;
    }

    int ledPin = leds["pin"];
    int ledCount = leds["count"];
    int ledBrightness = leds["brightness"];
    int ledRefreshRate = leds["refreshRate"];

    // Validate LED pin using GPIO validation from config.h
    if (!isValidGPIO(ledPin))
    {
        sendValidationError(request, ValidationResult(false, "Invalid GPIO pin " + String(ledPin) + " for LEDs. " + String(getGPIODescription(ledPin))));
        return;
    }
    if (!isSafeGPIO(ledPin))
    {
        sendValidationError(request, ValidationResult(false, "GPIO " + String(ledPin) + " is not safe to use: " + String(getGPIODescription(ledPin))));
        return;
    }

    // Validate LED count
    if (ledCount < 1 || ledCount > 300)
    {
        sendValidationError(request, ValidationResult(false, "LED count must be between 1 and 300"));
        return;
    }

    // Validate LED brightness
    if (ledBrightness < 0 || ledBrightness > 255)
    {
        sendValidationError(request, ValidationResult(false, "LED brightness must be between 0 and 255"));
        return;
    }

    // Validate refresh rate
    if (ledRefreshRate < 10 || ledRefreshRate > 120)
    {
        sendValidationError(request, ValidationResult(false, "LED refresh rate must be between 10 and 120 Hz"));
        return;
    }

    // Validate autonomous per-effect configurations if present
    JsonObject effects = leds["effects"];
    if (!effects.isNull())
    {
        // Validate Chase Single if present
        JsonObject chaseSingle = effects["chaseSingle"];
        if (!chaseSingle.isNull())
        {
            int speed = chaseSingle["speed"];
            if (speed < 1 || speed > 100)
            {
                sendValidationError(request, ValidationResult(false, "Chase Single speed must be between 1 and 100"));
                return;
            }
        }

        // Validate Chase Multi if present
        JsonObject chaseMulti = effects["chaseMulti"];
        if (!chaseMulti.isNull())
        {
            int speed = chaseMulti["speed"];
            if (speed < 1 || speed > 100)
            {
                sendValidationError(request, ValidationResult(false, "Chase Multi speed must be between 1 and 100"));
                return;
            }
        }

        // Validate Matrix if present
        JsonObject matrix = effects["matrix"];
        if (!matrix.isNull())
        {
            int drops = matrix["drops"];
            if (drops < 1 || drops > 20)
            {
                sendValidationError(request, ValidationResult(false, "Matrix drops must be between 1 and 20"));
                return;
            }
        }

        // Validate Twinkle if present
        JsonObject twinkle = effects["twinkle"];
        if (!twinkle.isNull())
        {
            int density = twinkle["density"];
            if (density < 1 || density > 20)
            {
                sendValidationError(request, ValidationResult(false, "Twinkle density must be between 1 and 20"));
                return;
            }
            int fadeSpeed = twinkle["fadeSpeed"];
            if (fadeSpeed < 1 || fadeSpeed > 255)
            {
                sendValidationError(request, ValidationResult(false, "Twinkle fade speed must be between 1 and 255"));
                return;
            }
        }
    }

    // Store LED configuration
    newConfig.ledPin = ledPin;
    newConfig.ledCount = ledCount;
    newConfig.ledBrightness = ledBrightness;
    newConfig.ledRefreshRate = ledRefreshRate;

    // Load LED effects configuration (simplified for now)
    newConfig.ledEffects = getDefaultLedEffectsConfig(); // TODO: Parse effects from JSON
#endif

    // Save configuration to NVS
    if (!saveNVSConfig(newConfig))
    {
        LOG_ERROR("WEB", "Failed to save configuration to NVS");
        sendErrorResponse(request, 500, "Failed to save configuration to NVS");
        return;
    }

    // Update global runtime configuration
    setRuntimeConfig(newConfig);

    // Update MQTT subscription to new device owner topic
    updateMQTTSubscription();

#if ENABLE_LEDS
    // Reinitialize LED system with new configuration
    if (ledEffects.reinitialize(newConfig.ledPin, newConfig.ledCount, newConfig.ledBrightness,
                                newConfig.ledRefreshRate, newConfig.ledEffects))
    {
        LOG_VERBOSE("WEB", "LED system reinitialized with new configuration");

        // Trigger green chase single effect as visual confirmation of successful save
        ledEffects.startEffectCycles("chase_single", 1, CRGB::Green);
        LOG_VERBOSE("WEB", "LED confirmation effect triggered for config save");
    }
    else
    {
        LOG_WARNING("WEB", "Failed to reinitialize LED system with new configuration");
    }
#endif

    sendSuccessResponse(request, "Configuration saved successfully");

    if (isAPMode())
    {
        LOG_NOTICE("WEB", "Device in AP mode - rebooting to connect to new WiFi configuration");
        delay(1000);
        ESP.restart();
    }
}

// ========================================
// MEMOS API ENDPOINTS
// ========================================

void handleMemosGet(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "handleMemosGet() called");

    // Create JSON response with all memos
    DynamicJsonDocument memosDoc(2048);
    
    // Get memo values from NVS
    Preferences prefs;
    if (!prefs.begin("scribe-app", true)) { // read-only
        LOG_ERROR("WEB", "Failed to access NVS for memos");
        sendErrorResponse(request, 500, "Failed to access memo storage");
        return;
    }
    
    memosDoc["memo1"] = prefs.getString(NVS_MEMO_1, "");
    memosDoc["memo2"] = prefs.getString(NVS_MEMO_2, "");
    memosDoc["memo3"] = prefs.getString(NVS_MEMO_3, "");
    memosDoc["memo4"] = prefs.getString(NVS_MEMO_4, "");
    
    prefs.end();
    
    // Check for overflow
    if (memosDoc.overflowed()) {
        LOG_ERROR("MEMOS", "Memos JSON document overflow detected!");
        sendErrorResponse(request, 500, "Memos too large for buffer");
        return;
    }
    
    // Serialize and send
    String memosString;
    size_t jsonSize = serializeJson(memosDoc, memosString);
    
    if (jsonSize == 0) {
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
    if (body.length() == 0) {
        sendErrorResponse(request, 400, "No JSON body provided");
        return;
    }
    
    // Parse JSON
    DynamicJsonDocument memosDoc(2048);
    DeserializationError error = deserializeJson(memosDoc, body);
    if (error) {
        LOG_ERROR("WEB", "Failed to parse memos JSON: %s", error.c_str());
        sendErrorResponse(request, 400, "Invalid JSON format");
        return;
    }
    
    // Validate and save each memo to NVS
    Preferences prefs;
    if (!prefs.begin("scribe-app", false)) { // read-write
        LOG_ERROR("WEB", "Failed to access NVS for memo saving");
        sendErrorResponse(request, 500, "Failed to access memo storage");
        return;
    }
    
    // Save each memo with validation
    const char* memoKeys[] = {NVS_MEMO_1, NVS_MEMO_2, NVS_MEMO_3, NVS_MEMO_4};
    const char* memoNames[] = {"memo1", "memo2", "memo3", "memo4"};
    
    for (int i = 0; i < 4; i++) {
        if (memosDoc.containsKey(memoNames[i])) {
            String memoContent = memosDoc[memoNames[i]].as<String>();
            
            // Validate memo length
            if (memoContent.length() > MEMO_MAX_LENGTH) {
                prefs.end();
                sendErrorResponse(request, 400, 
                    String("Memo ") + String(i+1) + " exceeds maximum length of " + String(MEMO_MAX_LENGTH) + " characters");
                return;
            }
            
            prefs.putString(memoKeys[i], memoContent);
            LOG_VERBOSE("WEB", "Saved memo %d (%d characters)", i+1, memoContent.length());
        }
    }
    
    prefs.end();
    
    sendSuccessResponse(request, "Memos saved successfully");
    LOG_NOTICE("WEB", "All memos saved successfully");
}
