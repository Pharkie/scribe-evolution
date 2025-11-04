/**
 * @file heap_utils.cpp
 * @brief Implementation of heap memory utilities
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#include "heap_utils.h"
#include <core/logging.h>
#include <esp_heap_caps.h>

bool checkContiguousHeap(size_t requiredContiguous,
                        size_t safetyMargin,
                        const char* componentName,
                        const char* operationName) {
    // Calculate total required heap
    const size_t totalRequired = requiredContiguous + safetyMargin;

    // Get heap statistics
    size_t freeHeap = ESP.getFreeHeap();
    size_t maxContiguousBlock = ESP.getMaxAllocHeap();

    // Check 1: Do we have a large enough contiguous block?
    // This is the CRITICAL check - allocation will fail even if total heap is sufficient
    if (maxContiguousBlock < requiredContiguous) {
        LOG_ERROR(componentName, "Heap too fragmented for %s!", operationName);
        LOG_ERROR(componentName, "Total free: %d bytes, largest block: %d bytes, need: %d bytes",
                  freeHeap, maxContiguousBlock, requiredContiguous);
        return false;
    }

    // Check 2: Do we have enough total free heap?
    // This ensures system stability for related operations
    if (freeHeap < totalRequired) {
        LOG_ERROR(componentName, "Insufficient total heap for %s: %d bytes free, need %d bytes",
                  operationName, freeHeap, totalRequired);
        return false;
    }

    // Success - log verbose info for diagnostics
    LOG_VERBOSE(componentName, "Heap check passed for %s: %d bytes free, %d bytes largest block (need %d bytes)",
                operationName, freeHeap, maxContiguousBlock, requiredContiguous);

    return true;
}
