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
#include <core/config_loader.h>
#include <core/nvs_keys.h>
#include <core/logging.h>
#include <utils/time_utils.h>
#include <ArduinoJson.h>
#include <Preferences.h>

void handleNVSDump(AsyncWebServerRequest *request)
{
    LOG_VERBOSE("WEB", "NVS dump requested from %s", request->client()->remoteIP().toString().c_str());

    // Use heap allocation for large JSON documents to prevent stack overflow
    std::unique_ptr<JsonDocument> doc(new JsonDocument());

    Preferences prefs;
    if (!prefs.begin("scribe-app", true))
    { // read-only mode
        (*doc)["error"] = "Failed to open NVS namespace";
        (*doc)["namespace"] = "scribe-app";
        (*doc)["status"] = "error";

        String response;
        serializeJson(*doc, response);
        request->send(500, "application/json", response);
        return;
    }

    (*doc)["namespace"] = "scribe-app";
    (*doc)["timestamp"] = getFormattedDateTime();

    JsonObject keys = doc->createNestedObject("keys");
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
        {NVS_DEVICE_OWNER, "string", "Device owner name", false, 0, 0},
        {NVS_DEVICE_TIMEZONE, "string", "Device timezone", false, 0, 0},

        // WiFi Configuration
        {NVS_WIFI_SSID, "string", "WiFi network SSID", false, 0, 0},
        {NVS_WIFI_PASSWORD, "string", "WiFi network password", true, 0, 0},
        {NVS_WIFI_TIMEOUT, "int", "WiFi connect timeout (ms)", false, 5000, 60000},

        // MQTT Configuration
        {NVS_MQTT_ENABLED, "bool", "MQTT enabled flag", false, 0, 0},
        {NVS_MQTT_SERVER, "string", "MQTT broker server", false, 0, 0},
        {NVS_MQTT_PORT, "int", "MQTT broker port", false, 1, 65535},
        {NVS_MQTT_USERNAME, "string", "MQTT username", false, 0, 0},
        {NVS_MQTT_PASSWORD, "string", "MQTT password", true, 0, 0},

        // API Configuration
        {NVS_CHATGPT_TOKEN, "string", "ChatGPT API token", true, 0, 0},

        // Unbidden Ink Configuration
        {NVS_UNBIDDEN_ENABLED, "bool", "Unbidden Ink enabled", false, 0, 0},
        {NVS_UNBIDDEN_FREQUENCY, "int", "Unbidden Ink frequency (minutes)", false, 30, 1440},
        {NVS_UNBIDDEN_START_HOUR, "int", "Unbidden Ink start hour", false, 0, 23},
        {NVS_UNBIDDEN_END_HOUR, "int", "Unbidden Ink end hour", false, 0, 23},
        {NVS_UNBIDDEN_PROMPT, "string", "Unbidden Ink prompt template", false, 0, 0},

        // Memo Configuration
        {NVS_MEMO_1, "string", "Memo 1 content", false, 0, 0},
        {NVS_MEMO_2, "string", "Memo 2 content", false, 0, 0},
        {NVS_MEMO_3, "string", "Memo 3 content", false, 0, 0},
        {NVS_MEMO_4, "string", "Memo 4 content", false, 0, 0},

        // Button Configuration (4 buttons × 4 fields = 16 keys)
        {"btn1_short_act", "string", "Button 1 short press action", false, 0, 0},
        {"btn1_short_mq", "string", "Button 1 short press MQTT topic", false, 0, 0},
        {"btn1_long_act", "string", "Button 1 long press action", false, 0, 0},
        {"btn1_long_mq", "string", "Button 1 long press MQTT topic", false, 0, 0},
        {"btn1_short_led", "string", "Button 1 short press LED effect", false, 0, 0},
        {"btn1_long_led", "string", "Button 1 long press LED effect", false, 0, 0},
        {"btn2_short_act", "string", "Button 2 short press action", false, 0, 0},
        {"btn2_short_mq", "string", "Button 2 short press MQTT topic", false, 0, 0},
        {"btn2_long_act", "string", "Button 2 long press action", false, 0, 0},
        {"btn2_long_mq", "string", "Button 2 long press MQTT topic", false, 0, 0},
        {"btn2_short_led", "string", "Button 2 short press LED effect", false, 0, 0},
        {"btn2_long_led", "string", "Button 2 long press LED effect", false, 0, 0},
        {"btn3_short_act", "string", "Button 3 short press action", false, 0, 0},
        {"btn3_short_mq", "string", "Button 3 short press MQTT topic", false, 0, 0},
        {"btn3_long_act", "string", "Button 3 long press action", false, 0, 0},
        {"btn3_long_mq", "string", "Button 3 long press MQTT topic", false, 0, 0},
        {"btn3_short_led", "string", "Button 3 short press LED effect", false, 0, 0},
        {"btn3_long_led", "string", "Button 3 long press LED effect", false, 0, 0},
        {"btn4_short_act", "string", "Button 4 short press action", false, 0, 0},
        {"btn4_short_mq", "string", "Button 4 short press MQTT topic", false, 0, 0},
        {"btn4_long_act", "string", "Button 4 long press action", false, 0, 0},
        {"btn4_long_mq", "string", "Button 4 long press MQTT topic", false, 0, 0},
        {"btn4_short_led", "string", "Button 4 short press LED effect", false, 0, 0},
        {"btn4_long_led", "string", "Button 4 long press LED effect", false, 0, 0},

#if ENABLE_LEDS
        // LED Configuration (only when ENABLE_LEDS is defined)
        {NVS_LED_PIN, "int", "LED strip GPIO pin", false, 0, 39},
        {NVS_LED_COUNT, "int", "Number of LEDs", false, 1, 1000},
        {NVS_LED_BRIGHTNESS, "int", "LED brightness", false, 1, 255},
        {NVS_LED_REFRESH_RATE, "int", "LED refresh rate", false, 10, 120}
#endif
    };

    const size_t numKeys = sizeof(knownKeys) / sizeof(knownKeys[0]);

    // Check each known key
    for (size_t i = 0; i < numKeys; i++)
    {
        const NVSKey &keyInfo = knownKeys[i];
        JsonObject keyObj = keys[keyInfo.key].to<JsonObject>();

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

    // Add summary object with counts
    JsonObject summary = (*doc)["summary"].to<JsonObject>();
    summary["totalKeys"] = totalKeys;
    summary["validKeys"] = validKeys;  
    summary["correctedKeys"] = correctedKeys;
    summary["invalidKeys"] = invalidKeys;

    prefs.end();

    String response;
    size_t jsonSize = serializeJson(*doc, response);

    LOG_VERBOSE("WEB", "NVS JSON serialization: %d bytes", jsonSize);
    if (jsonSize == 0) {
        LOG_ERROR("WEB", "NVS JSON serialization failed - response too large or memory issue");
        doc->clear();
        (*doc)["error"] = "JSON serialization failed - response too large";
        (*doc)["namespace"] = "scribe-app";
        (*doc)["status"] = "error";
        response = "";
        serializeJson(*doc, response);
        request->send(500, "application/json", response);
        return;
    }

    LOG_VERBOSE("WEB", "NVS dump completed - %d total, %d valid, %d corrected, %d invalid",
                totalKeys, validKeys, correctedKeys, invalidKeys);
    request->send(200, "application/json", response);
}
