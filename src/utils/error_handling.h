/**
 * @file error_handling.h
 * @brief Consistent error handling utilities for the Scribe project
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <Arduino.h>
#include "../core/logging.h"

/**
 * @brief Standard error result codes
 */
enum class ErrorCode {
    SUCCESS = 0,
    WIFI_NOT_CONNECTED = 1,
    NETWORK_ERROR = 2,
    INVALID_INPUT = 3,
    MEMORY_ERROR = 4,
    FILE_ERROR = 5,
    TIMEOUT_ERROR = 6,
    AUTHENTICATION_ERROR = 7,
    UNKNOWN_ERROR = 255
};

/**
 * @brief Convert error code to human-readable string
 */
inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS: return "Success";
        case ErrorCode::WIFI_NOT_CONNECTED: return "WiFi not connected";
        case ErrorCode::NETWORK_ERROR: return "Network error";
        case ErrorCode::INVALID_INPUT: return "Invalid input";
        case ErrorCode::MEMORY_ERROR: return "Memory error";
        case ErrorCode::FILE_ERROR: return "File error";
        case ErrorCode::TIMEOUT_ERROR: return "Timeout error";
        case ErrorCode::AUTHENTICATION_ERROR: return "Authentication error";
        case ErrorCode::UNKNOWN_ERROR: return "Unknown error";
        default: return "Unrecognized error";
    }
}

/**
 * @brief Standard error result structure
 */
template<typename T>
struct Result {
    ErrorCode error;
    T value;
    String message;
    
    Result(T val) : error(ErrorCode::SUCCESS), value(val), message("") {}
    Result(ErrorCode err, const String& msg = "") : error(err), value(T{}), message(msg) {}
    
    bool isSuccess() const { return error == ErrorCode::SUCCESS; }
    bool isError() const { return error != ErrorCode::SUCCESS; }
    
    // Log error if present
    void logIfError(const char* component) const {
        if (isError()) {
            LOG_ERROR(component, "%s: %s", errorCodeToString(error), message.c_str());
        }
    }
};

/**
 * @brief Macro for consistent error logging
 */
#define LOG_AND_RETURN_ERROR(component, code, msg) \
    do { \
        LOG_ERROR(component, "%s: %s", errorCodeToString(code), msg); \
        return Result<String>(code, msg); \
    } while(0)

/**
 * @brief Macro for consistent success logging
 */
#define LOG_AND_RETURN_SUCCESS(component, value, msg) \
    do { \
        if (strlen(msg) > 0) { \
            LOG_VERBOSE(component, "Success: %s", msg); \
        } \
        return Result<String>(value); \
    } while(0)

#endif // ERROR_HANDLING_H