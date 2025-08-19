/**
 * @file api_nvs_handlers.cpp
 * @brief NVS debugging and diagnostic API endpoint handlers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "api_nvs_handlers.h"
#include "api_handlers.h" // For shared utilities
#include "../core/config_loader.h"
#include "../core/logging.h"
#include "../utils/time_utils.h"
#include <ArduinoJson.h>
#include <Preferences.h>

void handleNVSDump(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "NVS dump requested from %s", request->client()->remoteIP().toString().c_str());

    DynamicJsonDocument doc(6144); // Larger buffer for complete NVS dump

    Preferences prefs;
    if (!prefs.begin("scribe-app", true))
    { // read-only mode
        doc["error"] = "Failed to open NVS namespace";
        doc["namespace"] = "scribe-app";
        doc["status"] = "error";

        String response;
        serializeJson(doc, response);
        request->send(500, "application/json", response);
        return;
    }

    doc["namespace"] = "scribe-app";
    doc["status"] = "success";
    doc["timestamp"] = getFormattedDateTime();

    JsonObject keys = doc.createNestedObject("keys");
    int totalKeys = 0;
    int validKeys = 0;
    int correctedKeys = 0;
    int invalidKeys = 0;

    // Define ALL known NVS keys with their types and validation info
    struct NVSKey
    {
        const char *key;
        const char *type;
        const char *description;
        bool isSecret;
        int minValue;
        int maxValue;
    };

    const NVSKey knownKeys[] = {
        // Device Configuration
        {"device_owner", "string", "Device owner name", false, 0, 0},
        {"device_tz", "string", "Device timezone", false, 0, 0},
        {"max_chars", "int", "Maximum message characters", false, 50, 2000},

        // WiFi Configuration
        {"wifi_ssid", "string", "WiFi network SSID", false, 0, 0},
        {"wifi_pass", "string", "WiFi network password", true, 0, 0},
        {"wifi_timeout", "int", "WiFi connect timeout (ms)", false, 5000, 60000},

        // MQTT Configuration
        {"mqtt_enabled", "bool", "MQTT connection enabled", false, 0, 0},
        {"mqtt_server", "string", "MQTT broker hostname", false, 0, 0},
        {"mqtt_port", "int", "MQTT broker port", false, 1, 65535},
        {"mqtt_user", "string", "MQTT username", false, 0, 0},
        {"mqtt_pass", "string", "MQTT password", true, 0, 0},
        {"mqtt_tls", "bool", "MQTT TLS enabled", false, 0, 0},

        // API Configuration
        {"chatgpt_token", "string", "ChatGPT API token", true, 0, 0},

        // Unbidden Ink Configuration
        {"unbid_enabled", "bool", "Unbidden ink enabled", false, 0, 0},
        {"unbid_start_hr", "int", "Unbidden ink start hour", false, 0, 23},
        {"unbid_end_hr", "int", "Unbidden ink end hour", false, 0, 23},
        {"unbid_freq_min", "int", "Unbidden ink frequency (minutes)", false, 15, 1440},
        {"unbidden_prompt", "string", "Unbidden ink custom prompt", false, 0, 0},

        // Button Configuration - All 4 buttons
        {"btn1_short_act", "string", "Button 1 short press action", false, 0, 0},
        {"btn1_short_mq", "string", "Button 1 short press MQTT topic", false, 0, 0},
        {"btn1_long_act", "string", "Button 1 long press action", false, 0, 0},
        {"btn1_long_mq", "string", "Button 1 long press MQTT topic", false, 0, 0},
        {"btn2_short_act", "string", "Button 2 short press action", false, 0, 0},
        {"btn2_short_mq", "string", "Button 2 short press MQTT topic", false, 0, 0},
        {"btn2_long_act", "string", "Button 2 long press action", false, 0, 0},
        {"btn2_long_mq", "string", "Button 2 long press MQTT topic", false, 0, 0},
        {"btn3_short_act", "string", "Button 3 short press action", false, 0, 0},
        {"btn3_short_mq", "string", "Button 3 short press MQTT topic", false, 0, 0},
        {"btn3_long_act", "string", "Button 3 long press action", false, 0, 0},
        {"btn3_long_mq", "string", "Button 3 long press MQTT topic", false, 0, 0},
        {"btn4_short_act", "string", "Button 4 short press action", false, 0, 0},
        {"btn4_short_mq", "string", "Button 4 short press MQTT topic", false, 0, 0},
        {"btn4_long_act", "string", "Button 4 long press action", false, 0, 0},
        {"btn4_long_mq", "string", "Button 4 long press MQTT topic", false, 0, 0},

        // LED Configuration
        {"led_refresh", "int", "LED refresh rate", false, 10, 200},
        {"led_brightness", "int", "LED brightness level", false, 1, 255}};

    const int numKeys = sizeof(knownKeys) / sizeof(NVSKey);

    // Check each known key
    for (int i = 0; i < numKeys; i++)
    {
        const NVSKey &keyInfo = knownKeys[i];
        JsonObject keyObj = keys.createNestedObject(keyInfo.key);

        keyObj["type"] = keyInfo.type;
        keyObj["description"] = keyInfo.description;
        keyObj["exists"] = false;
        keyObj["value"] = nullptr;
        keyObj["status"] = "missing";
        keyObj["validation"] = "❌";

        if (strcmp(keyInfo.type, "string") == 0)
        {
            String value = prefs.getString(keyInfo.key, "");
            if (prefs.isKey(keyInfo.key))
            {
                keyObj["exists"] = true;
                totalKeys++;
                keyObj["length"] = value.length();

                if (keyInfo.isSecret && value.length() > 0)
                {
                    keyObj["value"] = value.length() > 8 ? value.substring(0, 2) + "●●●●●●●●" + value.substring(value.length() - 2) : "●●●●●●●●";
                }
                else
                {
                    keyObj["value"] = value;
                }

                keyObj["status"] = "valid";
                keyObj["validation"] = "✅";
                validKeys++;
            }
        }
        else if (strcmp(keyInfo.type, "int") == 0)
        {
            if (prefs.isKey(keyInfo.key))
            {
                keyObj["exists"] = true;
                totalKeys++;
                int value = prefs.getInt(keyInfo.key, 0);
                keyObj["value"] = value;

                // Validate integer ranges
                if (keyInfo.minValue != keyInfo.maxValue &&
                    (value < keyInfo.minValue || value > keyInfo.maxValue))
                {
                    keyObj["status"] = "corrected";
                    keyObj["validation"] = "⚠️";
                    keyObj["originalValue"] = value;
                    keyObj["note"] = String("Value ") + value + " outside valid range [" + keyInfo.minValue + "-" + keyInfo.maxValue + "]";
                    correctedKeys++;
                }
                else
                {
                    keyObj["status"] = "valid";
                    keyObj["validation"] = "✅";
                    validKeys++;
                }
            }
        }
        else if (strcmp(keyInfo.type, "bool") == 0)
        {
            if (prefs.isKey(keyInfo.key))
            {
                keyObj["exists"] = true;
                totalKeys++;
                bool value = prefs.getBool(keyInfo.key, false);
                keyObj["value"] = value;
                keyObj["status"] = "valid";
                keyObj["validation"] = "✅";
                validKeys++;
            }
        }

        if (!keyObj["exists"])
        {
            invalidKeys++;
        }
    }

    doc["totalKeys"] = totalKeys;
    doc["validKeys"] = validKeys;
    doc["correctedKeys"] = correctedKeys;
    doc["invalidKeys"] = invalidKeys;

    prefs.end();

    String response;
    serializeJson(doc, response);

    LOG_VERBOSE("WEB", "NVS dump completed - %d total, %d valid, %d corrected, %d invalid",
                totalKeys, validKeys, correctedKeys, invalidKeys);
    request->send(200, "application/json", response);
}
