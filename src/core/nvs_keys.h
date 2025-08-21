/**
 * @file nvs_keys.h
 * @brief Central definition of all NVS key names used throughout the Scribe project
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 *
 * This file ensures DRY principle by defining all NVS keys in one place.
 * All modules that read from or write to NVS should use these constants.
 */

#ifndef NVS_KEYS_H
#define NVS_KEYS_H

// Device Configuration Keys
constexpr const char *NVS_DEVICE_OWNER = "device_owner";
constexpr const char *NVS_DEVICE_TIMEZONE = "device_timezone"; // Note: stored as device_timezone, not device_tz

// WiFi Configuration Keys
constexpr const char *NVS_WIFI_SSID = "wifi_ssid";
constexpr const char *NVS_WIFI_PASSWORD = "wifi_password";
constexpr const char *NVS_WIFI_TIMEOUT = "wifi_timeout";

// MQTT Configuration Keys
constexpr const char *NVS_MQTT_SERVER = "mqtt_server";
constexpr const char *NVS_MQTT_PORT = "mqtt_port";
constexpr const char *NVS_MQTT_USERNAME = "mqtt_username";
constexpr const char *NVS_MQTT_PASSWORD = "mqtt_password";

// API Configuration Keys
constexpr const char *NVS_CHATGPT_TOKEN = "chatgpt_token";

// Unbidden Ink Keys
constexpr const char *NVS_UNBIDDEN_ENABLED = "unbid_enabled";
constexpr const char *NVS_UNBIDDEN_FREQUENCY = "unbid_freq_min";
constexpr const char *NVS_UNBIDDEN_START_HOUR = "unbid_start_hr";
constexpr const char *NVS_UNBIDDEN_END_HOUR = "unbid_end_hr";
constexpr const char *NVS_UNBIDDEN_PROMPT = "unbidden_prompt";

// LED Configuration Keys (only when ENABLE_LEDS is defined)
constexpr const char *NVS_LED_PIN = "led_pin";
constexpr const char *NVS_LED_COUNT = "led_count";
constexpr const char *NVS_LED_BRIGHTNESS = "led_brightness";
constexpr const char *NVS_LED_REFRESH_RATE = "led_refresh";

// Note: Button configuration keys are dynamically generated in config_loader.cpp:
// btn1_short_act, btn1_short_mq, btn1_long_act, btn1_long_mq
// btn2_short_act, btn2_short_mq, btn2_long_act, btn2_long_mq
// btn3_short_act, btn3_short_mq, btn3_long_act, btn3_long_mq
// btn4_short_act, btn4_short_mq, btn4_long_act, btn4_long_mq
// These 16 keys are created using: String buttonPrefix = "btn" + String(i + 1) + "_";

#endif // NVS_KEYS_H
