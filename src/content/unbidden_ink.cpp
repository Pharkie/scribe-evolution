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
 */

#include "unbidden_ink.h"
#include <config/config.h>
#include <core/config_loader.h>
#include <utils/time_utils.h>
#include <core/logging.h>
#include <web/web_server.h>
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
}

void initializeUnbiddenInk()
{
    // Use the new manager function for consistent behavior
    startUnbiddenInk(false); // false = don't schedule immediately, use normal timing
}

bool isInWorkingHours()
{
    if (!currentSettings.enabled)
    {
        return false;
    }

    // Get current time from ezTime
    int currentHour = hour();

    // Check if current hour is within working hours
    return (currentHour >= currentSettings.startHour && currentHour < currentSettings.endHour);
}

void scheduleNextUnbiddenInk()
{
    if (!currentSettings.enabled)
    {
        return;
    }

    // Calculate frequency in milliseconds
    unsigned long frequencyMs = currentSettings.frequencyMinutes * 60 * 1000;

    // Calculate ±20% range around the target frequency
    // For 15 mins: 12-18 mins (80%-120% of target)
    // For 60 mins: 48-72 mins (80%-120% of target)
    unsigned long minTime = (frequencyMs * 80) / 100;  // 80% of frequency
    unsigned long maxTime = (frequencyMs * 120) / 100; // 120% of frequency

    // Random time within the ±20% range
    unsigned long randomOffset = random(minTime, maxTime);

    // Check if the proposed time falls within working hours
    // Get current hour and calculate proposed hour (approximate, doesn't handle day rollover perfectly)
    int currentHour = hour();
    unsigned long hoursFromNow = randomOffset / (60UL * 60UL * 1000UL);
    int proposedHour = (currentHour + hoursFromNow) % 24;

    // If proposed time is outside working hours, schedule for start of next working window
    if (proposedHour < currentSettings.startHour || proposedHour >= currentSettings.endHour)
    {
        // Calculate hours until start of working hours
        int hoursUntilStart;
        if (currentHour < currentSettings.startHour)
        {
            // Same day - wait until startHour
            hoursUntilStart = currentSettings.startHour - currentHour;
        }
        else
        {
            // Next day - wait until tomorrow's startHour
            hoursUntilStart = (24 - currentHour) + currentSettings.startHour;
        }

        // Convert to milliseconds and add small random jitter (0-60 seconds)
        randomOffset = (unsigned long)hoursUntilStart * 60UL * 60UL * 1000UL + random(0, 60000);

        LOG_VERBOSE("UNBIDDENINK", "Proposed time outside working hours - rescheduled for start of working window");
    }

    nextUnbiddenInkTime = millis() + randomOffset;

    LOG_VERBOSE("UNBIDDENINK", "Next Unbidden Ink message scheduled in %lu minutes (target: %d mins ±20%%)",
                randomOffset / (60 * 1000), currentSettings.frequencyMinutes);
}

void checkUnbiddenInk()
{
    if (!currentSettings.enabled)
    {
        return;
    }

    unsigned long currentTime = millis();

    // Check if it's time for an Unbidden Ink message (rollover-safe comparison)
    if ((long)(currentTime - nextUnbiddenInkTime) >= 0)
    {
        // Check if we're within working hours
        if (!isInWorkingHours())
        {
            LOG_NOTICE("UNBIDDENINK", "Scheduled execution skipped - outside working hours (%02d:00-%02d:00)",
                       currentSettings.startHour, currentSettings.endHour);
            LOG_NOTICE("UNBIDDENINK", "Will reschedule for next attempt");
            scheduleNextUnbiddenInk();
            return;
        }

        LOG_NOTICE("UNBIDDENINK", "Triggering Unbidden Ink message");
        LOG_VERBOSE("UNBIDDENINK", "API endpoint: %s", chatgptApiEndpoint);
        const RuntimeConfig &config = getRuntimeConfig();
        LOG_VERBOSE("UNBIDDENINK", "Token configured: %s", config.chatgptApiToken.length() > 0 ? "yes" : "no");
        LOG_VERBOSE("UNBIDDENINK", "Prompt: %s", currentSettings.prompt.c_str());

        // Feed watchdog before potentially long API call
        esp_task_wdt_reset();

        // Call generateAndQueueUnbiddenInk to generate and print content
        generateAndQueueUnbiddenInk();
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

// ========================================
// UNBIDDEN INK MANAGER FUNCTIONS
// ========================================

void startUnbiddenInk(bool immediate)
{
    // Load current settings from runtime config
    loadUnbiddenInkSettings();

    if (!currentSettings.enabled)
    {
        LOG_VERBOSE("UNBIDDENINK", "Unbidden Ink: disabled in config");
        return;
    }

    LOG_VERBOSE("UNBIDDENINK", "Starting Unbidden Ink (immediate=%s)", immediate ? "true" : "false");
    
    // Schedule the first/next print
    if (immediate)
    {
        // Schedule very soon (within 1-2 minutes) for immediate feedback
        unsigned long shortDelay = random(60000, 120000); // 1-2 minutes
        nextUnbiddenInkTime = millis() + shortDelay;
        LOG_NOTICE("UNBIDDENINK", "Unbidden Ink enabled - first message scheduled in %lu seconds", shortDelay / 1000);
    }
    else
    {
        // Schedule normally
        scheduleNextUnbiddenInk();
    }
    
    LOG_VERBOSE("UNBIDDENINK", "Unbidden Ink feature enabled - Working hours: %02d:00-%02d:00, Frequency: %d minutes",
                currentSettings.startHour, currentSettings.endHour, currentSettings.frequencyMinutes);
}

void stopUnbiddenInk()
{
    LOG_NOTICE("UNBIDDENINK", "Stopping Unbidden Ink");
    
    // Clear the schedule
    nextUnbiddenInkTime = 0;
    
    // Update settings to reflect disabled state
    currentSettings.enabled = false;
    
    LOG_VERBOSE("UNBIDDENINK", "Unbidden Ink stopped and schedule cleared");
}

void restartUnbiddenInk()
{
    LOG_NOTICE("UNBIDDENINK", "Restarting Unbidden Ink with updated settings");
    
    // Stop current scheduling
    stopUnbiddenInk();
    
    // Brief delay for clean state transition
    delay(10);
    
    // Start with new settings
    startUnbiddenInk(true); // Immediate scheduling for quick feedback
}
