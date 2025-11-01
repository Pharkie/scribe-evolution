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
constexpr const char *NVS_BOARD_TYPE = "board_type";           // Board identifier (e.g., "C3_MINI", "S3_MINI", "S3_CUSTOM_PCB")

// Hardware GPIO Configuration Keys
constexpr const char *NVS_PRINTER_TX_PIN = "printer_tx_pin";
constexpr const char *NVS_PRINTER_RX_PIN = "printer_rx_pin";
constexpr const char *NVS_PRINTER_DTR_PIN = "printer_dtr_pin";
constexpr const char *NVS_BUTTON1_GPIO = "btn1_gpio";
constexpr const char *NVS_BUTTON2_GPIO = "btn2_gpio";
constexpr const char *NVS_BUTTON3_GPIO = "btn3_gpio";
constexpr const char *NVS_BUTTON4_GPIO = "btn4_gpio";

// WiFi Configuration Keys
constexpr const char *NVS_WIFI_SSID = "wifi_ssid";
constexpr const char *NVS_WIFI_PASSWORD = "wifi_password";
// Note: wifi_timeout is NOT in NVS - it's a compile-time constant in system_constants.h

// MQTT Configuration Keys
constexpr const char *NVS_MQTT_ENABLED = "mqtt_enabled";
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

// Memo Configuration Keys
constexpr const char *NVS_MEMO_1 = "memo_1";
constexpr const char *NVS_MEMO_2 = "memo_2"; 
constexpr const char *NVS_MEMO_3 = "memo_3";
constexpr const char *NVS_MEMO_4 = "memo_4";

// Note: Button configuration keys are dynamically generated in config_loader.cpp:
// btn1_short_act, btn1_short_mq, btn1_long_act, btn1_long_mq, btn1_short_led, btn1_long_led
// btn2_short_act, btn2_short_mq, btn2_long_act, btn2_long_mq, btn2_short_led, btn2_long_led
// btn3_short_act, btn3_short_mq, btn3_long_act, btn3_long_mq, btn3_short_led, btn3_long_led
// btn4_short_act, btn4_short_mq, btn4_long_act, btn4_long_mq, btn4_short_led, btn4_long_led
// These 24 keys are created using: String buttonPrefix = "btn" + String(i + 1) + "_";

#endif // NVS_KEYS_H
