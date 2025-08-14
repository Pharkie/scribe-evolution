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
    waitForSync(ntpSyncTimeoutSeconds);

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
