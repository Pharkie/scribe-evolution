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
String getISOTimestamp();
String getDeviceBootTime();
void setupTimezone();

#endif // TIME_UTILS_H
