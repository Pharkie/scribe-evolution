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
        errorResponse["success"] = false;
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

    // WiFi configuration - nested under device to match settings structure
    JsonObject wifi = device.createNestedObject("wifi");
    wifi["ssid"] = config.wifiSSID;
    wifi["password"] = maskSecret(config.wifiPassword);
    wifi["connect_timeout"] = config.wifiConnectTimeoutMs;

    // Include fallback AP details for client use - always available regardless of current mode
    wifi["fallback_ap_ssid"] = fallbackAPSSID;
    wifi["fallback_ap_password"] = fallbackAPPassword;
    // Always provide mDNS hostname as it's consistent and preferred
    wifi["fallback_ap_mdns"] = String(getMdnsHostname()) + ".local";

    // WiFi status information
    JsonObject wifiStatus = wifi.createNestedObject("status");
    wifiStatus["connected"] = (WiFi.status() == WL_CONNECTED);
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

    // Memos configuration
    JsonObject memos = configDoc.createNestedObject("memos");
    
    // Load memo content from NVS
    Preferences prefs;
    if (prefs.begin("scribe-app", true)) // read-only
    {
        const char* memoKeys[] = {NVS_MEMO_1, NVS_MEMO_2, NVS_MEMO_3, NVS_MEMO_4};
        const char* defaultMemos[] = {
            "Good morning! Today is [weekday], [date]. Current time: [time]",
            "Random task: [pick:Call Mum|Do Laundry|Walk Dog|Buy Groceries|Clean Kitchen]",
            "Lucky numbers: [dice:10], [dice:20], [dice:6]. Coin flip: [coin]",
            "Device info - Uptime: [uptime], IP: [ip], mDNS: [mdns]"
        };
        
        memos["memo1"] = prefs.getString(memoKeys[0], defaultMemos[0]);
        memos["memo2"] = prefs.getString(memoKeys[1], defaultMemos[1]);
        memos["memo3"] = prefs.getString(memoKeys[2], defaultMemos[2]);
        memos["memo4"] = prefs.getString(memoKeys[3], defaultMemos[3]);
        
        prefs.end();
    }
    else
    {
        // Fallback to defaults if NVS access fails
        memos["memo1"] = "Good morning! Today is [weekday], [date]. Current time: [time]";
        memos["memo2"] = "Random task: [pick:Call Mum|Do Laundry|Walk Dog|Buy Groceries|Clean Kitchen]";
        memos["memo3"] = "Lucky numbers: [dice:10], [dice:20], [dice:6]. Coin flip: [coin]";
        memos["memo4"] = "Device info - Uptime: [uptime], IP: [ip], mDNS: [mdns]";
    }

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
        button["gpio"] = defaultButtons[i].gpio;

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
    leds["pin"] = config.ledPin;
    leds["count"] = config.ledCount;
    leds["brightness"] = config.ledBrightness;
    leds["refreshRate"] = config.ledRefreshRate;

    // Add per-effect autonomous configurations
    saveLedEffectsToJson(leds, config.ledEffects);
#endif

    // Feed watchdog before JSON serialization
    delay(1);

    String configString;
    size_t jsonSize = serializeJson(configDoc, configString);

    if (jsonSize == 0)
    {
        LOG_ERROR("WEB", "Failed to serialize config JSON");
        DynamicJsonDocument errorDoc(256);
        errorDoc["success"] = false;
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
    DeserializationError error = deserializeJson(doc, body);
    if (error)
    {
        sendValidationError(request, ValidationResult(false, "Invalid JSON format: " + String(error.c_str())));
        return;
    }

    // Validate required top-level sections exist (matching new structure)
    const char *requiredSections[] = {"device", "mqtt", "unbiddenInk", "memos", "buttons", "leds"};
    for (int i = 0; i < 6; i++)
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

    // Validate and save memo configuration
    JsonObject memos = doc["memos"];
    if (memos)
    {
        const char* memoFields[] = {"memo1", "memo2", "memo3", "memo4"};
        
        // Validate all memo content
        for (int i = 0; i < MEMO_COUNT; i++)
        {
            if (memos.containsKey(memoFields[i]))
            {
                String memoContent = memos[memoFields[i]].as<String>();
                ValidationResult validation = validateMessage(memoContent, MEMO_MAX_LENGTH);
                if (!validation.isValid)
                {
                    sendValidationError(request, validation);
                    return;
                }
            }
        }
        
        // Save memos to NVS
        Preferences prefs;
        if (prefs.begin("scribe-app", false)) // read-write
        {
            const char* memoKeys[] = {NVS_MEMO_1, NVS_MEMO_2, NVS_MEMO_3, NVS_MEMO_4};
            
            for (int i = 0; i < MEMO_COUNT; i++)
            {
                if (memos.containsKey(memoFields[i]))
                {
                    String memoContent = memos[memoFields[i]].as<String>();
                    prefs.putString(memoKeys[i], memoContent);
                    LOG_VERBOSE("WEB", "Saved memo %d: %s", i + 1, memoContent.substring(0, 50).c_str());
                }
            }
            
            prefs.end();
            LOG_NOTICE("WEB", "Memo configuration saved successfully");
        }
        else
        {
            LOG_ERROR("WEB", "Failed to open NVS for memo saving");
            sendErrorResponse(request, "Failed to save memo configuration", 500);
            return;
        }
    }

    // Validate button configuration (exactly 4 buttons)
    JsonObject buttons = doc["buttons"];
    const char *buttonKeys[] = {"button1", "button2", "button3", "button4"};
    const char *validActions[] = {"/api/joke", "/api/riddle", "/api/quote", "/api/quiz", "/api/news", "/api/character-test", "/api/unbidden-ink", "MEMO1", "MEMO2", "MEMO3", "MEMO4", ""};

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
        for (int j = 0; j < 8; j++) // Include empty string for short actions
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
        for (int j = 0; j < 8; j++) // Include empty string for long actions
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

    // Validate LED pin (ESP32-C3 specific)
    if (ledPin < 0 || ledPin > 10)
    {
        sendValidationError(request, ValidationResult(false, "LED pin must be between 0 and 10 (ESP32-C3 compatible pins)"));
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
