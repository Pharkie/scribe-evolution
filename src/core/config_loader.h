/**
 * @file config_loader.h
 * @brief NVS-based configuration loader for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <config/config.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief Runtime configuration structure combining NVS-backed and compile-time settings
 *
 * This struct intentionally contains TWO types of configuration:
 *
 * 1. NVS-BACKED SETTINGS (User-Configurable via Web Interface)
 *    - Saved to ESP32 Non-Volatile Storage across reboots
 *    - Exposed in config_field_registry.cpp for web API access
 *    - Initial values from device_config.h, runtime values from NVS
 *    - ~51 total NVS keys (see nvs_keys.h for complete list)
 *
 * 2. RUNTIME-ONLY CONSTANTS (Compile-Time Configuration)
 *    - Always loaded from system_constants.h at runtime
 *    - NEVER saved to or loaded from NVS
 *    - NOT exposed in web configuration interface
 *    - Examples: API endpoints, timeouts, buffer sizes, validation limits
 *
 * This design provides a single source of truth for all configuration while
 * maintaining clear separation between user-configurable and system constants.
 */
struct RuntimeConfig
{
    // ===== NVS-BACKED SETTINGS (User-Configurable) =====

    // Device Configuration (web: device.*)
    String deviceOwner;              // Web: device.owner
    String timezone;                 // Web: device.timezone (IANA format)

    // Hardware GPIO Configuration (web: device.*)
    int printerTxPin;                // Web: device.printerTxPin
    int printerRxPin;                // Web: device.printerRxPin (bidirectional comms, -1 if disabled)
    int printerDtrPin;               // Web: device.printerDtrPin (hardware flow control, -1 if disabled)
    int buttonGpios[4];              // Web: buttons.button{1-4}.gpio

    // WiFi Configuration (web: wifi.*)
    String wifiSSID;                 // Web: wifi.ssid
    String wifiPassword;             // Web: wifi.password

    // MQTT Configuration (web: mqtt.*)
    bool mqttEnabled;                // Web: mqtt.enabled
    String mqttServer;               // Web: mqtt.server
    int mqttPort;                    // Web: mqtt.port
    String mqttUsername;             // Web: mqtt.username
    String mqttPassword;             // Web: mqtt.password

    // API Configuration (web: unbiddenInk.*)
    String chatgptApiToken;          // Web: unbiddenInk.chatgptApiToken (OpenAI API key)
    String anthropicApiKey;          // Web: unbiddenInk.anthropicApiKey
    String googleApiKey;             // Web: unbiddenInk.googleApiKey
    String aiProvider;               // Web: unbiddenInk.aiProvider (openai/anthropic/google)
    String aiModel;                  // Web: unbiddenInk.aiModel (provider-specific model name)
    float aiTemperature;             // Web: unbiddenInk.aiTemperature (0.0-2.0)
    int aiMaxTokens;                 // Web: unbiddenInk.aiMaxTokens (50-500)

    // Unbidden Ink Configuration (web: unbiddenInk.*)
    bool unbiddenInkEnabled;         // Web: unbiddenInk.enabled
    int unbiddenInkStartHour;        // Web: unbiddenInk.startHour (0-24)
    int unbiddenInkEndHour;          // Web: unbiddenInk.endHour (0-24)
    int unbiddenInkFrequencyMinutes; // Web: unbiddenInk.frequencyMinutes (15-480)
    String unbiddenInkPrompt;        // Web: unbiddenInk.prompt

    // Button Configuration (web: buttons.button{1-4}.*)
    String buttonShortActions[4];    // Web: buttons.button{1-4}.shortAction
    String buttonLongActions[4];     // Web: buttons.button{1-4}.longAction
    String buttonShortMqttTopics[4]; // Web: buttons.button{1-4}.shortMqttTopic
    String buttonLongMqttTopics[4];  // Web: buttons.button{1-4}.longMqttTopic
    String buttonShortLedEffects[4]; // Web: buttons.button{1-4}.shortLedEffect
    String buttonLongLedEffects[4];  // Web: buttons.button{1-4}.longLedEffect

    // Memo Configuration (NOT in web registry - use /api/print-memo endpoint)
    String memos[4];                 // Memo content for slots 1-4

#if ENABLE_LEDS
    // LED Configuration (web: leds.*)
    int ledPin;                      // Web: leds.pin
    int ledCount;                    // Web: leds.count (1-300)
    int ledBrightness;               // Web: leds.brightness (0-255)
    // ledRefreshRate removed - hardcoded to 60 Hz (see DEFAULT_LED_REFRESH_RATE in led_config.h)
    LedEffectsConfig ledEffects;     // Per-effect autonomous configuration
#endif

    // ===== RUNTIME-ONLY CONSTANTS (NOT in NVS or Web Config) =====

    // WiFi Constants (from system_constants.h)
    unsigned long wifiConnectTimeoutMs; // Compile-time constant (15 seconds)

    // API Endpoints (from system_constants.h)
    String jokeAPI;                  // Fixed: https://icanhazdadjoke.com/
    String quoteAPI;                 // Fixed: https://zenquotes.io/api/random
    String triviaAPI;                // Fixed: https://the-trivia-api.com/...
    String newsAPI;                  // Fixed: https://feeds.bbci.co.uk/news/rss.xml
    String chatgptApiEndpoint;       // Fixed: https://api.openai.com/v1/chat/completions
    String anthropicApiEndpoint;     // Fixed: https://api.anthropic.com/v1/messages
    String googleApiEndpoint;        // Fixed: https://generativelanguage.googleapis.com/v1beta/models/

    // Validation Configuration (from system_constants.h)
    int maxCharacters;               // Maximum characters for text input
};

/**
 * @class ConfigManager
 * @brief Thread-safe singleton for NVS/LittleFS configuration operations
 *
 * Provides mutex-protected access to NVS (Non-Volatile Storage) and LittleFS
 * for saving/loading configuration. Read operations (getRuntimeConfig) remain
 * direct for performance.
 *
 * Thread Safety:
 * - Write operations (save/set/reset) use mutex protection
 * - Read operations remain direct (no mutex needed for const reads)
 * - Safe for concurrent access from AsyncWebServer, buttons, and main loop
 *
 * Usage:
 *   ConfigManager::instance().begin();  // Call once in setup()
 *   ConfigManager::instance().saveNVSConfig(config);  // Thread-safe save
 *   const RuntimeConfig& config = getRuntimeConfig();  // Direct read
 */
class ConfigManager
{
public:
    static ConfigManager& instance();

    /**
     * @brief Initialize ConfigManager (creates mutex)
     * Must be called once in setup() before any config operations
     */
    void begin();

    /**
     * @brief Load configuration from NVS storage
     * If NVS is empty or invalid, populates with defaults from config.h
     * @return true if configuration loaded successfully, false if using defaults
     */
    bool loadRuntimeConfig();

    /**
     * @brief Save complete configuration to NVS storage (thread-safe)
     * @param config Configuration to save
     * @return true if configuration was saved successfully
     */
    bool saveNVSConfig(const RuntimeConfig &config);

    /**
     * @brief Update the global runtime configuration (thread-safe)
     * @param config New configuration to set
     */
    void setRuntimeConfig(const RuntimeConfig &config);

    /**
     * @brief Apply JSON config changes directly to runtime config (thread-safe, zero-copy)
     * @param jsonObj JSON object with partial configuration updates
     * @param errorMsg Output parameter for validation error message
     * @return true if all fields were validated and applied successfully
     *
     * This method validates and applies changes in-place to the global config.
     * On validation failure, automatically rolls back by reloading from NVS.
     * This eliminates the need for defensive copying before validation.
     */
    bool applyConfigChanges(JsonObject jsonObj, String& errorMsg);

    /**
     * @brief Factory reset - erase all NVS data and reload defaults (thread-safe)
     * @return true if factory reset was successful
     */
    bool factoryResetNVS();

    /**
     * @brief Initialize NVS configuration system
     * @return true if initialization successful
     */
    bool initializeConfigSystem();

    /**
     * @brief Initialize NVS with default values from config.h
     * Called on first boot or when schema version changes
     * @return true if NVS was initialized successfully
     */
    bool initializeNVSConfig();

    /**
     * @brief Check NVS schema version and migrate if needed
     * @return true if schema is current or migration successful
     */
    bool checkAndMigrateNVSSchema();

    /**
     * @brief Load default configuration from config.h constants (internal use)
     * Public only for getRuntimeConfig() first-call initialization
     */
    void loadDefaultConfigInternal();

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Internal helpers (mutex already held by caller)
    bool loadNVSConfigInternal();
    bool saveNVSConfigInternal(const RuntimeConfig &config);
    bool factoryResetNVSInternal();

    SemaphoreHandle_t mutex = nullptr;
    bool initialized = false;
};

// ============================================================================
// BACKWARD-COMPATIBLE WRAPPER FUNCTIONS
// ============================================================================

/**
 * @brief Load configuration from NVS storage
 * Wrapper for ConfigManager::instance().loadRuntimeConfig()
 */
inline bool loadRuntimeConfig() {
    return ConfigManager::instance().loadRuntimeConfig();
}

/**
 * @brief Save complete configuration to NVS storage (thread-safe)
 * Wrapper for ConfigManager::instance().saveNVSConfig()
 */
inline bool saveNVSConfig(const RuntimeConfig &config) {
    return ConfigManager::instance().saveNVSConfig(config);
}

/**
 * @brief Update the global runtime configuration (thread-safe)
 * Wrapper for ConfigManager::instance().setRuntimeConfig()
 */
inline void setRuntimeConfig(const RuntimeConfig &config) {
    ConfigManager::instance().setRuntimeConfig(config);
}

/**
 * @brief Factory reset - erase all NVS data and reload defaults (thread-safe)
 * Wrapper for ConfigManager::instance().factoryResetNVS()
 */
inline bool factoryResetNVS() {
    return ConfigManager::instance().factoryResetNVS();
}

/**
 * @brief Initialize NVS configuration system
 * Wrapper for ConfigManager::instance().initializeConfigSystem()
 */
inline bool initializeConfigSystem() {
    return ConfigManager::instance().initializeConfigSystem();
}

/**
 * @brief Initialize NVS with default values from config.h
 * Wrapper for ConfigManager::instance().initializeNVSConfig()
 */
inline bool initializeNVSConfig() {
    return ConfigManager::instance().initializeNVSConfig();
}

/**
 * @brief Check NVS schema version and migrate if needed
 * Wrapper for ConfigManager::instance().checkAndMigrateNVSSchema()
 */
inline bool checkAndMigrateNVSSchema() {
    return ConfigManager::instance().checkAndMigrateNVSSchema();
}

// ============================================================================
// DIRECT ACCESS FUNCTIONS (NO MUTEX - READ-ONLY)
// ============================================================================

/**
 * @brief Get the current runtime configuration (direct read, no mutex)
 * @return Reference to the runtime configuration
 */
const RuntimeConfig &getRuntimeConfig();

/**
 * @brief Load default configuration from config.h constants
 * Internal use only - called during initialization
 */
void loadDefaultConfig();

#if ENABLE_LEDS
#include "led_config.h"
#include "led_config_loader.h"
#endif

/**
 * @brief Global flag indicating if config has been loaded
 */
extern bool g_configLoaded;

#endif // CONFIG_LOADER_H