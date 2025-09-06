/**
 * @file unbidden_ink.h
 * @brief Unbidden Ink feature for automated AI-generated content
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 */

#ifndef UNBIDDEN_INK_H
#define UNBIDDEN_INK_H

#include <Arduino.h>

// Default prompts are now defined in config.h

// Settings structure for diagnostics and external access
struct UnbiddenInkSettings
{
    bool enabled;
    String prompt;
    int startHour;
    int endHour;
    int frequencyMinutes;
};

/**
 * @brief Unbidden Ink feature for automated AI-generated content
 *
 * This module handles the automated printing of AI-generated content from
 * ChatGPT via Pipedream API during configured working hours. Users can
 * customize the prompt sent to ChatGPT or choose from preset options.
 *
 * Features:
 * - Working hours scheduling (configurable start/end times)
 * - Random timing within frequency windows for natural feel
 * - Customizable ChatGPT prompts with preset options
 * - Pipedream API integration with Bearer token authentication
 * - Automatic fallback to default messages
 * - Integration with existing printing and logging systems
 */

/**
 * @brief Initialize the Unbidden Ink system and schedule first message
 * Should be called during system setup after WiFi and time sync
 */
void initializeUnbiddenInk();

/**
 * @brief Check if Unbidden Ink message should be sent
 * Should be called regularly from main loop when WiFi is connected
 */
void checkUnbiddenInk();

/**
 * @brief Check if current time is within configured working hours
 * @return true if within working hours and Unbidden Ink is enabled
 */
bool isInWorkingHours();

/**
 * @brief Schedule the next Unbidden Ink message at random time within frequency window
 * Called automatically after each message is sent
 */
void scheduleNextUnbiddenInk();

/**
 * @brief Get current ChatGPT prompt from settings
 * @return Current prompt string
 */
String getUnbiddenInkPrompt();

/**
 * @brief Load Unbidden Ink settings from file
 */
void loadUnbiddenInkSettings();

/**
 * @brief Get current Unbidden Ink settings for status display
 * @return Current settings structure
 */
UnbiddenInkSettings getCurrentUnbiddenInkSettings();

/**
 * @brief Get next scheduled Unbidden Ink message time
 * @return Next message time in milliseconds since boot
 */
unsigned long getNextUnbiddenInkTime();

#endif // UNBIDDEN_INK_H
