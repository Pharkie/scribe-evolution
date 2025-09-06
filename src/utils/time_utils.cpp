#include "time_utils.h"
#include "retry_utils.h"
#include <core/logging.h>
#include <core/config_utils.h>
#include <config/config.h>
#include <core/config_loader.h>
#include <core/network.h>
#include <ezTime.h>
#include <esp_task_wdt.h>

// External reference to boot time from main.cpp
extern String deviceBootTime;

// Local timezone object for proper timezone handling
Timezone localTZ;
static bool timezoneConfigured = false;

// === Time Utilities ===
String getFormattedDateTime()
{
    // Format: "Tue 22 Jul 2025 14:30"
    // Use local timezone object for proper timezone handling
    if (timezoneConfigured)
    {
        return localTZ.dateTime("D d M Y H:i");
    }
    else
    {
        // Fallback to UTC if timezone not set
        return dateTime("D d M Y H:i");
    }
}

String formatCustomDate(String customDate)
{
    // Use ezTime's makeTime for robust date parsing with minimal custom logic
    customDate.trim();

    int year = 0, month = 0, day = 0;
    bool parsed = false;

    // Try different date formats in order of preference
    // Add bounds checking for year, month, day to prevent overflow
    if (sscanf(customDate.c_str(), "%4d-%2d-%2d", &year, &month, &day) == 3)
    {
        // ISO format: YYYY-MM-DD or DD-MM-YYYY
        // Validate parsed values are in reasonable ranges
        if (year >= 1900 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31)
        {
            parsed = true;
        }
    }
    else if (sscanf(customDate.c_str(), "%2d/%2d/%4d", &day, &month, &year) == 3)
    {
        // European format: DD/MM/YYYY
        // Validate parsed values are in reasonable ranges
        if (year >= 1900 && year <= 2100 && month >= 1 && month <= 12 && day >= 1 && day <= 31)
        {
            parsed = true;
        }
    }

    if (parsed)
    {
        // Handle 2-digit years sensibly: 69 and below = 2069+, 70+ = 1970+
        if (year < 100)
        {
            year += (year <= 69) ? 2000 : 1900;
        }

        // Try the parsed format first
        time_t parsedTime = makeTime(0, 0, 0, day, month, year);
        if (parsedTime != 0) // makeTime returns 0 for invalid dates
        {
            String formatted = dateTime(parsedTime, "D d M Y H:i");
            // Parsed date successfully (from input)
            return formatted;
        }

        // If European format failed and day <= 12, try US format (MM/DD/YYYY)
        if (day <= 12 && month <= 31 && day != month)
        {
            parsedTime = makeTime(0, 0, 0, month, day, year);
            if (parsedTime != 0)
            {
                String formatted = dateTime(parsedTime, "D d M Y H:i");
                // Parsed date successfully (US format)
                return formatted;
            }
        }
    }

    // If all parsing failed, fall back to current time
    // Invalid date format, using current date
    // Supported formats: YYYY-MM-DD, DD/MM/YYYY, MM/DD/YYYY
    return getFormattedDateTime();
}

String formatRFC2822Date(const String &rfc2822Date)
{
    // Parse RFC 2822 format: "Mon, 16 Aug 2025 23:00:00 GMT"
    // Extract components for makeTime
    int commaPos = rfc2822Date.indexOf(", ");
    if (commaPos == -1)
        return "";

    String dateTimePart = rfc2822Date.substring(commaPos + 2);

    // Parse "16 Aug 2025 23:00:00 GMT"
    int spacePos1 = dateTimePart.indexOf(' ');                // After day
    int spacePos2 = dateTimePart.indexOf(' ', spacePos1 + 1); // After month
    int spacePos3 = dateTimePart.indexOf(' ', spacePos2 + 1); // After year

    if (spacePos1 <= 0 || spacePos2 <= spacePos1 || spacePos3 <= spacePos2)
        return "";

    int day = dateTimePart.substring(0, spacePos1).toInt();
    String monthStr = dateTimePart.substring(spacePos1 + 1, spacePos2);
    int year = dateTimePart.substring(spacePos2 + 1, spacePos3).toInt();
    String timeStr = dateTimePart.substring(spacePos3 + 1, spacePos3 + 9); // "HH:MM:SS"

    // Convert month name to number
    int month = 0;
    if (monthStr == "Jan")
        month = 1;
    else if (monthStr == "Feb")
        month = 2;
    else if (monthStr == "Mar")
        month = 3;
    else if (monthStr == "Apr")
        month = 4;
    else if (monthStr == "May")
        month = 5;
    else if (monthStr == "Jun")
        month = 6;
    else if (monthStr == "Jul")
        month = 7;
    else if (monthStr == "Aug")
        month = 8;
    else if (monthStr == "Sep")
        month = 9;
    else if (monthStr == "Oct")
        month = 10;
    else if (monthStr == "Nov")
        month = 11;
    else if (monthStr == "Dec")
        month = 12;

    if (month == 0)
        return "";

    // Parse time
    if (timeStr.length() < 8)
        return "";
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    int second = timeStr.substring(6, 8).toInt();

    // Create time_t using ezTime's makeTime
    time_t parsedTime = makeTime(hour, minute, second, day, month, year);
    if (parsedTime <= 0)
        return "";

    // Format to match other quick actions: "Mon 16 Aug 23:00"
    return dateTime(parsedTime, "D d M H:i");
}

String getISOTimestamp()
{
    // Return ISO 8601 timestamp in UTC: 2025-01-01T12:00:00Z
    return UTC.dateTime("Y-m-d\\TH:i:s\\Z");
}

// === Timezone Setup ===
void setupTime()
{
    LOG_VERBOSE("time_utils", "Setting up time synchronization...");

    // Configure NTP servers from config
    for (int i = 0; i < ntpServerCount; i++)
    {
        setServer(ntpServers[i]);
        LOG_VERBOSE("time_utils", "NTP server %d: %s", i + 1, ntpServers[i]);
    }

    // Set timeout for initial sync
    setInterval(ntpSyncTimeoutSeconds);

    // Wait for time sync with configurable timeout
    LOG_VERBOSE("time_utils", "Waiting for NTP sync (timeout: %d seconds)...", ntpSyncTimeoutSeconds);
    unsigned long startTime = millis();

    // Use ezTime's built-in timeout but with watchdog resets
    unsigned long lastWatchdogReset = millis();
    bool syncStarted = false;

    while (timeStatus() == timeNotSet && (millis() - startTime) < (ntpSyncTimeoutSeconds * 1000))
    {
        // Start sync process once
        if (!syncStarted)
        {
            updateNTP(); // Trigger NTP update
            syncStarted = true;
        }

        // Process ezTime events
        events();

        // Reset watchdog every 5 seconds during NTP sync wait
        if (millis() - lastWatchdogReset > 5000)
        {
            esp_task_wdt_reset();
            lastWatchdogReset = millis();
            LOG_VERBOSE("time_utils", "Still waiting for NTP sync... (%lu seconds elapsed)",
                        (millis() - startTime) / 1000);
        }
        delay(100); // Small delay to prevent tight polling
    }
    if (timeStatus() != timeSet)
    {
        LOG_WARNING("time_utils", "NTP sync failed within %d seconds", ntpSyncTimeoutSeconds);
    }
    else
    {
        unsigned long syncTime = millis() - startTime;
        LOG_VERBOSE("time_utils", "NTP sync successful in %lu ms", syncTime);
    }

    // Set ongoing sync interval
    setInterval(ntpSyncIntervalSeconds);
    
    // Configure local timezone after NTP sync
    if (timeStatus() == timeSet)
    {
        const RuntimeConfig &config = getRuntimeConfig();
        String timezoneStr = config.timezone;
        if (timezoneStr.length() == 0)
        {
            timezoneStr = defaultTimezone; // Fallback to config.h default
        }
        
        LOG_VERBOSE("time_utils", "Setting timezone to: %s", timezoneStr.c_str());
        
        // Enable caching to NVS location 0 (50 bytes storage)
        LOG_VERBOSE("time_utils", "Attempting to load timezone from cache...");
        bool cacheLoaded = localTZ.setCache(0);
        
        bool timezoneSet = false;
        if (cacheLoaded) {
            LOG_VERBOSE("time_utils", "Timezone loaded from cache");
            timezoneSet = true;
            timezoneConfigured = true;
        } else {
            LOG_VERBOSE("time_utils", "Cache miss - fetching timezone from network with retry");
            
            // Use shared retry utility with exponential backoff
            timezoneSet = retryWithBackoff([&]() -> bool {
                return localTZ.setLocation(timezoneStr);
            }, 3, 1000); // 3 retries, 1s base delay
        }
        
        if (timezoneSet)
        {
            timezoneConfigured = true;
            LOG_VERBOSE("time_utils", "Timezone successfully configured for %s", timezoneStr.c_str());
            LOG_VERBOSE("time_utils", "Current local time: %s", localTZ.dateTime().c_str());
        }
        else
        {
            timezoneConfigured = false;
            LOG_WARNING("time_utils", "Failed to set timezone %s after retries, using UTC", 
                       timezoneStr.c_str());
        }
    }
    else
    {
        timezoneConfigured = false;
        LOG_WARNING("time_utils", "Cannot set timezone - NTP sync failed");
    }
}

String getDeviceBootTime()
{
    return deviceBootTime;
}

// === Additional functions for memo placeholder expansion ===

String getMemoDate()
{
    // Format: "24Aug25" (ddMmmyy)
    // Use local timezone object for proper timezone handling
    if (timezoneConfigured)
    {
        return localTZ.dateTime("dMy");
    }
    else
    {
        // Fallback to UTC if timezone not set
        return dateTime("dMy");
    }
}

String getMemoTime()
{
    // Format: "12:30" (HH:MM)
    // Use local timezone object for proper timezone handling
    if (timezoneConfigured)
    {
        return localTZ.dateTime("H:i");
    }
    else
    {
        // Fallback to UTC if timezone not set
        return dateTime("H:i");
    }
}

String getMemoWeekday()
{
    // Format: "Sunday" (full day name)
    // Use local timezone object for proper timezone handling
    if (timezoneConfigured)
    {
        return localTZ.dateTime("l");
    }
    else
    {
        // Fallback to UTC if timezone not set
        return dateTime("l");
    }
}

String getDeviceUptime()
{
    // Format: "2h13m" (hours and minutes)
    unsigned long uptimeMs = millis();
    unsigned long hours = uptimeMs / 3600000;
    unsigned long minutes = (uptimeMs % 3600000) / 60000;
    
    return String(hours) + "h" + String(minutes) + "m";
}

// === Timezone Update Function ===
bool updateTimezone(const String &newTimezone)
{
    LOG_VERBOSE("time_utils", "Updating timezone to: %s", newTimezone.c_str());
    
    // Check if timezone actually changed
    String currentPosix = localTZ.getPosix();
    
    // Clear cache to force fresh lookup for new timezone
    // Note: ezTime doesn't have a clearCache method, but setCache(0) will reload
    LOG_VERBOSE("time_utils", "Attempting to update timezone with cache refresh");
    
    // Try to set new timezone with cache support using shared retry utility
    bool timezoneSet = retryWithBackoff([&]() -> bool {
        return localTZ.setLocation(newTimezone);
    }, 3, 1000); // 3 retries, 1s base delay
    
    if (timezoneSet) {
        timezoneConfigured = true;
        LOG_VERBOSE("time_utils", "Timezone successfully updated to %s", newTimezone.c_str());
        LOG_VERBOSE("time_utils", "Current local time: %s", localTZ.dateTime().c_str());
        return true;
    } else {
        LOG_WARNING("time_utils", "Failed to update timezone to %s after retries", 
                   newTimezone.c_str());
        return false;
    }
}
