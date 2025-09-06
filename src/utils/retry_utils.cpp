#include "retry_utils.h"
#include <core/logging.h>
#include <config/system_constants.h>
#include <esp_task_wdt.h>
#include <Arduino.h>

bool retryWithBackoff(std::function<bool()> operation, int maxRetries, int baseDelayMs)
{
    // Use system defaults if not specified
    if (maxRetries == -1) maxRetries = defaultMaxRetries;
    if (baseDelayMs == -1) baseDelayMs = defaultBaseDelayMs;
    
    int delayMs = baseDelayMs;
    for (int attempt = 0; attempt < maxRetries; attempt++)
    {
        if (operation())
        {
            if (attempt > 0)
            {
                LOG_NOTICE("RETRY", "Operation succeeded after %d retries", attempt);
            }
            return true;
        }
        
        if (attempt < maxRetries - 1) // Don't delay after the last attempt
        {
            LOG_VERBOSE("RETRY", "Retry attempt %d failed, waiting %dms", attempt + 1, delayMs);
            esp_task_wdt_reset(); // Keep watchdog happy during delays
            delay(delayMs);
            delayMs *= 2; // Exponential backoff
        }
    }
    
    LOG_WARNING("RETRY", "Operation failed after %d retries", maxRetries);
    return false;
}