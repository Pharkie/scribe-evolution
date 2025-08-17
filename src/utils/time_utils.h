#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>
#include <ezTime.h>
#include "../core/config.h"

// External timezone object
extern Timezone myTZ;

// Function declarations
String getFormattedDateTime();
String formatCustomDate(String customDate);
String formatRFC2822Date(const String &rfc2822Date);
String getISOTimestamp();
String getDeviceBootTime();
void setupTime();

#endif // TIME_UTILS_H
