/**
 * @file ManagerLock.cpp
 * @brief Implementation of RAII lock guard for thread-safe managers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "ManagerLock.h"
#include "LogManager.h"

ManagerLock::ManagerLock(SemaphoreHandle_t m, const char* name, uint32_t timeoutMs)
    : mutex(m), locked(false), managerName(name)
{
    if (mutex != nullptr)
    {
        // Attempt to acquire mutex with specified timeout
        locked = (xSemaphoreTake(mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE);

        // NOTE: Logging removed here to avoid cross-manager mutex acquisition
        // which can cause delays on ESP32-C3 (especially during LED operations).
        // Timeout errors should be logged by the calling manager if needed.
    }
}

ManagerLock::~ManagerLock()
{
    // Release mutex if we successfully acquired it
    if (mutex != nullptr && locked)
    {
        xSemaphoreGive(mutex);
    }
}
