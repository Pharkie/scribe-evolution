/**
 * @file gpio_map.h
 * @brief ESP32-C3 GPIO pin mapping and validation functions
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file contains hardware-specific GPIO definitions and validation functions
 * for the ESP32-C3 microcontroller. These are compile-time constants based on
 * the ESP32-C3 datasheet and hardware constraints.
 */

#ifndef GPIO_MAP_H
#define GPIO_MAP_H

// ESP32-C3 GPIO pin characteristics and availability
enum GPIOType
{
    GPIO_TYPE_AVOID = 0,
    GPIO_TYPE_SAFE = 1
};

struct GPIOInfo
{
    int pin;
    GPIOType type;
    const char *description;
};

// ESP32-C3 GPIO configuration (compile-time, not user-configurable)
// Based on ESP32-C3 datasheet and hardware constraints
static const GPIOInfo ESP32C3_GPIO_MAP[] = {
    {-1, GPIO_TYPE_SAFE, "Not connected"},
    {0, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {1, GPIO_TYPE_AVOID, "Avoid: TX for UART0 (USB-Serial)"},
    {2, GPIO_TYPE_SAFE, "Safe"},
    {3, GPIO_TYPE_AVOID, "Avoid: RX for UART0 (USB-Serial)"},
    {4, GPIO_TYPE_SAFE, "Safe"},
    {5, GPIO_TYPE_SAFE, "Safe"},
    {6, GPIO_TYPE_SAFE, "Safe"},
    {7, GPIO_TYPE_SAFE, "Safe"},
    {8, GPIO_TYPE_AVOID, "Avoid: Onboard LED"},
    {9, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {10, GPIO_TYPE_SAFE, "Safe"},
    {20, GPIO_TYPE_SAFE, "Safe (UART1 TX)"},
    {21, GPIO_TYPE_SAFE, "Safe (UART1 RX)"}
};

static const int ESP32C3_GPIO_COUNT = sizeof(ESP32C3_GPIO_MAP) / sizeof(ESP32C3_GPIO_MAP[0]);

// Helper functions for GPIO validation
static bool isValidGPIO(int pin)
{
    for (int i = 0; i < ESP32C3_GPIO_COUNT; i++)
    {
        if (ESP32C3_GPIO_MAP[i].pin == pin)
        {
            return true;
        }
    }
    return false;
}

static bool isSafeGPIO(int pin)
{
    for (int i = 0; i < ESP32C3_GPIO_COUNT; i++)
    {
        if (ESP32C3_GPIO_MAP[i].pin == pin)
        {
            return ESP32C3_GPIO_MAP[i].type == GPIO_TYPE_SAFE;
        }
    }
    return false;
}

static const char *getGPIODescription(int pin)
{
    for (int i = 0; i < ESP32C3_GPIO_COUNT; i++)
    {
        if (ESP32C3_GPIO_MAP[i].pin == pin)
        {
            return ESP32C3_GPIO_MAP[i].description;
        }
    }
    return "Unknown GPIO";
}

#endif // GPIO_MAP_H