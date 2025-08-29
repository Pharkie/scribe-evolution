#pragma once

#include <ArduinoJson.h>
#include "core/config.h"
#include "core/runtime_config.h"

/**
 * @file config_field_registry.h
 * @brief Data-driven configuration field definitions and validation
 * 
 * This system provides a single source of truth for all configuration fields,
 * their validation rules, and automatic mapping to RuntimeConfig struct fields.
 * 
 * Adding a new config field requires only one entry in CONFIG_FIELDS array.
 */

enum class ValidationType {
    STRING,              // Any string value (including empty)
    NON_EMPTY_STRING,    // String that cannot be empty
    GPIO,                // GPIO pin number (validates safety)
    RANGE_INT,           // Integer within min/max range
    RANGE_FLOAT,         // Float within min/max range
    BOOLEAN,             // Boolean true/false
    ENUM_STRING          // String that must match one of provided options
};

struct ConfigFieldDef {
    const char* jsonPath;           // JSON path like "device.owner" or "buttons.button1.gpio"
    ValidationType validator;       // Validation type to apply
    size_t configFieldOffset;      // Offset in RuntimeConfig struct (use offsetof macro)
    int minValue;                  // For RANGE_INT/RANGE_FLOAT
    int maxValue;                  // For RANGE_INT/RANGE_FLOAT
    const char* const* enumValues; // For ENUM_STRING validation
    int enumCount;                 // Number of enum values
};

// Valid button actions for ENUM_STRING validation
extern const char* const VALID_BUTTON_ACTIONS[];
extern const int VALID_BUTTON_ACTIONS_COUNT;

// Valid LED effects for ENUM_STRING validation  
extern const char* const VALID_LED_EFFECTS[];
extern const int VALID_LED_EFFECTS_COUNT;

// Main configuration field registry - single source of truth
extern const ConfigFieldDef CONFIG_FIELDS[];
extern const int CONFIG_FIELDS_COUNT;

/**
 * @brief Find field definition by JSON path
 * @param jsonPath Path like "device.owner" or "buttons.button1.gpio"
 * @return Pointer to field definition, or nullptr if not found
 */
const ConfigFieldDef* findConfigField(const char* jsonPath);

/**
 * @brief Validate and update a single field in the config
 * @param field Field definition to validate against
 * @param value JSON value to validate and apply
 * @param config Runtime configuration to update
 * @param errorMsg Output parameter for validation error message
 * @return true if validation succeeded and field was updated
 */
bool validateAndUpdateField(const ConfigFieldDef* field, JsonVariant value, RuntimeConfig& config, String& errorMsg);

/**
 * @brief Process a JSON path and value, updating config if valid
 * @param jsonPath Complete path like "device.owner"
 * @param value JSON value to process
 * @param config Runtime configuration to update  
 * @param errorMsg Output parameter for validation error message
 * @return true if field was found, validated, and updated
 */
bool processConfigField(const char* jsonPath, JsonVariant value, RuntimeConfig& config, String& errorMsg);

/**
 * @brief Recursively process nested JSON objects, building dot-notation paths
 * @param pathPrefix Current path prefix (e.g., "device")
 * @param jsonObj JSON object to process
 * @param config Runtime configuration to update
 * @param errorMsg Output parameter for validation error message
 * @return true if all fields were successfully processed
 */
bool processJsonObject(const String& pathPrefix, JsonObject jsonObj, RuntimeConfig& config, String& errorMsg);