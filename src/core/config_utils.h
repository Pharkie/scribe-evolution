/**
 * @file config_utils.h
 * @brief Configuration validation and utilities for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 */

#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <config/config.h>
#include "config_loader.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <Arduino.h>
#include <web/validation.h>

// ========================================
// STRING BUILDING UTILITIES
// ========================================

// Global string buffers
// Static string buffers for efficiency
static char derivedMqttTopic[stringBufferSize];
static char derivedMdnsHostname[stringBufferSize];
static char otherTopics[maxOtherPrinters][topicBufferSize];

// String building functions
inline const char *buildMqttTopic(const char *key)
{
    String topic = MqttTopics::buildPrintTopic(key);
    strncpy(derivedMqttTopic, topic.c_str(), sizeof(derivedMqttTopic));
    derivedMqttTopic[sizeof(derivedMqttTopic) - 1] = '\0'; // Ensure null termination
    return derivedMqttTopic;
}

inline const char *buildPersistentMqttTopic(int index, const char *key)
{
    if (index >= 0 && index < maxOtherPrinters)
    {
        String topic = MqttTopics::buildPrintTopic(key);
        strncpy(otherTopics[index], topic.c_str(), sizeof(otherTopics[index]));
        otherTopics[index][sizeof(otherTopics[index]) - 1] = '\0'; // Ensure null termination
        return otherTopics[index];
    }
    return "";
}

// ========================================
// SIMPLIFIED CONFIGURATION VALIDATION
// ========================================

// Unified string validation helper
inline bool isValidString(const char *str, int maxLen, const char *fieldName, String &error)
{
    if (!str || strlen(str) == 0)
    {
        error = String(fieldName) + " cannot be empty";
        return false;
    }
    if (strlen(str) > maxLen)
    {
        error = String(fieldName) + " too long (max " + String(maxLen) + " chars)";
        return false;
    }
    return true;
}

// ========================================
// CONFIGURATION ACCESS FUNCTIONS
// ========================================

// Function declarations - defined first so they can be used by validation
inline const char *getDeviceOwnerKey()
{
    // Check if config system is initialized first to avoid accessing uninitialized strings
    // Use the config loader's initialization status
    extern bool g_configLoaded; // Reference to the global from config_loader.cpp

    if (!g_configLoaded)
    {
        // If config not loaded yet, return default to avoid string access issues
        return defaultDeviceOwner;
    }

    // Use runtime config if available, otherwise fallback to default
    const RuntimeConfig &config = getRuntimeConfig();
    return config.deviceOwner.isEmpty() ? defaultDeviceOwner : config.deviceOwner.c_str();
}

// ========================================
// CONFIGURATION VALIDATION
// ========================================

// Main configuration validation function (includes GPIO validation)
void validateConfig();

// GPIO usage summary function
void logGPIOUsageSummary();

// Simple validation functions
inline ValidationResult validateDeviceConfig()
{
    String error;
    const RuntimeConfig &config = getRuntimeConfig();

    // For validation purposes, we need to check if required config exists
    // Since this is called during validation, use defaults if runtime not loaded yet
    const char *deviceOwner = getDeviceOwnerKey();

    // Validate deviceOwner
    if (!isValidString(deviceOwner, 32, "Device owner", error))
    {
        return ValidationResult(false, error.c_str());
    }

    return ValidationResult(true);
}

// ========================================
// MORE CONFIGURATION ACCESS FUNCTIONS
// ========================================

inline const char *getWifiSSID()
{
    return defaultWifiSSID;
}

inline const char *getWifiPassword()
{
    return defaultWifiPassword;
}

inline const char *getLocalPrinterName()
{
    // Check if we have a registered mDNS hostname (handles conflict resolution)
    extern const char* getRegisteredMdnsHostname();
    const char* registered = getRegisteredMdnsHostname();

    if (registered != nullptr && registered[0] != '\0')
    {
        // Extract printer name from registered hostname (strip "scribe-" prefix)
        // e.g., "scribe-pharkie2" -> "pharkie2"
        if (strncmp(registered, "scribe-", 7) == 0)
        {
            return registered + 7; // Return pointer to name after "scribe-"
        }
        // Fallback if hostname doesn't have expected prefix
        return registered;
    }

    // Fallback: Use deviceOwner (AP mode, early boot before mDNS setup)
    return getDeviceOwnerKey();
}

inline const char *getLocalPrinterTopic()
{
    // Use printer name (which includes mDNS conflict resolution suffix if present)
    // This ensures MQTT topics stay synchronized with mDNS hostnames
    return buildMqttTopic(getLocalPrinterName());
}

inline const char *getMdnsHostname()
{
    // First, check if we have a registered hostname from setupmDNS()
    // (requires forward declaration or include of network.h)
    extern const char* getRegisteredMdnsHostname();
    const char* registered = getRegisteredMdnsHostname();

    if (registered != nullptr && registered[0] != '\0')
    {
        // Return the actual registered hostname (may have conflict suffix like "scribe-pharkie2")
        return registered;
    }

    // Fallback: Build desired hostname from deviceOwner for pre-setup calls
    // This is used by setupmDNS() to determine what name to try first
    snprintf(derivedMdnsHostname, sizeof(derivedMdnsHostname), "scribe-%s", getDeviceOwnerKey());
    // Convert to lowercase for URL compatibility
    for (int i = 0; derivedMdnsHostname[i]; i++)
    {
        derivedMdnsHostname[i] = tolower(derivedMdnsHostname[i]);
    }
    return derivedMdnsHostname;
}

inline const char *getTimezone()
{
    return defaultTimezone;
}

// Simple initialization function
inline void initializePrinterConfig()
{
    Serial.begin(115200);
    delay(100);
    // Configuration initialized silently - details available via diagnostics page
}

#endif
