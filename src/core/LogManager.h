#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>

/**
 * Production-ready single-writer LogManager for ESP32-S3
 * Serializes all logging through FreeRTOS queue to prevent concurrent Serial corruption
 *
 * Features:
 * - Thread-safe enqueueing from any task
 * - ISR-safe logging via logfISR()
 * - Registers as ESP-IDF vprintf handler (captures ESP_LOGx macros)
 * - Single dedicated writer task eliminates race conditions
 * - Heap-based message buffering with graceful overflow handling
 */
class LogManager
{
public:
    static LogManager& instance();

    /**
     * Initialize LogManager and start writer task
     * @param baudRate Serial baud rate (default 115200)
     * @param queueLen Maximum messages in queue (default 128)
     * @param maxLineLen Maximum line length in bytes (default 512)
     */
    void begin(uint32_t baudRate = 115200, size_t queueLen = 128, size_t maxLineLen = 512);

    /**
     * Printf-style logging from normal task context
     * Non-blocking - drops message if queue full
     */
    void logf(const char* fmt, ...);

    /**
     * Printf-style logging from ISR or high-priority task
     * Uses xQueueSendFromISR for interrupt safety
     */
    void logfISR(const char* fmt, ...);

private:
    LogManager();
    ~LogManager() = default;
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    static void writerTask(void* param);
    static int espLogVprintf(const char* fmt, va_list args);

    QueueHandle_t logQueue = nullptr;
    TaskHandle_t writerTaskHandle = nullptr;
    size_t maxLineLen = 512;
    bool initialized = false;
};

// Drop-in replacements for Serial.print/println
#define LOG_PRINT(...) LogManager::instance().logf(__VA_ARGS__)
#define LOG_PRINTLN(fmt, ...) LogManager::instance().logf(fmt "\n", ##__VA_ARGS__)
