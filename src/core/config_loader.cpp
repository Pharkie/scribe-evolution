/**
 * @file config_loader.cpp
 * @brief Implementation of NVS-based configuration loader
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "config_loader.h"
#include <config/config.h>
#include "nvs_keys.h"
#include "logging.h"
#include "ManagerLock.h"
#include <Preferences.h>
#include <nvs_flash.h>
#include <esp_err.h>
#include <web/config_field_registry.h>

#if ENABLE_LEDS
#include "led_config_loader.h"
#endif

// NVS configuration constants
static const char *NVS_NAMESPACE = "scribe-app";

// Global runtime configuration instance
static RuntimeConfig g_runtimeConfig;
bool g_configLoaded = false;

// ============================================================================
// CONFIGMANAGER SINGLETON IMPLEMENTATION
// ============================================================================

// Singleton instance (Meyer's singleton - thread-safe in C++11+)
ConfigManager& ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

// Constructor
ConfigManager::ConfigManager() {
    // Mutex created in begin()
}

// Initialize (must be called in setup)
void ConfigManager::begin() {
    if (initialized) {
        LOG_VERBOSE("CONFIG", "ConfigManager already initialized");
        return;
    }

    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        LOG_ERROR("CONFIG", "Failed to create ConfigManager mutex!");
        return;
    }

    initialized = true;
    LOG_NOTICE("CONFIG", "ConfigManager initialized (thread-safe singleton)");
}

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

// Helper function to validate and get float from NVS with fallback - saves default if missing
float getNVSFloat(Preferences &prefs, const char *key, float defaultValue)
{
    if (!prefs.isKey(key))
    {
        LOG_NOTICE("CONFIG", "NVS key '%s' missing - saving default value: %.2f", key, defaultValue);
        prefs.putFloat(key, defaultValue);
        return defaultValue;
    }
    return prefs.getFloat(key, defaultValue);
}

// Helper function to validate and get port from NVS with fallback - ensures valid port range
int getNVSPort(Preferences &prefs, const char *key, int defaultValue)
{
    return getNVSInt(prefs, key, defaultValue, 1, 65535);
}

// Public method: loadRuntimeConfig (thread-safe)
bool ConfigManager::loadRuntimeConfig()
{
    if (!initialized) {
        LOG_ERROR("CONFIG", "ConfigManager not initialized - call begin() first!");
        return false;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "CONFIG");
    if (!lock.isLocked()) {
        LOG_ERROR("CONFIG", "Failed to acquire ConfigManager mutex!");
        return false;
    }

    LOG_NOTICE("CONFIG", "Loading runtime configuration from NVS");

    // Load configuration from NVS (with auto-initialization of missing keys)
    bool success = true;
    if (!loadNVSConfigInternal())
    {
        LOG_WARNING("CONFIG", "Failed to load from NVS, using defaults");
        loadDefaultConfigInternal();
        g_configLoaded = true;
        LOG_NOTICE("CONFIG", "Runtime configuration loaded from defaults");
    }
    else
    {
        g_configLoaded = true;
        LOG_NOTICE("CONFIG", "Runtime configuration loaded from NVS");
    }

    // Mutex automatically released by ManagerLock destructor
    return success;
}

// Internal helper: loadNVSConfigInternal (mutex already held)
bool ConfigManager::loadNVSConfigInternal()
{
    Preferences prefs;

    // Open in write mode since our helper functions need to save defaults for missing keys
    if (!prefs.begin(NVS_NAMESPACE, false))
    {
        LOG_ERROR("CONFIG", "Failed to open NVS namespace: %s", NVS_NAMESPACE);
        return false;
    }

    // Board type detection and mismatch handling
    // const BoardPinDefaults &boardDefaults = getBoardDefaults();
    String currentBoardType = String(BOARD_NAME);
    // const BoardPinDefaults &boardDefaults = getBoardDefaults();

    // Check if board_type exists - we need to know this BEFORE getNVSString changes it
    bool boardTypeWasMissing = !prefs.isKey(NVS_BOARD_TYPE);

    // Load saved board type (getNVSString will save currentBoardType if missing)
    String savedBoardType = getNVSString(prefs, NVS_BOARD_TYPE, currentBoardType, 50);

    // Reset GPIO pins if board type was missing or mismatched (hardware change detected)
    // Note: We no longer check if GPIO differs from default - users can customize pins via web interface
    // and those custom configurations will be preserved across reboots
    if (boardTypeWasMissing || savedBoardType != currentBoardType)
    {
        if (!boardTypeWasMissing)
        {
            // Board mismatch - show warning
            LOG_WARNING("CONFIG", "╔═══════════════════════════════════════════════════════════╗");
            LOG_WARNING("CONFIG", "║  ⚠️  BOARD MISMATCH DETECTED - RESETTING GPIO CONFIGS  ⚠️  ║");
            LOG_WARNING("CONFIG", "╠═══════════════════════════════════════════════════════════╣");
            LOG_WARNING("CONFIG", "║  Saved Board:   %-41s ║", savedBoardType.c_str());
            LOG_WARNING("CONFIG", "║  Current Board: %-41s ║", currentBoardType.c_str());
            LOG_WARNING("CONFIG", "║  Resetting all GPIO pins to new board defaults...        ║");
            LOG_WARNING("CONFIG", "╚═══════════════════════════════════════════════════════════╝");

            // Update board type in NVS (getNVSString already did this if it was missing)
            prefs.putString(NVS_BOARD_TYPE, currentBoardType);
        }

        // Reset GPIO configurations to current board defaults
        prefs.putInt(NVS_PRINTER_TX_PIN, defaultPrinterTxPin);
        prefs.putInt(NVS_PRINTER_RX_PIN, BOARD_PRINTER_RX_PIN);
        prefs.putInt(NVS_PRINTER_DTR_PIN, BOARD_PRINTER_DTR_PIN);
        prefs.putInt(NVS_BUTTON1_GPIO, BOARD_BUTTON_PINS[0]);
        prefs.putInt(NVS_BUTTON2_GPIO, BOARD_BUTTON_PINS[1]);
        prefs.putInt(NVS_BUTTON3_GPIO, BOARD_BUTTON_PINS[2]);
        prefs.putInt(NVS_BUTTON4_GPIO, BOARD_BUTTON_PINS[3]);

        #if ENABLE_LEDS
        prefs.putInt(NVS_LED_PIN, BOARD_LED_STRIP_PIN);
        #endif

        LOG_NOTICE("CONFIG", "GPIO configurations initialized for %s", currentBoardType.c_str());
    }

    // Load device configuration
    g_runtimeConfig.deviceOwner = getNVSString(prefs, NVS_DEVICE_OWNER, defaultDeviceOwner, 50);
    g_runtimeConfig.timezone = getNVSString(prefs, NVS_DEVICE_TIMEZONE, defaultTimezone, 50);

    // Load hardware GPIO configuration (now board-aware)
    g_runtimeConfig.printerTxPin = getNVSInt(prefs, NVS_PRINTER_TX_PIN, defaultPrinterTxPin, 0, BOARD_MAX_GPIO);
    g_runtimeConfig.printerRxPin = getNVSInt(prefs, NVS_PRINTER_RX_PIN, BOARD_PRINTER_RX_PIN, -1, BOARD_MAX_GPIO);
    g_runtimeConfig.printerDtrPin = getNVSInt(prefs, NVS_PRINTER_DTR_PIN, BOARD_PRINTER_DTR_PIN, -1, BOARD_MAX_GPIO);
    g_runtimeConfig.buttonGpios[0] = getNVSInt(prefs, NVS_BUTTON1_GPIO, BOARD_BUTTON_PINS[0], 0, BOARD_MAX_GPIO);
    g_runtimeConfig.buttonGpios[1] = getNVSInt(prefs, NVS_BUTTON2_GPIO, BOARD_BUTTON_PINS[1], 0, BOARD_MAX_GPIO);
    g_runtimeConfig.buttonGpios[2] = getNVSInt(prefs, NVS_BUTTON3_GPIO, BOARD_BUTTON_PINS[2], 0, BOARD_MAX_GPIO);
    g_runtimeConfig.buttonGpios[3] = getNVSInt(prefs, NVS_BUTTON4_GPIO, BOARD_BUTTON_PINS[3], 0, BOARD_MAX_GPIO);

    // Load WiFi configuration
    g_runtimeConfig.wifiSSID = getNVSString(prefs, NVS_WIFI_SSID, defaultWifiSSID, 32);
    g_runtimeConfig.wifiPassword = getNVSString(prefs, NVS_WIFI_PASSWORD, defaultWifiPassword, 63);
    g_runtimeConfig.wifiConnectTimeoutMs = wifiConnectTimeoutMs; // Runtime-only constant from system_constants.h

    // Load MQTT configuration (write default if missing)
    g_runtimeConfig.mqttEnabled = getNVSBool(prefs, NVS_MQTT_ENABLED, defaultMqttEnabled);
    g_runtimeConfig.mqttServer = getNVSString(prefs, NVS_MQTT_SERVER, defaultMqttServer, 255);
    g_runtimeConfig.mqttPort = getNVSInt(prefs, NVS_MQTT_PORT, defaultMqttPort, 1, 65535);
    g_runtimeConfig.mqttUsername = getNVSString(prefs, NVS_MQTT_USERNAME, defaultMqttUsername, 100);
    g_runtimeConfig.mqttPassword = getNVSString(prefs, NVS_MQTT_PASSWORD, defaultMqttPassword, 100);

    // Load API configuration (non-user configurable APIs remain as constants)
    g_runtimeConfig.jokeAPI = jokeAPI;
    g_runtimeConfig.quoteAPI = quoteAPI;
    g_runtimeConfig.triviaAPI = triviaAPI;
    g_runtimeConfig.newsAPI = newsAPI;
    g_runtimeConfig.chatgptApiToken = getNVSString(prefs, NVS_CHATGPT_TOKEN, defaultChatgptApiToken, 300);
    g_runtimeConfig.anthropicApiKey = getNVSString(prefs, NVS_ANTHROPIC_KEY, defaultAnthropicApiKey, 300);
    g_runtimeConfig.googleApiKey = getNVSString(prefs, NVS_GOOGLE_KEY, defaultGoogleApiKey, 300);
    g_runtimeConfig.aiProvider = getNVSString(prefs, NVS_AI_PROVIDER, defaultAiProvider, 50);
    g_runtimeConfig.aiModel = getNVSString(prefs, NVS_AI_MODEL, defaultAiModel, 100);
    g_runtimeConfig.aiTemperature = getNVSFloat(prefs, NVS_AI_TEMPERATURE, defaultAiTemperature);
    g_runtimeConfig.aiMaxTokens = getNVSInt(prefs, NVS_AI_MAX_TOKENS, defaultAiMaxTokens, 50, 500);
    g_runtimeConfig.chatgptApiEndpoint = chatgptApiEndpoint;
    g_runtimeConfig.anthropicApiEndpoint = anthropicApiEndpoint;
    g_runtimeConfig.googleApiEndpoint = googleApiEndpoint;

    // Load validation configuration (hardcoded from config.h)
    g_runtimeConfig.maxCharacters = maxCharacters;

    // Load Unbidden Ink settings
    g_runtimeConfig.unbiddenInkEnabled = getNVSBool(prefs, NVS_UNBIDDEN_ENABLED, defaultEnableUnbiddenInk);
    g_runtimeConfig.unbiddenInkStartHour = getNVSInt(prefs, NVS_UNBIDDEN_START_HOUR, defaultUnbiddenInkStartHour, 0, 24);
    g_runtimeConfig.unbiddenInkEndHour = getNVSInt(prefs, NVS_UNBIDDEN_END_HOUR, defaultUnbiddenInkEndHour, 0, 24);
    g_runtimeConfig.unbiddenInkFrequencyMinutes = getNVSInt(prefs, NVS_UNBIDDEN_FREQUENCY, defaultUnbiddenInkFrequencyMinutes, minUnbiddenInkFrequencyMinutes, maxUnbiddenInkFrequencyMinutes);
    g_runtimeConfig.unbiddenInkPrompt = getNVSString(prefs, NVS_UNBIDDEN_PROMPT, defaultUnbiddenInkPrompt, 500);

    // Load button configuration (4 buttons, 6 fields each = 24 keys)
    for (int i = 0; i < 4; i++)
    {
        String buttonPrefix = "btn" + String(i + 1) + "_";

        // Load button actions from NVS (use defaults if missing)
        g_runtimeConfig.buttonShortActions[i] = getNVSString(prefs, (buttonPrefix + "short_act").c_str(), defaultButtonActions[i].shortAction, 20);
        g_runtimeConfig.buttonLongActions[i] = getNVSString(prefs, (buttonPrefix + "long_act").c_str(), defaultButtonActions[i].longAction, 20);

        // Load MQTT topics and LED effects
        g_runtimeConfig.buttonShortMqttTopics[i] = getNVSString(prefs, (buttonPrefix + "short_mq").c_str(), defaultButtonActions[i].shortMqttTopic, 128);
        g_runtimeConfig.buttonLongMqttTopics[i] = getNVSString(prefs, (buttonPrefix + "long_mq").c_str(), defaultButtonActions[i].longMqttTopic, 128);

        // Load LED effect configuration with defaults from ButtonConfig struct
        g_runtimeConfig.buttonShortLedEffects[i] = getNVSString(prefs, (buttonPrefix + "short_led").c_str(), defaultButtonActions[i].shortLedEffect, 20);
        g_runtimeConfig.buttonLongLedEffects[i] = getNVSString(prefs, (buttonPrefix + "long_led").c_str(), defaultButtonActions[i].longLedEffect, 20);
    }

#if ENABLE_LEDS
    // Load LED configuration (board-aware GPIO limits)
    g_runtimeConfig.ledPin = getNVSInt(prefs, NVS_LED_PIN, DEFAULT_LED_PIN, 0, BOARD_MAX_GPIO);
    g_runtimeConfig.ledCount = getNVSInt(prefs, NVS_LED_COUNT, DEFAULT_LED_COUNT, 1, 1000);
    g_runtimeConfig.ledBrightness = getNVSInt(prefs, NVS_LED_BRIGHTNESS, DEFAULT_LED_BRIGHTNESS, 1, 255);
    // ledRefreshRate removed - hardcoded to 60 Hz in LedEffects

    // Load LED effects configuration (this will need custom handling if complex)
    g_runtimeConfig.ledEffects = getDefaultLedEffectsConfig();
#endif

    // Load memo configuration (4 memo slots)
    g_runtimeConfig.memos[0] = getNVSString(prefs, NVS_MEMO_1, defaultMemo1, 500);
    g_runtimeConfig.memos[1] = getNVSString(prefs, NVS_MEMO_2, defaultMemo2, 500);
    g_runtimeConfig.memos[2] = getNVSString(prefs, NVS_MEMO_3, defaultMemo3, 500);
    g_runtimeConfig.memos[3] = getNVSString(prefs, NVS_MEMO_4, defaultMemo4, 500);

    prefs.end();
    return true;
}

// Internal helper: loadDefaultConfigInternal (mutex already held)
void ConfigManager::loadDefaultConfigInternal()
{
    // const BoardPinDefaults &boardDefaults = getBoardDefaults();

    // Load device defaults
    g_runtimeConfig.deviceOwner = defaultDeviceOwner;
    g_runtimeConfig.timezone = defaultTimezone;

    // Load hardware GPIO defaults (board-specific)
    g_runtimeConfig.printerTxPin = defaultPrinterTxPin;
    g_runtimeConfig.printerRxPin = BOARD_PRINTER_RX_PIN;
    g_runtimeConfig.printerDtrPin = BOARD_PRINTER_DTR_PIN;
    g_runtimeConfig.buttonGpios[0] = BOARD_BUTTON_PINS[0];
    g_runtimeConfig.buttonGpios[1] = BOARD_BUTTON_PINS[1];
    g_runtimeConfig.buttonGpios[2] = BOARD_BUTTON_PINS[2];
    g_runtimeConfig.buttonGpios[3] = BOARD_BUTTON_PINS[3];

    // Load WiFi defaults (empty by default, must be configured)
    g_runtimeConfig.wifiSSID = defaultWifiSSID;
    g_runtimeConfig.wifiPassword = defaultWifiPassword;
    g_runtimeConfig.wifiConnectTimeoutMs = wifiConnectTimeoutMs;

    // Load defaults from config.h constants
    g_runtimeConfig.mqttEnabled = defaultMqttEnabled;
    g_runtimeConfig.mqttServer = defaultMqttServer;
    g_runtimeConfig.mqttPort = defaultMqttPort;
    g_runtimeConfig.mqttUsername = defaultMqttUsername;
    g_runtimeConfig.mqttPassword = defaultMqttPassword;

    g_runtimeConfig.jokeAPI = jokeAPI;
    g_runtimeConfig.quoteAPI = quoteAPI;
    g_runtimeConfig.triviaAPI = triviaAPI;
    g_runtimeConfig.newsAPI = newsAPI;
    g_runtimeConfig.chatgptApiToken = defaultChatgptApiToken;
    g_runtimeConfig.anthropicApiKey = defaultAnthropicApiKey;
    g_runtimeConfig.googleApiKey = defaultGoogleApiKey;
    g_runtimeConfig.aiProvider = defaultAiProvider;
    g_runtimeConfig.aiModel = defaultAiModel;
    g_runtimeConfig.aiTemperature = defaultAiTemperature;
    g_runtimeConfig.aiMaxTokens = defaultAiMaxTokens;
    g_runtimeConfig.chatgptApiEndpoint = chatgptApiEndpoint;
    g_runtimeConfig.anthropicApiEndpoint = anthropicApiEndpoint;
    g_runtimeConfig.googleApiEndpoint = googleApiEndpoint;

    g_runtimeConfig.maxCharacters = maxCharacters;

    g_runtimeConfig.unbiddenInkEnabled = defaultEnableUnbiddenInk;
    g_runtimeConfig.unbiddenInkStartHour = defaultUnbiddenInkStartHour;
    g_runtimeConfig.unbiddenInkEndHour = defaultUnbiddenInkEndHour;
    g_runtimeConfig.unbiddenInkFrequencyMinutes = defaultUnbiddenInkFrequencyMinutes;
    g_runtimeConfig.unbiddenInkPrompt = defaultUnbiddenInkPrompt;

    // Load default button configuration
    for (int i = 0; i < 4; i++)
    {
        g_runtimeConfig.buttonShortActions[i] = defaultButtonActions[i].shortAction;
        g_runtimeConfig.buttonShortMqttTopics[i] = defaultButtonActions[i].shortMqttTopic;
        g_runtimeConfig.buttonLongActions[i] = defaultButtonActions[i].longAction;
        g_runtimeConfig.buttonLongMqttTopics[i] = defaultButtonActions[i].longMqttTopic;

        // Load default LED effect configuration from ButtonConfig struct
        g_runtimeConfig.buttonShortLedEffects[i] = defaultButtonActions[i].shortLedEffect;
        g_runtimeConfig.buttonLongLedEffects[i] = defaultButtonActions[i].longLedEffect;
    }

#if ENABLE_LEDS
    // Load default LED configuration
    g_runtimeConfig.ledPin = DEFAULT_LED_PIN;
    g_runtimeConfig.ledCount = DEFAULT_LED_COUNT;
    g_runtimeConfig.ledBrightness = DEFAULT_LED_BRIGHTNESS;
    // ledRefreshRate removed - hardcoded to 60 Hz in LedEffects
    g_runtimeConfig.ledEffects = getDefaultLedEffectsConfig();
#endif

    g_configLoaded = true;
    LOG_NOTICE("CONFIG", "Using default configuration from config.h");
}

// Public method: saveNVSConfig (thread-safe)
bool ConfigManager::saveNVSConfig(const RuntimeConfig &config)
{
    if (!initialized) {
        LOG_ERROR("CONFIG", "ConfigManager not initialized - call begin() first!");
        return false;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "CONFIG");
    if (!lock.isLocked()) {
        LOG_ERROR("CONFIG", "Failed to acquire ConfigManager mutex!");
        return false;
    }

    bool result = saveNVSConfigInternal(config);

    // Mutex automatically released by ManagerLock destructor
    return result;
}

// Internal helper: saveNVSConfigInternal (mutex already held)
// Optimized to only write changed values to reduce NVS wear and fragmentation
bool ConfigManager::saveNVSConfigInternal(const RuntimeConfig &config)
{
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false))
    { // read-write
        LOG_ERROR("CONFIG", "Failed to open NVS namespace for writing: %s", NVS_NAMESPACE);
        return false;
    }

    int keysWritten = 0;
    int keysFailed = 0;

    // Helper macro to write only if value changed
    #define WRITE_IF_CHANGED_STRING(key, newValue) \
        do { \
            String currentValue = prefs.getString(key, ""); \
            if (currentValue != newValue) { \
                size_t written = prefs.putString(key, newValue); \
                if (written == 0) { \
                    LOG_ERROR("CONFIG", "Failed to write '%s' to NVS (storage may be full)", key); \
                    keysFailed++; \
                } else { \
                    keysWritten++; \
                } \
            } \
        } while(0)

    #define WRITE_IF_CHANGED_INT(key, newValue) \
        do { \
            int currentValue = prefs.getInt(key, -999999); \
            if (currentValue != newValue) { \
                size_t written = prefs.putInt(key, newValue); \
                if (written == 0) { \
                    LOG_ERROR("CONFIG", "Failed to write '%s' to NVS (storage may be full)", key); \
                    keysFailed++; \
                } else { \
                    keysWritten++; \
                } \
            } \
        } while(0)

    #define WRITE_IF_CHANGED_BOOL(key, newValue) \
        do { \
            bool currentValue = prefs.getBool(key, !newValue); \
            if (currentValue != newValue) { \
                size_t written = prefs.putBool(key, newValue); \
                if (written == 0) { \
                    LOG_ERROR("CONFIG", "Failed to write '%s' to NVS (storage may be full)", key); \
                    keysFailed++; \
                } else { \
                    keysWritten++; \
                } \
            } \
        } while(0)

    #define WRITE_IF_CHANGED_FLOAT(key, newValue) \
        do { \
            float currentValue = prefs.getFloat(key, -999999.0f); \
            if (currentValue != newValue) { \
                size_t written = prefs.putFloat(key, newValue); \
                if (written == 0) { \
                    LOG_ERROR("CONFIG", "Failed to write '%s' to NVS (storage may be full)", key); \
                    keysFailed++; \
                } else { \
                    keysWritten++; \
                } \
            } \
        } while(0)

    // Save device configuration (only if changed)
    WRITE_IF_CHANGED_STRING(NVS_DEVICE_OWNER, config.deviceOwner);
    WRITE_IF_CHANGED_STRING(NVS_DEVICE_TIMEZONE, config.timezone);

    // Save hardware GPIO configuration (only if changed)
    WRITE_IF_CHANGED_INT(NVS_PRINTER_TX_PIN, config.printerTxPin);
    WRITE_IF_CHANGED_INT(NVS_PRINTER_RX_PIN, config.printerRxPin);
    WRITE_IF_CHANGED_INT(NVS_PRINTER_DTR_PIN, config.printerDtrPin);
    WRITE_IF_CHANGED_INT(NVS_BUTTON1_GPIO, config.buttonGpios[0]);
    WRITE_IF_CHANGED_INT(NVS_BUTTON2_GPIO, config.buttonGpios[1]);
    WRITE_IF_CHANGED_INT(NVS_BUTTON3_GPIO, config.buttonGpios[2]);
    WRITE_IF_CHANGED_INT(NVS_BUTTON4_GPIO, config.buttonGpios[3]);

    // Save WiFi configuration (only if changed)
    WRITE_IF_CHANGED_STRING(NVS_WIFI_SSID, config.wifiSSID);
    WRITE_IF_CHANGED_STRING(NVS_WIFI_PASSWORD, config.wifiPassword);
    // Note: wifiConnectTimeoutMs is NOT saved - it's a runtime-only constant

    // Save MQTT configuration (only if changed)
    WRITE_IF_CHANGED_BOOL(NVS_MQTT_ENABLED, config.mqttEnabled);
    WRITE_IF_CHANGED_STRING(NVS_MQTT_SERVER, config.mqttServer);
    WRITE_IF_CHANGED_INT(NVS_MQTT_PORT, config.mqttPort);
    WRITE_IF_CHANGED_STRING(NVS_MQTT_USERNAME, config.mqttUsername);
    WRITE_IF_CHANGED_STRING(NVS_MQTT_PASSWORD, config.mqttPassword);

    // Save AI provider API tokens (other APIs are constants) (only if changed)
    WRITE_IF_CHANGED_STRING(NVS_CHATGPT_TOKEN, config.chatgptApiToken);
    WRITE_IF_CHANGED_STRING(NVS_ANTHROPIC_KEY, config.anthropicApiKey);
    WRITE_IF_CHANGED_STRING(NVS_GOOGLE_KEY, config.googleApiKey);
    WRITE_IF_CHANGED_STRING(NVS_AI_PROVIDER, config.aiProvider);
    WRITE_IF_CHANGED_STRING(NVS_AI_MODEL, config.aiModel);
    WRITE_IF_CHANGED_FLOAT(NVS_AI_TEMPERATURE, config.aiTemperature);
    WRITE_IF_CHANGED_INT(NVS_AI_MAX_TOKENS, config.aiMaxTokens);

    // Save Unbidden Ink configuration (only if changed)
    WRITE_IF_CHANGED_BOOL(NVS_UNBIDDEN_ENABLED, config.unbiddenInkEnabled);
    WRITE_IF_CHANGED_INT(NVS_UNBIDDEN_START_HOUR, config.unbiddenInkStartHour);
    WRITE_IF_CHANGED_INT(NVS_UNBIDDEN_END_HOUR, config.unbiddenInkEndHour);
    WRITE_IF_CHANGED_INT(NVS_UNBIDDEN_FREQUENCY, config.unbiddenInkFrequencyMinutes);
    WRITE_IF_CHANGED_STRING(NVS_UNBIDDEN_PROMPT, config.unbiddenInkPrompt);

    // Save button configuration (only if changed)
    for (int i = 0; i < 4; i++)
    {
        String buttonPrefix = "btn" + String(i + 1) + "_";
        WRITE_IF_CHANGED_STRING((buttonPrefix + "short_act").c_str(), config.buttonShortActions[i]);
        WRITE_IF_CHANGED_STRING((buttonPrefix + "short_mq").c_str(), config.buttonShortMqttTopics[i]);
        WRITE_IF_CHANGED_STRING((buttonPrefix + "long_act").c_str(), config.buttonLongActions[i]);
        WRITE_IF_CHANGED_STRING((buttonPrefix + "long_mq").c_str(), config.buttonLongMqttTopics[i]);

        // Save LED effect configuration (only if changed)
        WRITE_IF_CHANGED_STRING((buttonPrefix + "short_led").c_str(), config.buttonShortLedEffects[i]);
        WRITE_IF_CHANGED_STRING((buttonPrefix + "long_led").c_str(), config.buttonLongLedEffects[i]);
    }

#if ENABLE_LEDS
    // Save LED configuration (only if changed)
    WRITE_IF_CHANGED_INT(NVS_LED_PIN, config.ledPin);
    WRITE_IF_CHANGED_INT(NVS_LED_COUNT, config.ledCount);
    WRITE_IF_CHANGED_INT(NVS_LED_BRIGHTNESS, config.ledBrightness);
    // ledRefreshRate removed - hardcoded to 60 Hz in LedEffects

    // TODO: Save LED effects configuration if needed
#endif

    // Save memo configuration (4 memo slots) (only if changed)
    WRITE_IF_CHANGED_STRING(NVS_MEMO_1, config.memos[0]);
    WRITE_IF_CHANGED_STRING(NVS_MEMO_2, config.memos[1]);
    WRITE_IF_CHANGED_STRING(NVS_MEMO_3, config.memos[2]);
    WRITE_IF_CHANGED_STRING(NVS_MEMO_4, config.memos[3]);

    #undef WRITE_IF_CHANGED_STRING
    #undef WRITE_IF_CHANGED_INT
    #undef WRITE_IF_CHANGED_BOOL

    prefs.end();

    // Report results
    if (keysFailed > 0) {
        LOG_ERROR("CONFIG", "Configuration save FAILED: %d keys failed to write (NVS storage may be full)", keysFailed);
        return false;
    }

    if (keysWritten == 0) {
        LOG_VERBOSE("CONFIG", "Configuration unchanged - no NVS writes needed");
    } else {
        LOG_NOTICE("CONFIG", "Configuration saved to NVS (%d keys updated)", keysWritten);
    }

    return true;
}

// Public method: setRuntimeConfig (thread-safe)
void ConfigManager::setRuntimeConfig(const RuntimeConfig &config)
{
    if (!initialized) {
        LOG_ERROR("CONFIG", "ConfigManager not initialized - call begin() first!");
        return;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "CONFIG");
    if (!lock.isLocked()) {
        LOG_ERROR("CONFIG", "Failed to acquire ConfigManager mutex!");
        return;
    }

    g_runtimeConfig = config;
    g_configLoaded = true;

    // Mutex automatically released by ManagerLock destructor
}

// Public method: applyConfigChanges (thread-safe, zero-copy)
bool ConfigManager::applyConfigChanges(JsonObject jsonObj, String& errorMsg)
{
    if (!initialized) {
        LOG_ERROR("CONFIG", "ConfigManager not initialized - call begin() first!");
        errorMsg = "ConfigManager not initialized";
        return false;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "CONFIG");
    if (!lock.isLocked()) {
        LOG_ERROR("CONFIG", "Failed to acquire ConfigManager mutex!");
        errorMsg = "Failed to acquire configuration lock";
        return false;
    }

    // Apply changes directly to g_runtimeConfig (in-place mutation)
    // processJsonObject uses pointer arithmetic (offsetof) for efficient field updates
    if (!processJsonObject("", jsonObj, g_runtimeConfig, errorMsg)) {
        // Validation failed - rollback by reloading from NVS
        LOG_WARNING("CONFIG", "Validation failed, rolling back: %s", errorMsg.c_str());
        loadNVSConfigInternal();
        return false;
    }

    // Set runtime-only constants (these are NEVER in NVS or JSON)
    g_runtimeConfig.jokeAPI = jokeAPI;
    g_runtimeConfig.quoteAPI = quoteAPI;
    g_runtimeConfig.triviaAPI = triviaAPI;
    g_runtimeConfig.newsAPI = newsAPI;
    g_runtimeConfig.chatgptApiEndpoint = chatgptApiEndpoint;
    g_runtimeConfig.wifiConnectTimeoutMs = wifiConnectTimeoutMs;
    g_runtimeConfig.maxCharacters = maxCharacters;

    g_configLoaded = true;

    // Mutex automatically released by ManagerLock destructor
    return true;
}

// Public method: factoryResetNVS (thread-safe)
bool ConfigManager::factoryResetNVS()
{
    if (!initialized) {
        LOG_ERROR("CONFIG", "ConfigManager not initialized - call begin() first!");
        return false;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "CONFIG");
    if (!lock.isLocked()) {
        LOG_ERROR("CONFIG", "Failed to acquire ConfigManager mutex!");
        return false;
    }

    bool result = factoryResetNVSInternal();

    // Mutex automatically released by ManagerLock destructor
    return result;
}

// Public method: initializeConfigSystem
bool ConfigManager::initializeConfigSystem()
{
    // Initialize NVS-based configuration system
    return loadRuntimeConfig();
}

// Public method: initializeNVSConfig
bool ConfigManager::initializeNVSConfig()
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

// Public method: checkAndMigrateNVSSchema
bool ConfigManager::checkAndMigrateNVSSchema()
{
    // For now, just return true - migration logic would go here if needed
    // This could check version numbers, migrate old key names, etc.
    LOG_NOTICE("CONFIG", "NVS schema check complete (no migration needed)");
    return true;
}

// ============================================================================
// DIRECT ACCESS FUNCTIONS (NO MUTEX - READ-ONLY)
// ============================================================================

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

void loadDefaultConfig()
{
    // Call internal version directly (this is only called during initialization before mutex exists)
    ConfigManager::instance().loadDefaultConfigInternal();
}

// Internal helper: factoryResetNVSInternal (mutex already held)
bool ConfigManager::factoryResetNVSInternal()
{
    LOG_NOTICE("CONFIG", "Performing factory reset - erasing all NVS data");

    // Erase entire NVS partition
    esp_err_t err = nvs_flash_erase();
    if (err != ESP_OK)
    {
        LOG_ERROR("CONFIG", "Failed to erase NVS: %s", esp_err_to_name(err));
        return false;
    }

    // Reinitialize NVS
    err = nvs_flash_init();
    if (err != ESP_OK)
    {
        LOG_ERROR("CONFIG", "Failed to reinitialize NVS after erase: %s", esp_err_to_name(err));
        return false;
    }

    // Load defaults from config.h
    if (!loadNVSConfigInternal())
    {
        LOG_ERROR("CONFIG", "Failed to load default configuration after factory reset");
        return false;
    }

    LOG_NOTICE("CONFIG", "Factory reset completed - using defaults from config.h");
    return true;
}
