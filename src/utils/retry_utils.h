/**
 * @file retry_utils.h
 * @brief Retry and exponential backoff utilities
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file contains general-purpose retry utilities with exponential backoff
 * that can be used across the entire codebase for network operations, hardware
 * initialization, and any other operations that may benefit from retry logic.
 *
 */

#ifndef RETRY_UTILS_H
#define RETRY_UTILS_H

#include <functional>

/**
 * @brief Execute an operation with exponential backoff retry logic
 * 
 * This function attempts to execute a given operation up to maxRetries times,
 * with exponentially increasing delays between attempts (1s, 2s, 4s, etc.).
 * It handles watchdog resets during delays to prevent system resets.
 * 
 * @param operation Lambda or function that returns true on success, false on failure
 * @param maxRetries Maximum number of retry attempts (defaults to system constant)
 * @param baseDelayMs Base delay in milliseconds before first retry (defaults to system constant)
 * @return true if operation succeeded within retry limit, false otherwise
 * 
 * @example
 * bool success = retryWithBackoff([&]() {
 *     return someNetworkOperation();
 * }, 3, 1000); // 3 retries with 1s base delay
 */
bool retryWithBackoff(std::function<bool()> operation, 
                     int maxRetries = -1,    // Use system default
                     int baseDelayMs = -1);  // Use system default

#endif // RETRY_UTILS_H