#include "config_field_registry.h"
#include "config/config.h"
#include "core/logging.h"
#include <cstddef>

/**
 * @brief Validate IANA timezone string format
 * @param timezone Timezone string to validate
 * @return true if timezone appears to be a valid IANA timezone
 */
bool isValidIANATimezone(const String& timezone) {
    // Basic format validation for IANA timezone strings
    if (timezone.length() == 0 || timezone.length() > 50) {
        return false;
    }
    
    // Must contain at least one slash (Area/Location)
    if (timezone.indexOf('/') == -1) {
        return false;
    }
    
    // Cannot start or end with slash
    if (timezone.startsWith("/") || timezone.endsWith("/")) {
        return false;
    }
    
    // Cannot contain spaces, must use underscores
    if (timezone.indexOf(' ') != -1) {
        return false;
    }
    
    // Check for common valid patterns
    // Allow: Letters, numbers, underscores, slashes, hyphens, plus signs
    for (int i = 0; i < timezone.length(); i++) {
        char c = timezone.charAt(i);
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
              (c >= '0' && c <= '9') || c == '_' || c == '/' || c == '-' || c == '+')) {
            return false;
        }
    }
    
    // Validate common IANA timezone prefixes
    if (timezone.startsWith("Africa/") || 
        timezone.startsWith("America/") || 
        timezone.startsWith("Antarctica/") ||
        timezone.startsWith("Asia/") || 
        timezone.startsWith("Atlantic/") ||
        timezone.startsWith("Australia/") || 
        timezone.startsWith("Europe/") ||
        timezone.startsWith("Indian/") || 
        timezone.startsWith("Pacific/") ||
        timezone == "UTC" || timezone == "GMT" ||
        timezone.startsWith("Etc/")) {
        return true;
    }
    
    return false;
}

// Valid button actions
const char* const VALID_BUTTON_ACTIONS[] = {
    "JOKE", "RIDDLE", "QUOTE", "QUIZ", "NEWS", "CHARACTER_TEST", "POKE",
    "UNBIDDEN_INK", "MEMO1", "MEMO2", "MEMO3", "MEMO4", ""
};
const int VALID_BUTTON_ACTIONS_COUNT = 13;

// Valid LED effects
const char* const VALID_LED_EFFECTS[] = {
    "chase_single", "chase_multi", "rainbow", "twinkle", "pulse", "matrix", "none"
};
const int VALID_LED_EFFECTS_COUNT = 7;

// Configuration field registry - single source of truth
const ConfigFieldDef CONFIG_FIELDS[] = {
    // Device configuration
    {"device.owner", ValidationType::NON_EMPTY_STRING, offsetof(RuntimeConfig, deviceOwner), 0, 0, nullptr, 0},
    {"device.timezone", ValidationType::IANA_TIMEZONE, offsetof(RuntimeConfig, timezone), 0, 0, nullptr, 0},
    {"device.printerTxPin", ValidationType::GPIO, offsetof(RuntimeConfig, printerTxPin), 0, 0, nullptr, 0},
    {"device.printerRxPin", ValidationType::GPIO, offsetof(RuntimeConfig, printerRxPin), 0, 0, nullptr, 0},
    {"device.printerDtrPin", ValidationType::GPIO, offsetof(RuntimeConfig, printerDtrPin), 0, 0, nullptr, 0},
    
    // WiFi configuration
    {"wifi.ssid", ValidationType::NON_EMPTY_STRING, offsetof(RuntimeConfig, wifiSSID), 0, 0, nullptr, 0},
    {"wifi.password", ValidationType::STRING, offsetof(RuntimeConfig, wifiPassword), 0, 0, nullptr, 0},
    
    // MQTT configuration
    {"mqtt.enabled", ValidationType::BOOLEAN, offsetof(RuntimeConfig, mqttEnabled), 0, 0, nullptr, 0},
    {"mqtt.server", ValidationType::STRING, offsetof(RuntimeConfig, mqttServer), 0, 0, nullptr, 0},
    {"mqtt.port", ValidationType::RANGE_INT, offsetof(RuntimeConfig, mqttPort), 1, 65535, nullptr, 0},
    {"mqtt.username", ValidationType::STRING, offsetof(RuntimeConfig, mqttUsername), 0, 0, nullptr, 0},
    {"mqtt.password", ValidationType::STRING, offsetof(RuntimeConfig, mqttPassword), 0, 0, nullptr, 0},
    
    // Unbidden Ink configuration
    {"unbiddenInk.enabled", ValidationType::BOOLEAN, offsetof(RuntimeConfig, unbiddenInkEnabled), 0, 0, nullptr, 0},
    {"unbiddenInk.chatgptApiToken", ValidationType::STRING, offsetof(RuntimeConfig, chatgptApiToken), 0, 0, nullptr, 0},
    {"unbiddenInk.anthropicApiKey", ValidationType::STRING, offsetof(RuntimeConfig, anthropicApiKey), 0, 0, nullptr, 0},
    {"unbiddenInk.googleApiKey", ValidationType::STRING, offsetof(RuntimeConfig, googleApiKey), 0, 0, nullptr, 0},
    {"unbiddenInk.aiProvider", ValidationType::STRING, offsetof(RuntimeConfig, aiProvider), 0, 0, nullptr, 0},
    {"unbiddenInk.aiModel", ValidationType::STRING, offsetof(RuntimeConfig, aiModel), 0, 0, nullptr, 0},
    {"unbiddenInk.aiTemperature", ValidationType::RANGE_FLOAT, offsetof(RuntimeConfig, aiTemperature), 0, 2, nullptr, 0},
    {"unbiddenInk.aiMaxTokens", ValidationType::RANGE_INT, offsetof(RuntimeConfig, aiMaxTokens), 50, 500, nullptr, 0},
    {"unbiddenInk.startHour", ValidationType::RANGE_INT, offsetof(RuntimeConfig, unbiddenInkStartHour), 0, 24, nullptr, 0},
    {"unbiddenInk.endHour", ValidationType::RANGE_INT, offsetof(RuntimeConfig, unbiddenInkEndHour), 0, 24, nullptr, 0},
    {"unbiddenInk.frequencyMinutes", ValidationType::RANGE_INT, offsetof(RuntimeConfig, unbiddenInkFrequencyMinutes), 15, 480, nullptr, 0},
    {"unbiddenInk.prompt", ValidationType::NON_EMPTY_STRING, offsetof(RuntimeConfig, unbiddenInkPrompt), 0, 0, nullptr, 0},

    // Button GPIO configuration
    {"buttons.button1.gpio", ValidationType::GPIO, offsetof(RuntimeConfig, buttonGpios[0]), 0, 0, nullptr, 0},
    {"buttons.button2.gpio", ValidationType::GPIO, offsetof(RuntimeConfig, buttonGpios[1]), 0, 0, nullptr, 0},
    {"buttons.button3.gpio", ValidationType::GPIO, offsetof(RuntimeConfig, buttonGpios[2]), 0, 0, nullptr, 0},
    {"buttons.button4.gpio", ValidationType::GPIO, offsetof(RuntimeConfig, buttonGpios[3]), 0, 0, nullptr, 0},
    
    // Button action configuration
    {"buttons.button1.shortAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortActions[0]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    {"buttons.button1.longAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongActions[0]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    {"buttons.button2.shortAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortActions[1]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    {"buttons.button2.longAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongActions[1]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    {"buttons.button3.shortAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortActions[2]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    {"buttons.button3.longAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongActions[2]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    {"buttons.button4.shortAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortActions[3]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    {"buttons.button4.longAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongActions[3]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT},
    
    // Button LED effect configuration
    {"buttons.button1.shortLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortLedEffects[0]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    {"buttons.button1.longLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongLedEffects[0]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    {"buttons.button2.shortLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortLedEffects[1]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    {"buttons.button2.longLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongLedEffects[1]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    {"buttons.button3.shortLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortLedEffects[2]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    {"buttons.button3.longLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongLedEffects[2]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    {"buttons.button4.shortLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortLedEffects[3]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    {"buttons.button4.longLedEffect", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonLongLedEffects[3]), 0, 0, VALID_LED_EFFECTS, VALID_LED_EFFECTS_COUNT},
    
    // Button MQTT topic configuration
    {"buttons.button1.shortMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonShortMqttTopics[0]), 0, 0, nullptr, 0},
    {"buttons.button1.longMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonLongMqttTopics[0]), 0, 0, nullptr, 0},
    {"buttons.button2.shortMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonShortMqttTopics[1]), 0, 0, nullptr, 0},
    {"buttons.button2.longMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonLongMqttTopics[1]), 0, 0, nullptr, 0},
    {"buttons.button3.shortMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonShortMqttTopics[2]), 0, 0, nullptr, 0},
    {"buttons.button3.longMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonLongMqttTopics[2]), 0, 0, nullptr, 0},
    {"buttons.button4.shortMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonShortMqttTopics[3]), 0, 0, nullptr, 0},
    {"buttons.button4.longMqttTopic", ValidationType::STRING, offsetof(RuntimeConfig, buttonLongMqttTopics[3]), 0, 0, nullptr, 0},
    
#if ENABLE_LEDS
    // LED configuration
    {"leds.pin", ValidationType::GPIO, offsetof(RuntimeConfig, ledPin), 0, 0, nullptr, 0},
    {"leds.count", ValidationType::RANGE_INT, offsetof(RuntimeConfig, ledCount), 1, 300, nullptr, 0},
    {"leds.brightness", ValidationType::RANGE_INT, offsetof(RuntimeConfig, ledBrightness), 0, 255, nullptr, 0},
    // leds.refreshRate removed - hardcoded to 60 Hz
#endif
};

const int CONFIG_FIELDS_COUNT = sizeof(CONFIG_FIELDS) / sizeof(CONFIG_FIELDS[0]);

const ConfigFieldDef* findConfigField(const char* jsonPath) {
    for (int i = 0; i < CONFIG_FIELDS_COUNT; i++) {
        if (strcmp(CONFIG_FIELDS[i].jsonPath, jsonPath) == 0) {
            return &CONFIG_FIELDS[i];
        }
    }
    return nullptr;
}

bool validateAndUpdateField(const ConfigFieldDef* field, JsonVariant value, RuntimeConfig& config, String& errorMsg) {
    if (!field) {
        errorMsg = "Unknown field";
        return false;
    }
    
    // Get pointer to the field in the config struct
    char* basePtr = reinterpret_cast<char*>(&config);
    void* fieldPtr = basePtr + field->configFieldOffset;
    
    switch (field->validator) {
        case ValidationType::STRING: {
            String str = value.as<String>();
            *reinterpret_cast<String*>(fieldPtr) = str;
            return true;
        }
        
        case ValidationType::NON_EMPTY_STRING: {
            String str = value.as<String>();
            if (str.length() == 0) {
                errorMsg = String(field->jsonPath) + " cannot be empty";
                return false;
            }
            *reinterpret_cast<String*>(fieldPtr) = str;
            return true;
        }
        
        case ValidationType::IANA_TIMEZONE: {
            String str = value.as<String>();
            if (str.length() == 0) {
                errorMsg = String(field->jsonPath) + " cannot be empty";
                return false;
            }
            if (!isValidIANATimezone(str)) {
                errorMsg = String(field->jsonPath) + " invalid IANA timezone format: " + str + " (expected format: Area/Location, e.g., America/New_York, Europe/London)";
                return false;
            }
            *reinterpret_cast<String*>(fieldPtr) = str;
            return true;
        }
        
        case ValidationType::GPIO: {
            int gpio = value.as<int>();
            if (!isValidGPIO(gpio) || !isSafeGPIO(gpio)) {
                errorMsg = String(field->jsonPath) + " invalid GPIO pin: " + String(gpio) + " - " + String(getGPIODescription(gpio));
                return false;
            }
            *reinterpret_cast<int*>(fieldPtr) = gpio;
            return true;
        }
        
        case ValidationType::RANGE_INT: {
            int val = value.as<int>();
            if (val < field->minValue || val > field->maxValue) {
                errorMsg = String(field->jsonPath) + " must be between " + String(field->minValue) + " and " + String(field->maxValue);
                return false;
            }
            *reinterpret_cast<int*>(fieldPtr) = val;
            return true;
        }
        
        case ValidationType::RANGE_FLOAT: {
            float val = value.as<float>();
            if (val < field->minValue || val > field->maxValue) {
                errorMsg = String(field->jsonPath) + " must be between " + String(field->minValue) + " and " + String(field->maxValue);
                return false;
            }
            *reinterpret_cast<float*>(fieldPtr) = val;
            return true;
        }
        
        case ValidationType::BOOLEAN: {
            bool val = value.as<bool>();
            *reinterpret_cast<bool*>(fieldPtr) = val;
            return true;
        }
        
        case ValidationType::ENUM_STRING: {
            String str = value.as<String>();
            bool found = false;
            for (int i = 0; i < field->enumCount; i++) {
                if (str == field->enumValues[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                errorMsg = String(field->jsonPath) + " invalid value: " + str;
                return false;
            }
            *reinterpret_cast<String*>(fieldPtr) = str;
            return true;
        }
        
        default:
            errorMsg = String(field->jsonPath) + " unsupported validation type";
            return false;
    }
}

bool processConfigField(const char* jsonPath, JsonVariant value, RuntimeConfig& config, String& errorMsg) {
    const ConfigFieldDef* field = findConfigField(jsonPath);
    if (!field) {
        errorMsg = String("Unknown configuration field: ") + jsonPath;
        return false;
    }
    
    return validateAndUpdateField(field, value, config, errorMsg);
}

bool processJsonObject(const String& pathPrefix, JsonObject jsonObj, RuntimeConfig& config, String& errorMsg) {
    // Use ArduinoJson's flat iteration to avoid stack overflow - no recursion!
    // Process each key-value pair at current level only
    
    for (JsonPair kv : jsonObj) {
        const char* key = kv.key().c_str();
        JsonVariant value = kv.value();
        
        // Build path for this field
        String fieldPath;
        if (pathPrefix.length() > 0) {
            fieldPath = pathPrefix + "." + key;
        } else {
            fieldPath = key;
        }
        
        if (value.is<JsonObject>()) {
            // For nested objects, we need to handle them differently
            // Instead of recursion, process all known nested paths directly
            JsonObject nestedObj = value.as<JsonObject>();
            
            // Process each nested key-value pair
            for (JsonPair nestedKv : nestedObj) {
                String nestedPath = fieldPath + "." + nestedKv.key().c_str();
                
                if (nestedKv.value().is<JsonObject>()) {
                    // Handle double-nesting (like buttons.button1.*)
                    JsonObject doubleNestedObj = nestedKv.value().as<JsonObject>();
                    for (JsonPair doubleNestedKv : doubleNestedObj) {
                        String doublePath = nestedPath + "." + doubleNestedKv.key().c_str();
                        if (!processConfigField(doublePath.c_str(), doubleNestedKv.value(), config, errorMsg)) {
                            return false;
                        }
                    }
                } else {
                    // Single-nested field
                    if (!processConfigField(nestedPath.c_str(), nestedKv.value(), config, errorMsg)) {
                        return false;
                    }
                }
            }
        } else {
            // Top-level field
            if (!processConfigField(fieldPath.c_str(), value, config, errorMsg)) {
                return false;
            }
        }
        
        // Feed watchdog every few iterations
        yield();
    }
    
    return true;
}