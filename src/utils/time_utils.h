#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>
#include <ezTime.h>
#include "../core/config.h"


// Function declarations
String getFormattedDateTime();
String formatCustomDate(String customDate);
String formatRFC2822Date(const String &rfc2822Date);
String getISOTimestamp();
String getDeviceBootTime();
void setupTime();

// Additional functions for memo placeholder expansion
String getMemoDate();          // Format: "24Aug25"
String getMemoTime();          // Format: "12:30"
String getMemoWeekday();       // Format: "Sunday"
String getDeviceUptime();      // Format: "2h13m"

#endif // TIME_UTILS_H
