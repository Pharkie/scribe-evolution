/**
 * @file ManagerLock.h
 * @brief Shared RAII lock guard for all thread-safe singleton managers
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe Evolution ESP32 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 */

#ifndef MANAGER_LOCK_H
#define MANAGER_LOCK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief RAII lock guard for manager mutexes
 *
 * Automatically acquires mutex on construction and releases on destruction.
 * Prevents mutex leaks and ensures thread-safety on multi-core ESP32-S3.
 *
 * Usage:
 * @code
 * void MyManager::publicMethod() {
 *     ManagerLock lock(mutex, "MYMANAGER", 1000);  // 1 second timeout
 *     if (!lock.isLocked()) {
 *         LOG_ERROR("MYMANAGER", "Failed to acquire mutex!");
 *         return;
 *     }
 *
 *     // ... protected work ...
 *
 *     // Mutex automatically released when lock goes out of scope
 * }
 * @endcode
 *
 * Design Rationale:
 * - FreeRTOS SemaphoreHandle_t doesn't have native RAII (unlike C++ std::mutex)
 * - Manual xSemaphoreTake/Give is error-prone (easy to forget unlock on error paths)
 * - RAII ensures unlock even if exceptions thrown (rare on ESP32, but possible)
 * - Single shared implementation prevents code duplication across managers
 *
 * Thread Safety:
 * - Safe for ESP32-S3 dual-core (prevents Core 0 and Core 1 from concurrent access)
 * - Works on ESP32-C3 single-core (no concurrency, but pattern still valid)
 * - Non-copyable, non-movable to prevent accidental double-unlock
 */
class ManagerLock
{
private:
    SemaphoreHandle_t mutex;       ///< FreeRTOS mutex handle
    bool locked;                   ///< Whether mutex was successfully acquired
    const char* managerName;       ///< Manager name for logging context (optional)

public:
    /**
     * @brief Construct lock guard and attempt to acquire mutex
     *
     * @param m FreeRTOS mutex handle (must not be nullptr)
     * @param name Manager name for logging context (e.g., "CONFIG", "MQTT")
     * @param timeoutMs Timeout in milliseconds (default: portMAX_DELAY = blocking)
     *
     * @note If mutex is nullptr, isLocked() will return false
     * @note If timeout expires, isLocked() will return false
     * @note Always check isLocked() before proceeding with protected operations
     */
    ManagerLock(SemaphoreHandle_t m, const char* name = nullptr, uint32_t timeoutMs = portMAX_DELAY);

    /**
     * @brief Destructor - automatically releases mutex if locked
     *
     * Called when ManagerLock goes out of scope (RAII pattern).
     * Ensures mutex is always released, even on early returns or exceptions.
     */
    ~ManagerLock();

    /**
     * @brief Check if mutex was successfully acquired
     *
     * @return true if mutex is held, false if acquisition failed or timed out
     *
     * @note Always check this after constructing ManagerLock
     */
    bool isLocked() const { return locked; }

    // Delete copy constructor and assignment operator (prevent accidental copying)
    ManagerLock(const ManagerLock&) = delete;
    ManagerLock& operator=(const ManagerLock&) = delete;

    // Delete move constructor and assignment operator (RAII locks should not be moved)
    ManagerLock(ManagerLock&&) = delete;
    ManagerLock& operator=(ManagerLock&&) = delete;
};

#endif // MANAGER_LOCK_H
