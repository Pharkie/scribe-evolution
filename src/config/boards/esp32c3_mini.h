/**
 * @file esp32c3_mini.h
 * @brief Pin definitions for ESP32-C3-mini board
 * @author Adam Knowles
 * @date 2025
 */

#ifndef ESP32C3_MINI_H
#define ESP32C3_MINI_H

// Board identification
#define BOARD_NAME "ESP32-C3-mini"

// Pin assignments
#define BOARD_LED_STRIP_PIN 20
#define BOARD_PRINTER_TX_PIN 21
#define BOARD_STATUS_LED_PIN 8

// Button pins (button 0=GPIO4, button 1=GPIO5, button 2=GPIO6, button 3=GPIO7)
static const int BOARD_BUTTON_PINS[4] = {4, 5, 6, 7};

#endif // ESP32C3_MINI_H
