#include "time_utils.h"
#include "../core/logging.h"
#include "../core/config_utils.h"
#include "../core/config.h"
#include "../core/network.h"
#include <ezTime.h>
#include <esp_task_wdt.h>

// External reference to boot time from main.cpp
extern String deviceBootTime;

// Timezone object
Timezone myTZ;

// === Time Utilities ===
String getFormattedDateTime()
{
    // Use ezTime for automatic timezone handling
    // Format: "Tue 22 Jul 2025 14:30"
    String dateTime = myTZ.dateTime("D d M Y H:i");
    return dateTime;
}

String formatCustomDate(String customDate)
{
    // Use ezTime's makeTime for robust date parsing with minimal custom logic
    customDate.trim();

    int year = 0, month = 0, day = 0;
    bool parsed = false;

    // Try different date formats in order of preference
    if (sscanf(customDate.c_str(), "%d-%d-%d", &year, &month, &day) == 3)
    {
        // ISO format: YYYY-MM-DD or DD-MM-YYYY
        parsed = true;
    }
    else if (sscanf(customDate.c_str(), "%d/%d/%d", &day, &month, &year) == 3)
    {
        // European format: DD/MM/YYYY
        parsed = true;
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
            String formatted = myTZ.dateTime(parsedTime, "D d M Y H:i");
            // Parsed date successfully (from input)
            return formatted;
        }

        // If European format failed and day <= 12, try US format (MM/DD/YYYY)
        if (day <= 12 && month <= 31 && day != month)
        {
            parsedTime = makeTime(0, 0, 0, month, day, year);
            if (parsedTime != 0)
            {
                String formatted = myTZ.dateTime(parsedTime, "D d M Y H:i");
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
    return myTZ.dateTime(parsedTime, "D d M H:i");
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
}

String getDeviceBootTime()
{
    return deviceBootTime;
}
