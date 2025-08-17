/**
 * @file config_loader.h
 * @brief Dynamic configuration loader for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// Runtime configuration structure
struct RuntimeConfig
{
    // Device Configuration
    String deviceOwner;
    String timezone;

    // WiFi Configuration
    String wifiSSID;
    String wifiPassword;
    unsigned long wifiConnectTimeoutMs;

    // MQTT Configuration
    String mqttServer;
    int mqttPort;
    String mqttUsername;
    String mqttPassword;

    // API Configuration
    String jokeAPI;
    String quoteAPI;
    String triviaAPI;
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

#ifdef ENABLE_LEDS
    // LED Configuration (runtime configurable)
    int ledPin;                  // GPIO pin for LED strip data
    int ledCount;                // Number of LEDs in the strip  
    int ledBrightness;           // LED brightness (0-255)
    int ledRefreshRate;          // Refresh rate in Hz
    int ledEffectFadeSpeed;      // Fade speed for transitions (1-255)
    int ledTwinkleDensity;       // Number of twinkle stars simultaneously
    int ledChaseSpeed;           // Chase effect speed (pixels per update)
    int ledMatrixDrops;          // Number of matrix drops simultaneously
#endif
};

/**
 * @brief Load configuration from config.json file
 * If file doesn't exist or is invalid, uses defaults from config.h
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
 * @brief Initialize configuration system - must be called after LittleFS.begin()
 * @return true if initialization successful
 */
bool initializeConfigSystem();

/**
 * @brief Check if configuration file exists and is valid
 * @return true if config.json exists and is parseable
 */
bool isConfigFileValid();

/**
 * @brief Create default config.json file if it doesn't exist
 * Uses constants from config.h to populate the file
 * @return true if file was created successfully
 */
bool createDefaultConfigFile();

#ifdef ENABLE_LEDS
/**
 * @brief Update LED configuration at runtime and apply changes
 * @param pin GPIO pin for LED strip data
 * @param count Number of LEDs in the strip
 * @param brightness LED brightness (0-255)
 * @param refreshRate Refresh rate in Hz
 * @param fadeSpeed Fade speed for transitions (1-255)
 * @param twinkleDensity Number of twinkle stars simultaneously
 * @param chaseSpeed Chase effect speed (pixels per update)
 * @param matrixDrops Number of matrix drops simultaneously
 * @return true if configuration updated successfully
 */
bool updateLedConfiguration(int pin, int count, int brightness, int refreshRate,
                           int fadeSpeed, int twinkleDensity, int chaseSpeed, int matrixDrops);

/**
 * @brief Save LED configuration to config.json
 * @return true if saved successfully
 */
bool saveLedConfiguration();

/**
 * @brief Get current LED configuration from runtime config
 * @param pin Output: GPIO pin for LED strip data  
 * @param count Output: Number of LEDs in the strip
 * @param brightness Output: LED brightness (0-255)
 * @param refreshRate Output: Refresh rate in Hz
 * @param fadeSpeed Output: Fade speed for transitions (1-255)
 * @param twinkleDensity Output: Number of twinkle stars simultaneously
 * @param chaseSpeed Output: Chase effect speed (pixels per update)
 * @param matrixDrops Output: Number of matrix drops simultaneously
 */
void getLedConfiguration(int& pin, int& count, int& brightness, int& refreshRate,
                        int& fadeSpeed, int& twinkleDensity, int& chaseSpeed, int& matrixDrops);
#endif

/**
 * @brief Global flag indicating if config has been loaded
 */
extern bool g_configLoaded;

#endif // CONFIG_LOADER_H