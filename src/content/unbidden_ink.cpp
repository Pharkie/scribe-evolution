/**
 * @file unbidden_ink.cpp
 * @brief Implementation of Unbidden Ink feature for automated AI-generated content
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
 * Based on the original Project Scribe by UrbanCircles.
 */

#include "unbidden_ink.h"
#include "../core/config.h"
#include "../core/config_loader.h"
#include "../utils/time_utils.h"
#include "../core/logging.h"
#include "../web/web_server.h"
#include "content_handlers.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

// Unbidden Ink timing variables
static unsigned long nextUnbiddenInkTime = 0;

// Dynamic settings instance
static UnbiddenInkSettings currentSettings;

// Load settings from the runtime configuration system
void loadUnbiddenInkSettings()
{
    const RuntimeConfig &config = getRuntimeConfig();

    // Use configuration from the main config system
    currentSettings.enabled = config.unbiddenInkEnabled;
    currentSettings.prompt = config.unbiddenInkPrompt.c_str();
    currentSettings.startHour = config.unbiddenInkStartHour;
    currentSettings.endHour = config.unbiddenInkEndHour;
    currentSettings.frequencyMinutes = config.unbiddenInkFrequencyMinutes;

    LOG_VERBOSE("UNBIDDENINK", "Loaded settings from runtime configuration");
}

void initializeUnbiddenInk()
{
    // Load dynamic settings first
    loadUnbiddenInkSettings();

    if (!currentSettings.enabled)
    {
        LOG_NOTICE("UNBIDDENINK", "Unbidden Ink feature is disabled");
        return;
    }

    scheduleNextUnbiddenInk();
    LOG_NOTICE("UNBIDDENINK", "Unbidden Ink feature enabled - Working hours: %02d:00-%02d:00, Frequency: %d minutes",
               currentSettings.startHour, currentSettings.endHour, currentSettings.frequencyMinutes);
}

bool isInWorkingHours()
{
    if (!currentSettings.enabled)
    {
        return false;
    }

    // Get current time from ezTime
    int currentHour = myTZ.hour();

    // Check if current hour is within working hours
    return (currentHour >= currentSettings.startHour && currentHour < currentSettings.endHour);
}

void scheduleNextUnbiddenInk()
{
    if (!currentSettings.enabled)
    {
        return;
    }

    // Calculate random time within the next frequency window
    unsigned long frequencyMs = currentSettings.frequencyMinutes * 60 * 1000; // Convert to milliseconds
    unsigned long randomOffset = random(0, frequencyMs);                      // Random time within the window
    nextUnbiddenInkTime = millis() + randomOffset;

    LOG_VERBOSE("UNBIDDENINK", "Next Unbidden Ink message scheduled in %lu minutes", randomOffset / (60 * 1000));
}

void checkUnbiddenInk()
{
    if (!currentSettings.enabled)
    {
        return;
    }

    unsigned long currentTime = millis();

    // Check if it's time for an Unbidden Ink message
    if (currentTime >= nextUnbiddenInkTime && isInWorkingHours())
    {
        LOG_NOTICE("UNBIDDENINK", "Triggering Unbidden Ink message");
        LOG_VERBOSE("UNBIDDENINK", "API endpoint: %s", chatgptApiEndpoint);
        const RuntimeConfig &config = getRuntimeConfig();
        LOG_VERBOSE("UNBIDDENINK", "Token starts with: %.10s...", config.chatgptApiToken.c_str());
        LOG_VERBOSE("UNBIDDENINK", "Prompt: %s", currentSettings.prompt.c_str());

        // Feed watchdog before potentially long API call
        esp_task_wdt_reset();

        // Call handleUnbiddenInk directly to generate and print content
        handleUnbiddenInk();
        LOG_NOTICE("UNBIDDENINK", "Unbidden Ink content requested");

        // Feed watchdog after API call
        esp_task_wdt_reset();

        // Schedule the next Unbidden Ink message
        scheduleNextUnbiddenInk();
    }
}

String getUnbiddenInkPrompt()
{
    return currentSettings.prompt;
}

// Get current settings for status display
UnbiddenInkSettings getCurrentUnbiddenInkSettings()
{
    return currentSettings;
}

unsigned long getNextUnbiddenInkTime()
{
    return nextUnbiddenInkTime;
}
