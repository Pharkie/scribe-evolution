#include "time_utils.h"
#include "../core/logging.h"
#include "../core/config_utils.h"
#include "../core/network.h"
#include <esp_task_wdt.h>

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
    // Return ISO 8601 timestamp: 2025-01-01T12:00:00Z
    return myTZ.dateTime("Y-m-d\\TH:i:s\\Z");
}

// === Timezone Setup ===
void setupTimezone()
{
    // Set location (this doesn't require NTP sync)
    myTZ.setLocation(getTimezone());

    // Check if we're in AP mode - skip NTP sync if so
    if (isAPMode())
    {
        LOG_NOTICE("TIME", "In AP mode - skipping NTP sync | Timezone: %s | Using system time", getTimezone());
        return;
    }

    // Log combined timezone and NTP sync start (only in STA mode)
    LOG_VERBOSE("TIME", "Timezone set: %s | Waiting for NTP sync", getTimezone());

    // Feed watchdog while waiting for NTP sync
    int attempts = 0;
    const int maxAttempts = 60; // 60 seconds maximum wait

    while (timeStatus() != timeSet && attempts < maxAttempts)
    {
        events();             // Process ezTime events
        esp_task_wdt_reset(); // Feed watchdog
        delay(1000);
        attempts++;

        if (attempts % 10 == 0) // Log every 10 seconds
        {
            LOG_VERBOSE("TIME", "Still waiting for NTP sync... (%d/%d seconds)", attempts, maxAttempts);
        }
    }

    if (timeStatus() == timeSet)
    {
        LOG_NOTICE("TIME", "NTP sync completed successfully | Current time: %s", myTZ.dateTime().c_str());
    }
    else
    {
        LOG_WARNING("TIME", "NTP sync timeout after %d seconds - continuing with system time | Time-dependent features may not work correctly", maxAttempts);
    }
}
