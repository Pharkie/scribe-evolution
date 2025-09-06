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
#include "../config/config.h"

// Runtime configuration structure
struct RuntimeConfig
{
    // Device Configuration
    String deviceOwner;
    String timezone;
    
    // Hardware GPIO Configuration
    int printerTxPin;    // GPIO pin for printer TX (UART communication)
    int buttonGpios[4];  // GPIO pins for buttons 1-4

    // WiFi Configuration
    String wifiSSID;
    String wifiPassword;
    unsigned long wifiConnectTimeoutMs;

    // MQTT Configuration
    bool mqttEnabled;
    String mqttServer;
    int mqttPort;
    String mqttUsername;
    String mqttPassword;

    // API Configuration
    String jokeAPI;
    String quoteAPI;
    String triviaAPI;
    String newsAPI;
    String betterStackToken;
    String betterStackEndpoint;
    String chatgptApiToken;
    String chatgptApiEndpoint;

    // Validation Configuration (only user-configurable parts)
    int maxCharacters;

    // Unbidden Ink Configuration
    bool unbiddenInkEnabled;
    int unbiddenInkStartHour;
    int unbiddenInkEndHour;
    int unbiddenInkFrequencyMinutes;
    String unbiddenInkPrompt;

    // Button Configuration (exactly 4 buttons)
    String buttonShortActions[4];    // Short press actions for buttons 1-4
    String buttonLongActions[4];     // Long press actions for buttons 1-4 (empty string = no action)
    String buttonShortMqttTopics[4]; // MQTT topics for short press actions (empty string = use local print)
    String buttonLongMqttTopics[4];  // MQTT topics for long press actions (empty string = use local print)

    // Button LED Effects Configuration (exactly 4 buttons)
    String buttonShortLedEffects[4]; // LED effects for short press actions (default: simple_chase)
    String buttonLongLedEffects[4];  // LED effects for long press actions (default: simple_chase)

    // Memo Configuration (4 memo slots)
    String memos[4]; // Memo content for slots 1-4

#if ENABLE_LEDS
    // LED Configuration (runtime configurable)
    int ledPin;         // GPIO pin for LED strip data
    int ledCount;       // Number of LEDs in the strip
    int ledBrightness;  // LED brightness (0-255)
    int ledRefreshRate; // Refresh rate in Hz
    // Per-effect autonomous configuration
    LedEffectsConfig ledEffects; // Contains all effect-specific settings
#endif
};

/**
 * @brief Load configuration from NVS storage
 * If NVS is empty or invalid, populates with defaults from config.h
 * @return true if configuration loaded successfully, false if using defaults
 */
bool loadRuntimeConfig();

/**
 * @brief Load default configuration from config.h constants
 */
void loadDefaultConfig();

/**
 * @brief Get the current runtime configuration
 * @return Reference to the runtime configuration
 */
const RuntimeConfig &getRuntimeConfig();

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
 * @brief Load configuration from NVS storage
 * @return true if configuration loaded successfully
 */
bool loadNVSConfig();

/**
 * @brief Save complete configuration to NVS storage
 * @param config Configuration to save
 * @return true if configuration was saved successfully
 */
bool saveNVSConfig(const RuntimeConfig &config);

/**
 * @brief Check NVS schema version and migrate if needed
 * @return true if schema is current or migration successful
 */
bool checkAndMigrateNVSSchema();

/**
 * @brief Update the global runtime configuration and save to NVS
 * @param config New configuration to set
 */
void setRuntimeConfig(const RuntimeConfig &config);

/**
 * @brief Factory reset - erase all NVS data and reload defaults
 * @return true if factory reset was successful
 */
bool factoryResetNVS();

#if ENABLE_LEDS
#include "led_config.h"
#include "led_config_loader.h"
#endif

/**
 * @brief Global flag indicating if config has been loaded
 */
extern bool g_configLoaded;

#endif // CONFIG_LOADER_H