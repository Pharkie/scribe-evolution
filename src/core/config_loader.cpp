/**
 * @file config_loader.cpp
 * @brief Implementation of NVS-based configuration loader
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "config_loader.h"
#include "config.h"
#include "logging.h"
#include <Preferences.h>

#if ENABLE_LEDS
#include "led_config_loader.h"
#endif

// NVS configuration constants
static const char* NVS_NAMESPACE = "scribe-app";
static const char* SCHEMA_VERSION_KEY = "prefs_version";
static const int CURRENT_SCHEMA_VERSION = 1;

// Global runtime configuration instance
static RuntimeConfig g_runtimeConfig;
bool g_configLoaded = false;

// Helper function to validate and get string from NVS with fallback
String getNVSString(Preferences &prefs, const char* key, const String& defaultValue) {
    String value = prefs.getString(key, defaultValue);
    // Add validation logic here if needed
    return value;
}

// Helper function to validate and get int from NVS with fallback
int getNVSInt(Preferences &prefs, const char* key, int defaultValue, int minVal = INT_MIN, int maxVal = INT_MAX) {
    int value = prefs.getInt(key, defaultValue);
    if (value < minVal || value > maxVal) {
        LOG_WARNING("CONFIG", "Invalid NVS value for %s: %d, using default: %d", key, value, defaultValue);
        return defaultValue;
    }
    return value;
}

// Helper function to validate and get bool from NVS with fallback
bool getNVSBool(Preferences &prefs, const char* key, bool defaultValue) {
    return prefs.getBool(key, defaultValue);
}

bool loadRuntimeConfig()
{
    LOG_VERBOSE("CONFIG", "Loading runtime configuration from NVS");
    
    // Check schema version and migrate if needed
    if (!checkAndMigrateNVSSchema()) {
        LOG_ERROR("CONFIG", "Schema migration failed, using defaults");
        loadDefaultConfig();
        return false;
    }
    
    // Load configuration from NVS
    if (!loadNVSConfig()) {
        LOG_WARNING("CONFIG", "Failed to load from NVS, using defaults");
        loadDefaultConfig();
        return false;
    }
    
    g_configLoaded = true;
    LOG_NOTICE("CONFIG", "Runtime configuration loaded from NVS");
    return true;
}

bool loadNVSConfig()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) { // read-only
        LOG_ERROR("CONFIG", "Failed to open NVS namespace: %s", NVS_NAMESPACE);
        return false;
    }

    // Load device configuration
    g_runtimeConfig.deviceOwner = getNVSString(prefs, "device_owner", defaultDeviceOwner);
    g_runtimeConfig.timezone = getNVSString(prefs, "device_timezone", defaultTimezone);

    // Load WiFi configuration
    g_runtimeConfig.wifiSSID = getNVSString(prefs, "wifi_ssid", defaultWifiSSID);
    g_runtimeConfig.wifiPassword = getNVSString(prefs, "wifi_password", defaultWifiPassword);
    g_runtimeConfig.wifiConnectTimeoutMs = getNVSInt(prefs, "wifi_timeout", wifiConnectTimeoutMs, 5000, 60000);

    // Load MQTT configuration
    g_runtimeConfig.mqttServer = getNVSString(prefs, "mqtt_server", defaultMqttServer);
    g_runtimeConfig.mqttPort = getNVSInt(prefs, "mqtt_port", defaultMqttPort, 1, 65535);
    g_runtimeConfig.mqttUsername = getNVSString(prefs, "mqtt_username", defaultMqttUsername);
    g_runtimeConfig.mqttPassword = getNVSString(prefs, "mqtt_password", defaultMqttPassword);

    // Load API configuration (non-user configurable APIs remain as constants)
    g_runtimeConfig.jokeAPI = jokeAPI;
    g_runtimeConfig.quoteAPI = quoteAPI;
    g_runtimeConfig.triviaAPI = triviaAPI;
    g_runtimeConfig.betterStackToken = betterStackToken;
    g_runtimeConfig.betterStackEndpoint = betterStackEndpoint;
    g_runtimeConfig.chatgptApiToken = getNVSString(prefs, "chatgpt_token", defaultChatgptApiToken);
    g_runtimeConfig.chatgptApiEndpoint = chatgptApiEndpoint;

    // Load validation configuration
    g_runtimeConfig.maxCharacters = getNVSInt(prefs, "max_characters", maxCharacters, 100, 5000);

    // Load Unbidden Ink configuration
    g_runtimeConfig.unbiddenInkEnabled = getNVSBool(prefs, "unbidden_enabled", defaultEnableUnbiddenInk);
    g_runtimeConfig.unbiddenInkStartHour = getNVSInt(prefs, "unbidden_start_hour", defaultUnbiddenInkStartHour, 0, 24);
    g_runtimeConfig.unbiddenInkEndHour = getNVSInt(prefs, "unbidden_end_hour", defaultUnbiddenInkEndHour, 0, 24);
    g_runtimeConfig.unbiddenInkFrequencyMinutes = getNVSInt(prefs, "unbidden_frequency", defaultUnbiddenInkFrequencyMinutes, minUnbiddenInkFrequencyMinutes, maxUnbiddenInkFrequencyMinutes);
    g_runtimeConfig.unbiddenInkPrompt = getNVSString(prefs, "unbidden_prompt", "Generate a short, inspiring quote about creativity, technology, or daily life. Keep it under 200 characters.");

    // Load button configuration (4 buttons, 4 fields each = 16 keys)
    for (int i = 0; i < 4; i++) {
        String buttonPrefix = "button" + String(i + 1) + "_";
        g_runtimeConfig.buttonShortActions[i] = getNVSString(prefs, (buttonPrefix + "short_action").c_str(), defaultButtons[i].shortAction);
        g_runtimeConfig.buttonShortMqttTopics[i] = getNVSString(prefs, (buttonPrefix + "short_mqtt").c_str(), defaultButtons[i].shortMqttTopic);
        g_runtimeConfig.buttonLongActions[i] = getNVSString(prefs, (buttonPrefix + "long_action").c_str(), defaultButtons[i].longAction);
        g_runtimeConfig.buttonLongMqttTopics[i] = getNVSString(prefs, (buttonPrefix + "long_mqtt").c_str(), defaultButtons[i].longMqttTopic);
    }

#if ENABLE_LEDS
    // Load LED configuration
    g_runtimeConfig.ledPin = getNVSInt(prefs, "led_pin", DEFAULT_LED_PIN, 0, 39);
    g_runtimeConfig.ledCount = getNVSInt(prefs, "led_count", DEFAULT_LED_COUNT, 1, 1000);
    g_runtimeConfig.ledBrightness = getNVSInt(prefs, "led_brightness", DEFAULT_LED_BRIGHTNESS, 1, 255);
    g_runtimeConfig.ledRefreshRate = getNVSInt(prefs, "led_refresh_rate", DEFAULT_LED_REFRESH_RATE, 10, 120);
    
    // Load LED effects configuration (this will need custom handling if complex)
    g_runtimeConfig.ledEffects = getDefaultLedEffectsConfig(); // TODO: Implement NVS LED effects loading
#endif

    prefs.end();
    return true;
}

void loadDefaultConfig()
{
    // Load device defaults
    g_runtimeConfig.deviceOwner = defaultDeviceOwner;
    g_runtimeConfig.timezone = defaultTimezone;

    // Load WiFi defaults (empty by default, must be configured)
    g_runtimeConfig.wifiSSID = defaultWifiSSID;
    g_runtimeConfig.wifiPassword = defaultWifiPassword;
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
    g_runtimeConfig.unbiddenInkPrompt = "Generate a short, inspiring quote about creativity, technology, or daily life. Keep it under 200 characters.";

    // Load default button configuration
    for (int i = 0; i < 4; i++)
    {
        g_runtimeConfig.buttonShortActions[i] = defaultButtons[i].shortAction;
        g_runtimeConfig.buttonShortMqttTopics[i] = defaultButtons[i].shortMqttTopic;
        g_runtimeConfig.buttonLongActions[i] = defaultButtons[i].longAction;
        g_runtimeConfig.buttonLongMqttTopics[i] = defaultButtons[i].longMqttTopic;
    }

#if ENABLE_LEDS
    // Load default LED configuration
    g_runtimeConfig.ledPin = DEFAULT_LED_PIN;
    g_runtimeConfig.ledCount = DEFAULT_LED_COUNT;
    g_runtimeConfig.ledBrightness = DEFAULT_LED_BRIGHTNESS;
    g_runtimeConfig.ledRefreshRate = DEFAULT_LED_REFRESH_RATE;
    g_runtimeConfig.ledEffects = getDefaultLedEffectsConfig();
#endif

    g_configLoaded = true;
    LOG_NOTICE("CONFIG", "Using default configuration from config.h");
}

bool saveNVSConfig(const RuntimeConfig &config)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) { // read-write
        LOG_ERROR("CONFIG", "Failed to open NVS namespace for writing: %s", NVS_NAMESPACE);
        return false;
    }

    // Save device configuration
    prefs.putString("device_owner", config.deviceOwner);
    prefs.putString("device_timezone", config.timezone);

    // Save WiFi configuration
    prefs.putString("wifi_ssid", config.wifiSSID);
    prefs.putString("wifi_password", config.wifiPassword);
    prefs.putULong("wifi_timeout", config.wifiConnectTimeoutMs);

    // Save MQTT configuration
    prefs.putString("mqtt_server", config.mqttServer);
    prefs.putInt("mqtt_port", config.mqttPort);
    prefs.putString("mqtt_username", config.mqttUsername);
    prefs.putString("mqtt_password", config.mqttPassword);

    // Save ChatGPT API token (other APIs are constants)
    prefs.putString("chatgpt_token", config.chatgptApiToken);

    // Save validation configuration
    prefs.putInt("max_characters", config.maxCharacters);

    // Save Unbidden Ink configuration
    prefs.putBool("unbidden_enabled", config.unbiddenInkEnabled);
    prefs.putInt("unbidden_start_hour", config.unbiddenInkStartHour);
    prefs.putInt("unbidden_end_hour", config.unbiddenInkEndHour);
    prefs.putInt("unbidden_frequency", config.unbiddenInkFrequencyMinutes);
    prefs.putString("unbidden_prompt", config.unbiddenInkPrompt);

    // Save button configuration
    for (int i = 0; i < 4; i++) {
        String buttonPrefix = "button" + String(i + 1) + "_";
        prefs.putString((buttonPrefix + "short_action").c_str(), config.buttonShortActions[i]);
        prefs.putString((buttonPrefix + "short_mqtt").c_str(), config.buttonShortMqttTopics[i]);
        prefs.putString((buttonPrefix + "long_action").c_str(), config.buttonLongActions[i]);
        prefs.putString((buttonPrefix + "long_mqtt").c_str(), config.buttonLongMqttTopics[i]);
    }

#if ENABLE_LEDS
    // Save LED configuration
    prefs.putInt("led_pin", config.ledPin);
    prefs.putInt("led_count", config.ledCount);
    prefs.putInt("led_brightness", config.ledBrightness);
    prefs.putInt("led_refresh_rate", config.ledRefreshRate);
    
    // TODO: Save LED effects configuration if needed
#endif

    prefs.end();
    LOG_NOTICE("CONFIG", "Configuration saved to NVS");
    return true;
}

bool checkAndMigrateNVSSchema()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) { // read-only first
        LOG_ERROR("CONFIG", "Failed to open NVS namespace: %s", NVS_NAMESPACE);
        return false;
    }

    int currentVersion = prefs.getInt(SCHEMA_VERSION_KEY, 0);
    prefs.end();

    if (currentVersion == 0) {
        LOG_NOTICE("CONFIG", "First boot detected, initializing NVS with defaults");
        return initializeNVSConfig();
    }
    
    if (currentVersion != CURRENT_SCHEMA_VERSION) {
        LOG_WARNING("CONFIG", "NVS schema version mismatch: found %d, expected %d", currentVersion, CURRENT_SCHEMA_VERSION);
        LOG_NOTICE("CONFIG", "Resetting NVS to defaults for schema migration");
        return initializeNVSConfig();
    }

    LOG_VERBOSE("CONFIG", "NVS schema version %d is current", currentVersion);
    return true;
}

bool initializeNVSConfig()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) { // read-write
        LOG_ERROR("CONFIG", "Failed to open NVS namespace for initialization: %s", NVS_NAMESPACE);
        return false;
    }

    // Clear all existing preferences
    prefs.clear();
    
    // Set schema version
    prefs.putInt(SCHEMA_VERSION_KEY, CURRENT_SCHEMA_VERSION);

    // Initialize with defaults from config.h
    loadDefaultConfig();
    
    // Save defaults to NVS
    prefs.end(); // Close first, then reopen for saving
    bool result = saveNVSConfig(g_runtimeConfig);
    
    if (result) {
        LOG_NOTICE("CONFIG", "NVS initialized with defaults from config.h (schema version %d)", CURRENT_SCHEMA_VERSION);
    } else {
        LOG_ERROR("CONFIG", "Failed to initialize NVS with defaults");
    }
    
    return result;
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
    // Initialize NVS-based configuration system
    return loadRuntimeConfig();
}

void setRuntimeConfig(const RuntimeConfig &config)
{
    g_runtimeConfig = config;
    g_configLoaded = true;
}