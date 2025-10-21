#include "logging.h"
#include "LogManager.h"
#include <utils/time_utils.h>
#include "config_utils.h"

// Safe device owner accessor for logging - avoids recursive calls during initialization
static bool firstCall = true;
const char *getSafeDeviceOwner()
{
    if (firstCall)
    {
        // On first call during initialization, just use the default to avoid recursion
        firstCall = false;
        return defaultDeviceOwner;
    }
    // After first call, safe to use the dynamic version
    return getDeviceOwnerKey();
}

String getLogLevelString(int level)
{
    switch (level)
    {
    case LOG_LEVEL_ERROR:
        return "ERROR";
    case LOG_LEVEL_WARNING:
        return "WARNING";
    case LOG_LEVEL_NOTICE:
        return "NOTICE";
    case LOG_LEVEL_VERBOSE:
        return "VERBOSE";
    default:
        return "UNKNOWN";
    }
}
