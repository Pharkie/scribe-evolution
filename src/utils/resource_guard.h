/**
 * @file resource_guard.h
 * @brief RAII-style resource guards for automatic cleanup
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 */

#ifndef RESOURCE_GUARD_H
#define RESOURCE_GUARD_H

#include <Arduino.h>
#include <LittleFS.h>
#include <HTTPClient.h>
#include <core/logging.h>

/**
 * @brief RAII file guard that automatically closes files
 */
class FileGuard {
private:
    File file;
    bool valid;
    
public:
    FileGuard(const String& path, const char* mode = "r") : valid(false) {
        file = LittleFS.open(path, mode);
        valid = file;
        if (!valid) {
            LOG_WARNING("RESOURCE", "Failed to open file: %s", path.c_str());
        }
    }
    
    ~FileGuard() {
        if (valid) {
            file.close();
            LOG_VERBOSE("RESOURCE", "File closed automatically by guard");
        }
    }
    
    // No copy constructor or assignment
    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;
    
    // Allow move semantics
    FileGuard(FileGuard&& other) noexcept : file(other.file), valid(other.valid) {
        other.valid = false;
    }
    
    FileGuard& operator=(FileGuard&& other) noexcept {
        if (this != &other) {
            if (valid) file.close();
            file = other.file;
            valid = other.valid;
            other.valid = false;
        }
        return *this;
    }
    
    // Access operators
    File& operator*() { return file; }
    File* operator->() { return &file; }
    operator bool() const { return valid; }
    
    File& get() { return file; }
    bool isValid() const { return valid; }
};

/**
 * @brief RAII HTTP client guard that automatically calls end()
 */
class HTTPGuard {
private:
    HTTPClient* http;
    bool valid;
    
public:
    HTTPGuard(HTTPClient* client) : http(client), valid(false) {
        if (client) {
            valid = true;
            LOG_VERBOSE("RESOURCE", "HTTP client guard created");
        }
    }
    
    ~HTTPGuard() {
        if (valid && http) {
            http->end();
            LOG_VERBOSE("RESOURCE", "HTTP client closed automatically by guard");
        }
    }
    
    // No copy constructor or assignment
    HTTPGuard(const HTTPGuard&) = delete;
    HTTPGuard& operator=(const HTTPGuard&) = delete;
    
    // Allow move semantics
    HTTPGuard(HTTPGuard&& other) noexcept : http(other.http), valid(other.valid) {
        other.valid = false;
        other.http = nullptr;
    }
    
    HTTPGuard& operator=(HTTPGuard&& other) noexcept {
        if (this != &other) {
            if (valid && http) http->end();
            http = other.http;
            valid = other.valid;
            other.valid = false;
            other.http = nullptr;
        }
        return *this;
    }
    
    // Access operators
    HTTPClient& operator*() { return *http; }
    HTTPClient* operator->() { return http; }
    operator bool() const { return valid && http; }
    
    HTTPClient* get() { return http; }
    bool isValid() const { return valid && http; }
    
    void release() { valid = false; }  // Release ownership without cleanup
};

/**
 * @brief Generic resource guard with custom cleanup function
 */
template<typename T>
class ResourceGuard {
private:
    T resource;
    std::function<void(T&)> cleanup;
    bool valid;
    
public:
    template<typename CleanupFunc>
    ResourceGuard(T res, CleanupFunc&& cleanupFunc) 
        : resource(res), cleanup(std::forward<CleanupFunc>(cleanupFunc)), valid(true) {
        LOG_VERBOSE("RESOURCE", "Generic resource guard created");
    }
    
    ~ResourceGuard() {
        if (valid) {
            cleanup(resource);
            LOG_VERBOSE("RESOURCE", "Generic resource cleaned up by guard");
        }
    }
    
    // No copy constructor or assignment
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;
    
    // Allow move semantics
    ResourceGuard(ResourceGuard&& other) noexcept 
        : resource(std::move(other.resource)), cleanup(std::move(other.cleanup)), valid(other.valid) {
        other.valid = false;
    }
    
    ResourceGuard& operator=(ResourceGuard&& other) noexcept {
        if (this != &other) {
            if (valid) cleanup(resource);
            resource = std::move(other.resource);
            cleanup = std::move(other.cleanup);
            valid = other.valid;
            other.valid = false;
        }
        return *this;
    }
    
    // Access operators
    T& operator*() { return resource; }
    T* operator->() { return &resource; }
    
    T& get() { return resource; }
    void release() { valid = false; }  // Release ownership without cleanup
};

#endif // RESOURCE_GUARD_H