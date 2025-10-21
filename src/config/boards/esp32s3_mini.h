/**
 * @file esp32s3_mini.h
 * @brief Pin definitions for ESP32-S3-mini board
 * @author Adam Knowles
 * @date 2025
 */

#ifndef ESP32S3_MINI_H
#define ESP32S3_MINI_H

// Board identification
#define BOARD_NAME "ESP32-S3-mini"

// Pin assignments
#define BOARD_LED_STRIP_PIN 48
#define BOARD_PRINTER_TX_PIN 43
#define BOARD_STATUS_LED_PIN 47

// Button pins (physical hardware order: JOKE=5, RIDDLE=6, QUOTE=7, QUIZ=4)
static const int BOARD_BUTTON_PINS[4] = {5, 6, 7, 4};

#endif // ESP32S3_MINI_H
