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
#include "../core/config_loader.h"
#include "../core/config_utils.h"
#include "../core/led_config_loader.h"
#include "../core/logging.h"
#include "../core/printer_discovery.h"
#include "../utils/time_utils.h"
#include "../core/network.h"
#include "../core/mqtt_handler.h"
#include "../content/unbidden_ink.h"
#if ENABLE_LEDS
#include "../leds/LedEffects.h"
#include <FastLED.h>
#endif
#include <ArduinoJson.h>
#include <LittleFS.h>

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
            sendErrorResponse(request, 500, "Failed to create configuration file.");
            return;
        }
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        LOG_ERROR("WEB", "config.json still not accessible after creation");
        sendErrorResponse(request, 500, "Configuration file access error.");
        return;
    }

    // Parse config.json
    DynamicJsonDocument userConfig(largeJsonDocumentSize);
    DeserializationError error = deserializeJson(userConfig, configFile);
    configFile.close();

    if (error)
    {
        LOG_ERROR("WEB", "Failed to parse config.json: %s", error.c_str());
        sendErrorResponse(request, 500, "Invalid config.json format");
        return;
    }

    // Create response with config.json data (with defaults for missing keys)
    DynamicJsonDocument configDoc(largeJsonDocumentSize);

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

    // Add runtime device information
    device["firmware_version"] = getFirmwareVersion();
    device["boot_time"] = getDeviceBootTime();
    device["mdns"] = String(getMdnsHostname()) + ".local";
    device["ip_address"] = WiFi.localIP().toString();
    device["printer_name"] = getLocalPrinterName();
    device["mqtt_topic"] = getLocalPrinterTopic();
    device["type"] = "local";

    // WiFi configuration (empty if not configured in config.json)
    JsonObject wifi = configDoc.createNestedObject("wifi");
    if (userConfig.containsKey("wifi"))
    {
        wifi["ssid"] = userConfig["wifi"]["ssid"] | "";
        wifi["password"] = userConfig["wifi"]["password"] | "";
        wifi["connect_timeout"] = userConfig["wifi"]["connect_timeout"] | 15000;
    }
    else
    {
        wifi["ssid"] = "";
        wifi["password"] = "";
        wifi["connect_timeout"] = 15000;
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
    else if (userConfig.containsKey("unbiddenInk"))
    {
        configDoc["unbiddenInk"] = userConfig["unbiddenInk"];
        // Re-initialize unbidden ink after settings change
        initializeUnbiddenInk();
    }
    if (userConfig.containsKey("buttons"))
    {
        configDoc["buttons"] = userConfig["buttons"];
    }

#if ENABLE_LEDS
    // LED configuration (copy from config.json with defaults)
    if (userConfig.containsKey("leds"))
    {
        configDoc["leds"] = userConfig["leds"];
    }
    else
    {
        // Provide defaults if leds section doesn't exist
        JsonObject leds = configDoc.createNestedObject("leds");
        leds["pin"] = DEFAULT_LED_PIN;
        leds["count"] = DEFAULT_LED_COUNT;
        leds["brightness"] = DEFAULT_LED_BRIGHTNESS;
        leds["refreshRate"] = DEFAULT_LED_REFRESH_RATE;

        // Add autonomous per-effect configuration structure
        LedEffectsConfig defaultEffects = getDefaultLedEffectsConfig();
        saveLedEffectsToJson(leds, defaultEffects);
    }
#endif

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

    // Validate required top-level sections exist
    const char *requiredSections[] = {"device", "wifi", "mqtt", "apis", "validation", "unbiddenInk", "buttons", "leds"};
    for (int i = 0; i < 8; i++)
    {
        if (!doc.containsKey(requiredSections[i]))
        {
            sendValidationError(request, ValidationResult(false, "Missing required section: " + String(requiredSections[i])));
            return;
        }
    }

    // Validate device configuration
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

    // Validate WiFi configuration
    JsonObject wifi = doc["wifi"];
    if (!wifi.containsKey("ssid") || !wifi.containsKey("password"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required WiFi configuration fields"));
        return;
    }

    String ssid = wifi["ssid"];
    String password = wifi["password"];
    if (ssid.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "WiFi SSID cannot be empty"));
        return;
    }
    if (password.length() == 0)
    {
        sendValidationError(request, ValidationResult(false, "WiFi password cannot be empty"));
        return;
    } // Validate MQTT configuration
    JsonObject mqtt = doc["mqtt"];
    if (!mqtt.containsKey("server") || !mqtt.containsKey("port") ||
        !mqtt.containsKey("username") || !mqtt.containsKey("password"))
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

    // Validate APIs configuration (only user-configurable fields)
    JsonObject apis = doc["apis"];
    if (!apis.containsKey("chatgptApiToken"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required ChatGPT API token"));
        return;
    }

    // Validate validation configuration (only maxCharacters)
    JsonObject validation = doc["validation"];
    if (!validation.containsKey("maxCharacters"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required maxCharacters field"));
        return;
    }

    int maxChars = validation["maxCharacters"];
    if (maxChars < 100 || maxChars > 5000)
    {
        sendValidationError(request, ValidationResult(false, "Max characters must be between 100 and 5000"));
        return;
    }

    // Validate unbidden ink configuration
    JsonObject unbiddenInk = doc["unbiddenInk"];
    if (!unbiddenInk.containsKey("enabled") || !unbiddenInk.containsKey("startHour") ||
        !unbiddenInk.containsKey("endHour") || !unbiddenInk.containsKey("frequencyMinutes"))
    {
        sendValidationError(request, ValidationResult(false, "Missing required Unbidden Ink configuration fields"));
        return;
    }

    bool enabled = unbiddenInk["enabled"];
    int startHour = unbiddenInk["startHour"];
    int endHour = unbiddenInk["endHour"];
    int frequency = unbiddenInk["frequencyMinutes"];

    if (startHour < 0 || startHour > 23 || endHour < 0 || endHour > 23)
    {
        sendValidationError(request, ValidationResult(false, "Hours must be between 0 and 23"));
        return;
    }

    if (startHour >= endHour)
    {
        sendValidationError(request, ValidationResult(false, "Start hour must be before end hour"));
        return;
    }

    if (frequency < 15 || frequency > 480)
    {
        sendValidationError(request, ValidationResult(false, "Frequency must be between 15 minutes and 8 hours"));
        return;
    }

    if (enabled && (!unbiddenInk.containsKey("prompt") || String((const char *)unbiddenInk["prompt"]).length() == 0))
    {
        sendValidationError(request, ValidationResult(false, "Prompt required when Unbidden Ink is enabled"));
        return;
    }

    // Validate button configuration (exactly 4 buttons)
    JsonObject buttons = doc["buttons"];
    const char *buttonKeys[] = {"button1", "button2", "button3", "button4"};
    const char *validActions[] = {"/api/joke", "/api/riddle", "/api/quote", "/api/quiz", "/api/news", "/api/print-test", "/api/unbidden-ink", ""};

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

        // Add more effect validations as needed...
    }
#endif

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        LOG_ERROR("WEB", "Failed to open config.json for writing");
        sendErrorResponse(request, 500, "Failed to save configuration");
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

#if ENABLE_LEDS
        // Reinitialize LED system with new configuration
        const RuntimeConfig &config = getRuntimeConfig();
        if (ledEffects.reinitialize(config.ledPin, config.ledCount, config.ledBrightness,
                                    config.ledRefreshRate, config.ledEffects))
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
    }

    sendSuccessResponse(request, "Configuration saved successfully");

    if (isAPMode())
    {
        LOG_NOTICE("WEB", "Device in AP mode - rebooting to connect to new WiFi configuration");
        delay(1000);
        ESP.restart();
    }
}
