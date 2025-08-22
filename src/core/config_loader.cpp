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
#include "nvs_keys.h"
#include "logging.h"
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_err.h>

#if ENABLE_LEDS
#include "led_config_loader.h"
#endif

// NVS configuration constants
static const char *NVS_NAMESPACE = "scribe-app";

// Global runtime configuration instance
static RuntimeConfig g_runtimeConfig;
bool g_configLoaded = false;

// Helper function to validate and get string from NVS with fallback - saves default if missing
String getNVSString(Preferences &prefs, const char *key, const String &defaultValue, int maxLength = 1000)
{
    if (!prefs.isKey(key))
    {
        LOG_NOTICE("CONFIG", "NVS key '%s' missing - saving default value", key);
        prefs.putString(key, defaultValue);
        return defaultValue;
    }
    String result = prefs.getString(key, defaultValue);
    LOG_VERBOSE("CONFIG", "DEBUG: getNVSString('%s') key exists, value length=%d, default length=%d",
                key, result.length(), defaultValue.length());

    // Validate string length and basic sanity
    if (result.length() > maxLength)
    {
        LOG_WARNING("CONFIG", "NVS key '%s' value too long (%d > %d) - using and saving default", key, result.length(), maxLength);
        prefs.putString(key, defaultValue);
        return defaultValue;
    }

    // If the stored value is empty but we have a non-empty default, use the default
    if (result.length() == 0 && defaultValue.length() > 0)
    {
        LOG_NOTICE("CONFIG", "NVS key '%s' has empty value - using and saving default", key);
        prefs.putString(key, defaultValue);
        return defaultValue;
    }
    return result;
} // Helper function to validate and get int from NVS with fallback - saves default if missing
int getNVSInt(Preferences &prefs, const char *key, int defaultValue, int minVal = INT_MIN, int maxVal = INT_MAX)
{
    if (!prefs.isKey(key))
    {
        LOG_NOTICE("CONFIG", "NVS key '%s' missing - saving default value: %d", key, defaultValue);
        prefs.putInt(key, defaultValue);
        return defaultValue;
    }

    int value = prefs.getInt(key, defaultValue);
    if (value < minVal || value > maxVal)
    {
        LOG_WARNING("CONFIG", "Invalid NVS value for %s: %d, using and saving default: %d", key, value, defaultValue);
        prefs.putInt(key, defaultValue);
        return defaultValue;
    }
    return value;
}

// Helper function to validate and get bool from NVS with fallback - saves default if missing
bool getNVSBool(Preferences &prefs, const char *key, bool defaultValue)
{
    if (!prefs.isKey(key))
    {
        LOG_NOTICE("CONFIG", "NVS key '%s' missing - saving default value: %s", key, defaultValue ? "true" : "false");
        prefs.putBool(key, defaultValue);
        return defaultValue;
    }
    return prefs.getBool(key, defaultValue);
}

// Helper function to validate and get port from NVS with fallback - ensures valid port range
int getNVSPort(Preferences &prefs, const char *key, int defaultValue)
{
    return getNVSInt(prefs, key, defaultValue, 1, 65535);
}

bool loadRuntimeConfig()
{
    LOG_NOTICE("CONFIG", "Loading runtime configuration from NVS");

    // Load configuration from NVS (with auto-initialization of missing keys)
    if (!loadNVSConfig())
    {
        LOG_WARNING("CONFIG", "Failed to load from NVS, using defaults");
        loadDefaultConfig();
        g_configLoaded = true;
        LOG_NOTICE("CONFIG", "Runtime configuration loaded from defaults");
        return true; // Still successful - we have valid configuration
    }

    g_configLoaded = true;
    LOG_NOTICE("CONFIG", "Runtime configuration loaded from NVS");
    return true;
}

bool loadNVSConfig()
{
    Preferences prefs;

    // Open in write mode since our helper functions need to save defaults for missing keys
    if (!prefs.begin(NVS_NAMESPACE, false))
    {
        LOG_ERROR("CONFIG", "Failed to open NVS namespace: %s", NVS_NAMESPACE);
        return false;
    }

    // Load device configuration
    g_runtimeConfig.deviceOwner = getNVSString(prefs, NVS_DEVICE_OWNER, defaultDeviceOwner, 50);
    g_runtimeConfig.timezone = getNVSString(prefs, NVS_DEVICE_TIMEZONE, defaultTimezone, 50);

    // Load WiFi configuration
    g_runtimeConfig.wifiSSID = getNVSString(prefs, NVS_WIFI_SSID, defaultWifiSSID, 32);
    g_runtimeConfig.wifiPassword = getNVSString(prefs, NVS_WIFI_PASSWORD, defaultWifiPassword, 63);
    g_runtimeConfig.wifiConnectTimeoutMs = getNVSInt(prefs, NVS_WIFI_TIMEOUT, wifiConnectTimeoutMs, 5000, 60000);

    // Load MQTT configuration
    g_runtimeConfig.mqttServer = getNVSString(prefs, NVS_MQTT_SERVER, defaultMqttServer, 255);
    g_runtimeConfig.mqttPort = getNVSInt(prefs, NVS_MQTT_PORT, defaultMqttPort, 1, 65535);
    g_runtimeConfig.mqttUsername = getNVSString(prefs, NVS_MQTT_USERNAME, defaultMqttUsername, 100);
    g_runtimeConfig.mqttPassword = getNVSString(prefs, NVS_MQTT_PASSWORD, defaultMqttPassword, 100);

    // Load API configuration (non-user configurable APIs remain as constants)
    g_runtimeConfig.jokeAPI = jokeAPI;
    g_runtimeConfig.quoteAPI = quoteAPI;
    g_runtimeConfig.triviaAPI = triviaAPI;
    g_runtimeConfig.betterStackToken = betterStackToken;
    g_runtimeConfig.betterStackEndpoint = betterStackEndpoint;
    g_runtimeConfig.chatgptApiToken = getNVSString(prefs, NVS_CHATGPT_TOKEN, defaultChatgptApiToken, 300);
    g_runtimeConfig.chatgptApiEndpoint = chatgptApiEndpoint;

    // Load validation configuration (hardcoded from config.h)
    g_runtimeConfig.maxCharacters = maxCharacters;

    // Load Unbidden Ink settings
    LOG_VERBOSE("CONFIG", "DEBUG: Default values - startHour=%d, endHour=%d, frequency=%d",
                defaultUnbiddenInkStartHour, defaultUnbiddenInkEndHour, defaultUnbiddenInkFrequencyMinutes);

    g_runtimeConfig.unbiddenInkEnabled = getNVSBool(prefs, NVS_UNBIDDEN_ENABLED, defaultEnableUnbiddenInk);
    g_runtimeConfig.unbiddenInkStartHour = getNVSInt(prefs, NVS_UNBIDDEN_START_HOUR, defaultUnbiddenInkStartHour, 0, 24);
    g_runtimeConfig.unbiddenInkEndHour = getNVSInt(prefs, NVS_UNBIDDEN_END_HOUR, defaultUnbiddenInkEndHour, 0, 24);
    g_runtimeConfig.unbiddenInkFrequencyMinutes = getNVSInt(prefs, NVS_UNBIDDEN_FREQUENCY, defaultUnbiddenInkFrequencyMinutes, minUnbiddenInkFrequencyMinutes, maxUnbiddenInkFrequencyMinutes);

    LOG_VERBOSE("CONFIG", "DEBUG: Loaded values - startHour=%d, endHour=%d, frequency=%d",
                g_runtimeConfig.unbiddenInkStartHour, g_runtimeConfig.unbiddenInkEndHour, g_runtimeConfig.unbiddenInkFrequencyMinutes);
    g_runtimeConfig.unbiddenInkPrompt = getNVSString(prefs, NVS_UNBIDDEN_PROMPT, "Generate a short, inspiring quote about creativity, technology, or daily life. Keep it under 200 characters.", 500);
    LOG_VERBOSE("CONFIG", "DEBUG: Loaded prompt length=%d, first 50 chars='%s'",
                g_runtimeConfig.unbiddenInkPrompt.length(), g_runtimeConfig.unbiddenInkPrompt.substring(0, 50).c_str());

    // Load button configuration (4 buttons, 6 fields each = 24 keys)
    for (int i = 0; i < 4; i++)
    {
        String buttonPrefix = "btn" + String(i + 1) + "_";
        g_runtimeConfig.buttonShortActions[i] = getNVSString(prefs, (buttonPrefix + "short_act").c_str(), defaultButtons[i].shortAction, 50);
        g_runtimeConfig.buttonShortMqttTopics[i] = getNVSString(prefs, (buttonPrefix + "short_mq").c_str(), defaultButtons[i].shortMqttTopic, 128);
        g_runtimeConfig.buttonLongActions[i] = getNVSString(prefs, (buttonPrefix + "long_act").c_str(), defaultButtons[i].longAction, 50);
        g_runtimeConfig.buttonLongMqttTopics[i] = getNVSString(prefs, (buttonPrefix + "long_mq").c_str(), defaultButtons[i].longMqttTopic, 128);
        
        // Load LED effect configuration with defaults
        g_runtimeConfig.buttonShortLedEffects[i] = getNVSString(prefs, (buttonPrefix + "short_led").c_str(), "simple_chase", 20);
        g_runtimeConfig.buttonLongLedEffects[i] = getNVSString(prefs, (buttonPrefix + "long_led").c_str(), "simple_chase", 20);
    }

#if ENABLE_LEDS
    // Load LED configuration
    g_runtimeConfig.ledPin = getNVSInt(prefs, NVS_LED_PIN, DEFAULT_LED_PIN, 0, 39);
    g_runtimeConfig.ledCount = getNVSInt(prefs, NVS_LED_COUNT, DEFAULT_LED_COUNT, 1, 1000);
    g_runtimeConfig.ledBrightness = getNVSInt(prefs, NVS_LED_BRIGHTNESS, DEFAULT_LED_BRIGHTNESS, 1, 255);
    g_runtimeConfig.ledRefreshRate = getNVSInt(prefs, NVS_LED_REFRESH_RATE, DEFAULT_LED_REFRESH_RATE, 10, 120);

    // Load LED effects configuration (this will need custom handling if complex)
    g_runtimeConfig.ledEffects = getDefaultLedEffectsConfig();
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

    LOG_VERBOSE("CONFIG", "DEBUG: Setting defaults - startHour=%d, endHour=%d, frequency=%d",
                defaultUnbiddenInkStartHour, defaultUnbiddenInkEndHour, defaultUnbiddenInkFrequencyMinutes);

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
    if (!prefs.begin(NVS_NAMESPACE, false))
    { // read-write
        LOG_ERROR("CONFIG", "Failed to open NVS namespace for writing: %s", NVS_NAMESPACE);
        return false;
    }

    // Save device configuration
    prefs.putString(NVS_DEVICE_OWNER, config.deviceOwner);
    prefs.putString(NVS_DEVICE_TIMEZONE, config.timezone);

    // Save WiFi configuration
    prefs.putString(NVS_WIFI_SSID, config.wifiSSID);
    prefs.putString(NVS_WIFI_PASSWORD, config.wifiPassword);
    prefs.putULong(NVS_WIFI_TIMEOUT, config.wifiConnectTimeoutMs);

    // Save MQTT configuration
    prefs.putString(NVS_MQTT_SERVER, config.mqttServer);
    prefs.putInt(NVS_MQTT_PORT, config.mqttPort);
    prefs.putString(NVS_MQTT_USERNAME, config.mqttUsername);
    prefs.putString(NVS_MQTT_PASSWORD, config.mqttPassword);

    // Save ChatGPT API token (other APIs are constants)
    prefs.putString(NVS_CHATGPT_TOKEN, config.chatgptApiToken);

    // Save Unbidden Ink configuration
    prefs.putBool("unbid_enabled", config.unbiddenInkEnabled);
    prefs.putInt("unbid_start_hr", config.unbiddenInkStartHour);
    prefs.putInt("unbid_end_hr", config.unbiddenInkEndHour);
    prefs.putInt("unbid_freq_min", config.unbiddenInkFrequencyMinutes);
    prefs.putString(NVS_UNBIDDEN_PROMPT, config.unbiddenInkPrompt);

    // Save button configuration
    for (int i = 0; i < 4; i++)
    {
        String buttonPrefix = "btn" + String(i + 1) + "_";
        prefs.putString((buttonPrefix + "short_act").c_str(), config.buttonShortActions[i]);
        prefs.putString((buttonPrefix + "short_mq").c_str(), config.buttonShortMqttTopics[i]);
        prefs.putString((buttonPrefix + "long_act").c_str(), config.buttonLongActions[i]);
        prefs.putString((buttonPrefix + "long_mq").c_str(), config.buttonLongMqttTopics[i]);
        
        // Save LED effect configuration
        prefs.putString((buttonPrefix + "short_led").c_str(), config.buttonShortLedEffects[i]);
        prefs.putString((buttonPrefix + "long_led").c_str(), config.buttonLongLedEffects[i]);
    }

#if ENABLE_LEDS
    // Save LED configuration
    prefs.putInt(NVS_LED_PIN, config.ledPin);
    prefs.putInt(NVS_LED_COUNT, config.ledCount);
    prefs.putInt(NVS_LED_BRIGHTNESS, config.ledBrightness);
    prefs.putInt(NVS_LED_REFRESH_RATE, config.ledRefreshRate);

    // TODO: Save LED effects configuration if needed
#endif

    prefs.end();
    LOG_NOTICE("CONFIG", "Configuration saved to NVS");
    return true;
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

bool initializeNVSConfig()
{
    // Initialize NVS partition
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated or has different version, erase it and reinitialize
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK)
    {
        LOG_ERROR("CONFIG", "Failed to initialize NVS: %s", esp_err_to_name(err));
        return false;
    }

    LOG_NOTICE("CONFIG", "NVS initialized successfully");
    return true;
}

bool checkAndMigrateNVSSchema()
{
    // For now, just return true - migration logic would go here if needed
    // This could check version numbers, migrate old key names, etc.
    LOG_NOTICE("CONFIG", "NVS schema check complete (no migration needed)");
    return true;
}