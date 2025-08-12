/**
 * @file config_loader.cpp
 * @brief Implementation of dynamic configuration loader
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "config_loader.h"
#include "config.h"
#include "logging.h"
#include <LittleFS.h>

// Global runtime configuration instance
static RuntimeConfig g_runtimeConfig;
static bool g_configLoaded = false;

bool loadRuntimeConfig()
{
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        LOG_WARNING("CONFIG", "config.json not found, creating from defaults");
        if (createDefaultConfigFile())
        {
            configFile = LittleFS.open("/config.json", "r");
            if (!configFile)
            {
                LOG_ERROR("CONFIG", "Failed to open newly created config.json");
                loadDefaultConfig();
                return false;
            }
        }
        else
        {
            LOG_ERROR("CONFIG", "Failed to create default config.json, using in-memory defaults");
            loadDefaultConfig();
            return false;
        }
    }

    DynamicJsonDocument doc(largeJsonDocumentSize);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        LOG_ERROR("CONFIG", "Failed to parse config.json: %s, using defaults", error.c_str());
        loadDefaultConfig();
        return false;
    }

    // Load device configuration
    JsonObject device = doc["device"];
    g_runtimeConfig.deviceOwner = device["owner"] | defaultDeviceOwner;
    g_runtimeConfig.timezone = device["timezone"] | defaultTimezone;

    // Load WiFi configuration
    JsonObject wifi = doc["wifi"];
    g_runtimeConfig.wifiSSID = wifi["ssid"] | "";
    g_runtimeConfig.wifiPassword = wifi["password"] | "";
    g_runtimeConfig.wifiConnectTimeoutMs = wifi["connect_timeout"] | wifiConnectTimeoutMs;

    // Load MQTT configuration
    JsonObject mqtt = doc["mqtt"];
    g_runtimeConfig.mqttServer = mqtt["server"] | defaultMqttServer;
    g_runtimeConfig.mqttPort = mqtt["port"] | defaultMqttPort;
    g_runtimeConfig.mqttUsername = mqtt["username"] | defaultMqttUsername;
    g_runtimeConfig.mqttPassword = mqtt["password"] | defaultMqttPassword;

    // Load API configuration
    JsonObject apis = doc["apis"];
    g_runtimeConfig.jokeAPI = apis["jokeAPI"] | jokeAPI;
    g_runtimeConfig.quoteAPI = apis["quoteAPI"] | quoteAPI;
    g_runtimeConfig.triviaAPI = apis["triviaAPI"] | triviaAPI;
    g_runtimeConfig.betterStackToken = apis["betterStackToken"] | betterStackToken;
    g_runtimeConfig.betterStackEndpoint = apis["betterStackEndpoint"] | betterStackEndpoint;
    g_runtimeConfig.chatgptApiToken = apis["chatgptApiToken"] | defaultChatgptApiToken;
    g_runtimeConfig.chatgptApiEndpoint = apis["chatgptApiEndpoint"] | chatgptApiEndpoint;

    // Load validation configuration (only maxCharacters)
    JsonObject validation = doc["validation"];
    g_runtimeConfig.maxCharacters = validation["maxCharacters"] | maxCharacters;

    // Load Unbidden Ink configuration
    JsonObject unbiddenInk = doc["unbiddenInk"];
    g_runtimeConfig.unbiddenInkEnabled = unbiddenInk["enabled"] | enableUnbiddenInk;
    g_runtimeConfig.unbiddenInkStartHour = unbiddenInk["startHour"] | unbiddenInkStartHour;
    g_runtimeConfig.unbiddenInkEndHour = unbiddenInk["endHour"] | unbiddenInkEndHour;
    g_runtimeConfig.unbiddenInkFrequencyMinutes = unbiddenInk["frequencyMinutes"] | unbiddenInkFrequencyMinutes;
    g_runtimeConfig.unbiddenInkPrompt = unbiddenInk["prompt"] | "Generate a short, encouraging motivational message to help me stay focused and positive. Keep it brief, uplifting, and practical.";

    // Load Button configuration (exactly 4 buttons)
    JsonObject buttons = doc["buttons"];
    if (buttons.isNull())
    {
        // Use defaults if buttons section is missing
        g_runtimeConfig.buttonShortActions[0] = "/joke";
        g_runtimeConfig.buttonShortActions[1] = "/riddle";
        g_runtimeConfig.buttonShortActions[2] = "/quote";
        g_runtimeConfig.buttonShortActions[3] = "/quiz";
        g_runtimeConfig.buttonLongActions[0] = "/print-test";
        g_runtimeConfig.buttonLongActions[1] = "";
        g_runtimeConfig.buttonLongActions[2] = "";
        g_runtimeConfig.buttonLongActions[3] = "";
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            String buttonKey = "button" + String(i + 1);
            JsonObject button = buttons[buttonKey];
            g_runtimeConfig.buttonShortActions[i] = button["shortAction"] | (i == 0 ? "/joke" : i == 1 ? "/riddle"
                                                                                            : i == 2   ? "/quote"
                                                                                                       : "/quiz");
            g_runtimeConfig.buttonLongActions[i] = button["longAction"] | (i == 0 ? "/print-test" : "");
        }
    }

    g_configLoaded = true;
    return true;
}

void loadDefaultConfig()
{
    // Load device defaults
    g_runtimeConfig.deviceOwner = defaultDeviceOwner;
    g_runtimeConfig.timezone = defaultTimezone;

    // Load WiFi defaults (empty by default, must be configured)
    g_runtimeConfig.wifiSSID = "";
    g_runtimeConfig.wifiPassword = "";
    g_runtimeConfig.wifiConnectTimeoutMs = wifiConnectTimeoutMs;

    // Load defaults from config.h constants
    g_runtimeConfig.mqttServer = defaultMqttServer;
    g_runtimeConfig.mqttPort = defaultMqttPort;
    g_runtimeConfig.mqttUsername = defaultMqttUsername;
    g_runtimeConfig.mqttPassword = defaultMqttPassword;

    g_runtimeConfig.jokeAPI = jokeAPI;
    g_runtimeConfig.quoteAPI = quoteAPI;
    g_runtimeConfig.triviaAPI = triviaAPI;
    g_runtimeConfig.betterStackToken = betterStackToken;
    g_runtimeConfig.betterStackEndpoint = betterStackEndpoint;
    g_runtimeConfig.chatgptApiToken = defaultChatgptApiToken;
    g_runtimeConfig.chatgptApiEndpoint = chatgptApiEndpoint;

    g_runtimeConfig.maxCharacters = maxCharacters;

    g_runtimeConfig.unbiddenInkEnabled = enableUnbiddenInk;
    g_runtimeConfig.unbiddenInkStartHour = unbiddenInkStartHour;
    g_runtimeConfig.unbiddenInkEndHour = unbiddenInkEndHour;
    g_runtimeConfig.unbiddenInkFrequencyMinutes = unbiddenInkFrequencyMinutes;
    g_runtimeConfig.unbiddenInkPrompt = "Generate a short, encouraging motivational message to help me stay focused and positive. Keep it brief, uplifting, and practical.";

    // Load default button configuration
    g_runtimeConfig.buttonShortActions[0] = "/joke";
    g_runtimeConfig.buttonShortActions[1] = "/riddle";
    g_runtimeConfig.buttonShortActions[2] = "/quote";
    g_runtimeConfig.buttonShortActions[3] = "/quiz";
    g_runtimeConfig.buttonLongActions[0] = "/print-test";
    g_runtimeConfig.buttonLongActions[1] = "";
    g_runtimeConfig.buttonLongActions[2] = "";
    g_runtimeConfig.buttonLongActions[3] = "";

    g_configLoaded = true;
    LOG_NOTICE("CONFIG", "Using default configuration from config.h");
}

const RuntimeConfig &getRuntimeConfig()
{
    if (!g_configLoaded)
    {
        // Don't use LOG_NOTICE here to avoid recursive calls during logging initialization
        // First-time startup: Loading default configuration from config.h
        loadDefaultConfig();
    }
    return g_runtimeConfig;
}

bool initializeConfigSystem()
{
    // LittleFS should already be mounted by main.cpp before calling this function
    // Just load the runtime configuration
    return loadRuntimeConfig();
}

bool isConfigFileValid()
{
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        return false;
    }

    DynamicJsonDocument doc(largeJsonDocumentSize);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    return error == DeserializationError::Ok;
}

bool createDefaultConfigFile()
{
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        LOG_ERROR("CONFIG", "Failed to create config.json file");
        return false;
    }

    DynamicJsonDocument doc(largeJsonDocumentSize);

    // Device Configuration
    JsonObject device = doc.createNestedObject("device");
    device["owner"] = defaultDeviceOwner;
    device["timezone"] = defaultTimezone;

    // WiFi Configuration (populated from config.h on first boot)
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = defaultWifiSSID;
    wifi["password"] = defaultWifiPassword;
    wifi["connect_timeout"] = wifiConnectTimeoutMs;

    // MQTT Configuration
    JsonObject mqtt = doc.createNestedObject("mqtt");
    mqtt["server"] = defaultMqttServer;
    mqtt["port"] = defaultMqttPort;
    mqtt["username"] = defaultMqttUsername;
    mqtt["password"] = defaultMqttPassword;

    // API Configuration
    JsonObject apis = doc.createNestedObject("apis");
    apis["jokeAPI"] = jokeAPI;
    apis["quoteAPI"] = quoteAPI;
    apis["triviaAPI"] = triviaAPI;
    apis["betterStackToken"] = betterStackToken;
    apis["betterStackEndpoint"] = betterStackEndpoint;
    apis["chatgptApiToken"] = defaultChatgptApiToken;
    apis["chatgptApiEndpoint"] = chatgptApiEndpoint;

    // Validation Configuration (only maxCharacters)
    JsonObject validation = doc.createNestedObject("validation");
    validation["maxCharacters"] = maxCharacters;

    // Unbidden Ink Configuration
    JsonObject unbiddenInk = doc.createNestedObject("unbiddenInk");
    unbiddenInk["enabled"] = enableUnbiddenInk;
    unbiddenInk["startHour"] = unbiddenInkStartHour;
    unbiddenInk["endHour"] = unbiddenInkEndHour;
    unbiddenInk["frequencyMinutes"] = unbiddenInkFrequencyMinutes;
    unbiddenInk["prompt"] = "Generate a short, encouraging motivational message to help me stay focused and positive. Keep it brief, uplifting, and practical.";

    // Button Configuration (exactly 4 buttons)
    JsonObject buttons = doc.createNestedObject("buttons");
    JsonObject button1 = buttons.createNestedObject("button1");
    button1["shortAction"] = "/joke";
    button1["longAction"] = "/print-test";
    JsonObject button2 = buttons.createNestedObject("button2");
    button2["shortAction"] = "/riddle";
    button2["longAction"] = "";
    JsonObject button3 = buttons.createNestedObject("button3");
    button3["shortAction"] = "/quote";
    button3["longAction"] = "";
    JsonObject button4 = buttons.createNestedObject("button4");
    button4["shortAction"] = "/quiz";
    button4["longAction"] = "";

    serializeJson(doc, configFile);
    configFile.close();

    return true;
}