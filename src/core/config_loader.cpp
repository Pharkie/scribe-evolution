/**
 * @file config_loader.cpp
 * @brief Implementation of dynamic configuration loader
 * @author Adam Knowles        {
            String buttonKey = "button" + String(i + 1);
            JsonObject button = buttons[buttonKey];
            g_runtimeConfig.butto    // Button Configuration (exactly 4 buttons)
    JsonObject buttons = doc.createNestedObject("buttons");
    for (int i = 0; i < 4; i++)
    {
        String buttonKey = "button" + String(i + 1);
        JsonObject button = buttons.createNestedObject(buttonKey);
        button["shortAction"] = defaultButtons[i].shortAction;
        button["longAction"] = defaultButtons[i].longAction;
    }

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
bool g_configLoaded = false;

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

    // Load Unbidden Ink settings from config.json (with defaults from config.h)
    JsonObject unbiddenInk = doc["unbiddenInk"];
    g_runtimeConfig.unbiddenInkEnabled = unbiddenInk["enabled"] | defaultEnableUnbiddenInk;
    g_runtimeConfig.unbiddenInkStartHour = unbiddenInk["startHour"] | defaultUnbiddenInkStartHour;
    g_runtimeConfig.unbiddenInkEndHour = unbiddenInk["endHour"] | defaultUnbiddenInkEndHour;
    g_runtimeConfig.unbiddenInkFrequencyMinutes = unbiddenInk["frequencyMinutes"] | defaultUnbiddenInkFrequencyMinutes;
    g_runtimeConfig.unbiddenInkPrompt = unbiddenInk["prompt"] | "Generate a short, encouraging motivational message to help me stay focused and positive. Keep it brief, uplifting, and practical.";

    // Load Button configuration (exactly 4 buttons)
    JsonObject buttons = doc["buttons"];
    if (buttons.isNull())
    {
        // Use defaults if buttons section is missing
        for (int i = 0; i < 4; i++)
        {
            g_runtimeConfig.buttonShortActions[i] = defaultButtons[i].shortAction;
            g_runtimeConfig.buttonShortMqttTopics[i] = defaultButtons[i].shortMqttTopic;
            g_runtimeConfig.buttonLongActions[i] = defaultButtons[i].longAction;
            g_runtimeConfig.buttonLongMqttTopics[i] = defaultButtons[i].longMqttTopic;
        }
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            String buttonKey = "button" + String(i + 1);
            JsonObject button = buttons[buttonKey];
            g_runtimeConfig.buttonShortActions[i] = button["shortAction"] | (i == 0 ? "/api/joke" : i == 1 ? "/api/riddle"
                                                                                                : i == 2   ? "/api/quote"
                                                                                                           : "/api/quiz");
            g_runtimeConfig.buttonLongActions[i] = button["longAction"] | (i == 0 ? "/api/print-test" : "");
            g_runtimeConfig.buttonShortMqttTopics[i] = button["shortMqttTopic"] | "";
            g_runtimeConfig.buttonLongMqttTopics[i] = button["longMqttTopic"] | "";
        }
    }

#ifdef ENABLE_LEDS
    // Load LED configuration 
    JsonObject leds = doc["leds"];
    if (leds.isNull())
    {
        // Use defaults if leds section is missing
        g_runtimeConfig.ledPin = DEFAULT_LED_PIN;
        g_runtimeConfig.ledCount = DEFAULT_LED_COUNT;
        g_runtimeConfig.ledBrightness = DEFAULT_LED_BRIGHTNESS;
        g_runtimeConfig.ledRefreshRate = DEFAULT_LED_REFRESH_RATE;
        g_runtimeConfig.ledEffectFadeSpeed = DEFAULT_LED_EFFECT_FADE_SPEED;
        g_runtimeConfig.ledTwinkleDensity = DEFAULT_LED_TWINKLE_DENSITY;
        g_runtimeConfig.ledChaseSpeed = DEFAULT_LED_CHASE_SPEED;
        g_runtimeConfig.ledMatrixDrops = DEFAULT_LED_MATRIX_DROPS;
    }
    else
    {
        g_runtimeConfig.ledPin = leds["pin"] | DEFAULT_LED_PIN;
        g_runtimeConfig.ledCount = leds["count"] | DEFAULT_LED_COUNT;
        g_runtimeConfig.ledBrightness = leds["brightness"] | DEFAULT_LED_BRIGHTNESS;
        g_runtimeConfig.ledRefreshRate = leds["refreshRate"] | DEFAULT_LED_REFRESH_RATE;
        g_runtimeConfig.ledEffectFadeSpeed = leds["effectFadeSpeed"] | DEFAULT_LED_EFFECT_FADE_SPEED;
        g_runtimeConfig.ledTwinkleDensity = leds["twinkleDensity"] | DEFAULT_LED_TWINKLE_DENSITY;
        g_runtimeConfig.ledChaseSpeed = leds["chaseSpeed"] | DEFAULT_LED_CHASE_SPEED;
        g_runtimeConfig.ledMatrixDrops = leds["matrixDrops"] | DEFAULT_LED_MATRIX_DROPS;
    }
#endif

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

    g_runtimeConfig.unbiddenInkEnabled = defaultEnableUnbiddenInk;
    g_runtimeConfig.unbiddenInkStartHour = defaultUnbiddenInkStartHour;
    g_runtimeConfig.unbiddenInkEndHour = defaultUnbiddenInkEndHour;
    g_runtimeConfig.unbiddenInkFrequencyMinutes = defaultUnbiddenInkFrequencyMinutes;
    g_runtimeConfig.unbiddenInkPrompt = "Generate a short, encouraging motivational message to help me stay focused and positive. Keep it brief, uplifting, and practical.";

    // Load default button configuration
    for (int i = 0; i < 4; i++)
    {
        g_runtimeConfig.buttonShortActions[i] = defaultButtons[i].shortAction;
        g_runtimeConfig.buttonShortMqttTopics[i] = defaultButtons[i].shortMqttTopic;
        g_runtimeConfig.buttonLongActions[i] = defaultButtons[i].longAction;
        g_runtimeConfig.buttonLongMqttTopics[i] = defaultButtons[i].longMqttTopic;
    }

#ifdef ENABLE_LEDS
    // Load default LED configuration
    g_runtimeConfig.ledPin = DEFAULT_LED_PIN;
    g_runtimeConfig.ledCount = DEFAULT_LED_COUNT;
    g_runtimeConfig.ledBrightness = DEFAULT_LED_BRIGHTNESS;
    g_runtimeConfig.ledRefreshRate = DEFAULT_LED_REFRESH_RATE;
    g_runtimeConfig.ledEffectFadeSpeed = DEFAULT_LED_EFFECT_FADE_SPEED;
    g_runtimeConfig.ledTwinkleDensity = DEFAULT_LED_TWINKLE_DENSITY;
    g_runtimeConfig.ledChaseSpeed = DEFAULT_LED_CHASE_SPEED;
    g_runtimeConfig.ledMatrixDrops = DEFAULT_LED_MATRIX_DROPS;
#endif

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
    unbiddenInk["enabled"] = defaultEnableUnbiddenInk;
    unbiddenInk["startHour"] = defaultUnbiddenInkStartHour;
    unbiddenInk["endHour"] = defaultUnbiddenInkEndHour;
    unbiddenInk["frequencyMinutes"] = defaultUnbiddenInkFrequencyMinutes;
    unbiddenInk["prompt"] = "Generate a short, encouraging motivational message to help me stay focused and positive. Keep it brief, uplifting, and practical.";

    // Button Configuration (exactly 4 buttons)
    JsonObject buttons = doc.createNestedObject("buttons");
    for (int i = 0; i < 4; i++)
    {
        String buttonKey = "button" + String(i + 1);
        JsonObject button = buttons.createNestedObject(buttonKey);
        button["shortAction"] = defaultButtons[i].shortAction;
        button["shortMqttTopic"] = defaultButtons[i].shortMqttTopic;
        button["longAction"] = defaultButtons[i].longAction;
        button["longMqttTopic"] = defaultButtons[i].longMqttTopic;
    }

#ifdef ENABLE_LEDS
    // LED Configuration
    JsonObject leds = doc.createNestedObject("leds");
    leds["pin"] = DEFAULT_LED_PIN;
    leds["count"] = DEFAULT_LED_COUNT;
    leds["brightness"] = DEFAULT_LED_BRIGHTNESS;
    leds["refreshRate"] = DEFAULT_LED_REFRESH_RATE;
    leds["effectFadeSpeed"] = DEFAULT_LED_EFFECT_FADE_SPEED;
    leds["twinkleDensity"] = DEFAULT_LED_TWINKLE_DENSITY;
    leds["chaseSpeed"] = DEFAULT_LED_CHASE_SPEED;
    leds["matrixDrops"] = DEFAULT_LED_MATRIX_DROPS;
#endif

    serializeJson(doc, configFile);
    configFile.close();

    return true;
}

#ifdef ENABLE_LEDS

bool updateLedConfiguration(int pin, int count, int brightness, int refreshRate,
                           int fadeSpeed, int twinkleDensity, int chaseSpeed, int matrixDrops)
{
    // Update runtime configuration
    g_runtimeConfig.ledPin = pin;
    g_runtimeConfig.ledCount = count;
    g_runtimeConfig.ledBrightness = brightness;
    g_runtimeConfig.ledRefreshRate = refreshRate;
    g_runtimeConfig.ledEffectFadeSpeed = fadeSpeed;
    g_runtimeConfig.ledTwinkleDensity = twinkleDensity;
    g_runtimeConfig.ledChaseSpeed = chaseSpeed;
    g_runtimeConfig.ledMatrixDrops = matrixDrops;

    // Save to config.json
    return saveLedConfiguration();
}

bool saveLedConfiguration()
{
    // Load existing config
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        return false;
    }

    DynamicJsonDocument doc(largeJsonDocumentSize);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        return false;
    }

    // Update LED section
    JsonObject leds = doc["leds"];
    if (leds.isNull())
    {
        leds = doc.createNestedObject("leds");
    }

    leds["pin"] = g_runtimeConfig.ledPin;
    leds["count"] = g_runtimeConfig.ledCount;
    leds["brightness"] = g_runtimeConfig.ledBrightness;
    leds["refreshRate"] = g_runtimeConfig.ledRefreshRate;
    leds["effectFadeSpeed"] = g_runtimeConfig.ledEffectFadeSpeed;
    leds["twinkleDensity"] = g_runtimeConfig.ledTwinkleDensity;
    leds["chaseSpeed"] = g_runtimeConfig.ledChaseSpeed;
    leds["matrixDrops"] = g_runtimeConfig.ledMatrixDrops;

    // Save back to file
    configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        return false;
    }

    serializeJson(doc, configFile);
    configFile.close();

    return true;
}

void getLedConfiguration(int& pin, int& count, int& brightness, int& refreshRate,
                        int& fadeSpeed, int& twinkleDensity, int& chaseSpeed, int& matrixDrops)
{
    pin = g_runtimeConfig.ledPin;
    count = g_runtimeConfig.ledCount;
    brightness = g_runtimeConfig.ledBrightness;
    refreshRate = g_runtimeConfig.ledRefreshRate;
    fadeSpeed = g_runtimeConfig.ledEffectFadeSpeed;
    twinkleDensity = g_runtimeConfig.ledTwinkleDensity;
    chaseSpeed = g_runtimeConfig.ledChaseSpeed;
    matrixDrops = g_runtimeConfig.ledMatrixDrops;
}

#endif