/**
 * @file esp32s3_custom_pcb.h
 * @brief Pin definitions for ESP32-S3 custom PCB board
 * @author Adam Knowles
 * @date 2025
 */

#ifndef ESP32S3_CUSTOM_PCB_H
#define ESP32S3_CUSTOM_PCB_H

// Board identification
#define BOARD_NAME "ESP32-S3-custom-PCB"

// Pin assignments
#define BOARD_LED_STRIP_PIN 48
#define BOARD_PRINTER_TX_PIN 43
#define BOARD_STATUS_LED_PIN 47

// Button pins
static const int BOARD_BUTTON_PINS[4] = {4, 5, 6, 7};

#endif // ESP32S3_CUSTOM_PCB_H
