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

// Runtime configuration structure
struct RuntimeConfig {
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
    String buttonShortActions[4]; // Short press actions for buttons 1-4
    String buttonLongActions[4];  // Long press actions for buttons 1-4 (empty string = no action)
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
const RuntimeConfig& getRuntimeConfig();

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

#endif // CONFIG_LOADER_H