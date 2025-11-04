/**
 * @file heap_utils.h
 * @brief Heap memory utilities for contiguous allocation checks
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file contains utilities for checking heap memory availability,
 * specifically for operations requiring large contiguous memory blocks
 * (FastLED RMT buffers, SSL/TLS handshakes, DMA buffers, etc.).
 *
 * CRITICAL: Many ESP32 operations require CONTIGUOUS heap memory, not just
 * total free heap. ESP.getMaxAllocHeap() returns the largest contiguous block,
 * which is the critical metric for preventing allocation failures.
 */

#ifndef HEAP_UTILS_H
#define HEAP_UTILS_H

#include <Arduino.h>

/**
 * @brief Check if sufficient contiguous heap memory is available
 *
 * This function checks both:
 * 1. Largest contiguous block (critical for allocation success)
 * 2. Total free heap (ensures overall system health)
 *
 * Use this before any operation requiring >4KB contiguous memory:
 * - FastLED RMT buffers (96 bytes per LED)
 * - SSL/TLS handshakes (16-32KB for mbedTLS)
 * - DMA buffers
 * - Large arrays/structs
 * - Image/file buffers
 *
 * @param requiredContiguous Minimum contiguous bytes needed for allocation
 * @param safetyMargin Additional bytes for related operations (buffers, etc.)
 * @param componentName Name of component for logging (e.g., "LEDS", "API")
 * @param operationName Description of operation for logging (e.g., "FastLED init", "SSL connection")
 * @return true if sufficient heap available, false otherwise (logs errors)
 *
 * @example FastLED initialization:
 * if (!checkContiguousHeap(ledCount * 96, 10240, "LEDS", "FastLED init")) {
 *     return false; // Insufficient heap
 * }
 *
 * @example SSL/TLS connection:
 * if (!checkContiguousHeap(32768, 8192, "API", "SSL connection")) {
 *     return false; // Insufficient heap
 * }
 */
bool checkContiguousHeap(size_t requiredContiguous,
                        size_t safetyMargin,
                        const char* componentName,
                        const char* operationName);

#endif // HEAP_UTILS_H
