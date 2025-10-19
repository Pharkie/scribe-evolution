#include "LogManager.h"
#include <stdarg.h>
#include <string.h>

LogManager& LogManager::instance()
{
    static LogManager instance;
    return instance;
}

LogManager::LogManager() {}

void LogManager::begin(uint32_t baudRate, size_t queueLen, size_t maxLineLen)
{
    if (initialized) return;

    this->maxLineLen = maxLineLen;

    // Initialize Serial (Arduino) or ensure stdout flush (ESP-IDF)
    Serial.begin(baudRate);
    while (!Serial && millis() < 5000) {
        delay(10);
    }

    // Create queue for log messages (stores char* pointers to heap strings)
    logQueue = xQueueCreate(queueLen, sizeof(char*));
    if (!logQueue) {
        Serial.println("FATAL: LogManager queue creation failed");
        return;
    }

    // Create writer task on Core 0 (isolated from AsyncWebServer on Core 1)
    xTaskCreatePinnedToCore(
        writerTask,
        "LogWriter",
        4096,                    // Stack size
        this,                    // Parameters
        configMAX_PRIORITIES - 2, // High priority but below ISR handlers
        &writerTaskHandle,
        0                        // Core 0 (AsyncWebServer typically runs on Core 1)
    );

    if (!writerTaskHandle) {
        Serial.println("FATAL: LogManager task creation failed");
        vQueueDelete(logQueue);
        logQueue = nullptr;
        return;
    }

    // Register as ESP-IDF vprintf handler (captures ESP_LOGx macros)
    esp_log_set_vprintf(espLogVprintf);

    initialized = true;
    logf("[LogManager] Initialized - Queue: %d, MaxLine: %d\n", queueLen, maxLineLen);
}

void LogManager::logf(const char* fmt, ...)
{
    if (!logQueue) return;

    // Allocate buffer on heap
    char* line = (char*)malloc(maxLineLen);
    if (!line) {
        // Allocation failed - drop message silently
        return;
    }

    // Format message
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(line, maxLineLen, fmt, args);
    va_end(args);

    // Truncation handling
    if (len >= (int)maxLineLen) {
        line[maxLineLen - 4] = '.';
        line[maxLineLen - 3] = '.';
        line[maxLineLen - 2] = '.';
        line[maxLineLen - 1] = '\0';
    }

    // Enqueue message (non-blocking, drop if queue full)
    if (xQueueSend(logQueue, &line, 0) != pdTRUE) {
        // Queue full - free buffer and drop message
        free(line);
    }
}

void LogManager::logfISR(const char* fmt, ...)
{
    if (!logQueue) return;

    // Allocate buffer on heap (pvPortMalloc is ISR-safe for FreeRTOS heap)
    char* line = (char*)pvPortMalloc(maxLineLen);
    if (!line) return;

    // Format message
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(line, maxLineLen, fmt, args);
    va_end(args);

    // Truncation handling
    if (len >= (int)maxLineLen) {
        line[maxLineLen - 4] = '.';
        line[maxLineLen - 3] = '.';
        line[maxLineLen - 2] = '.';
        line[maxLineLen - 1] = '\0';
    }

    // Enqueue message (ISR-safe version)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(logQueue, &line, &xHigherPriorityTaskWoken) != pdTRUE) {
        vPortFree(line);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void LogManager::writerTask(void* param)
{
    LogManager* mgr = (LogManager*)param;
    char* line = nullptr;

    while (true) {
        // Block waiting for messages
        if (xQueueReceive(mgr->logQueue, &line, portMAX_DELAY) == pdTRUE) {
            if (line) {
                // Write to Serial (single writer - no race conditions)
                Serial.print(line);

                // Free heap buffer
                free(line);
                line = nullptr;
            }
        }
    }
}

int LogManager::espLogVprintf(const char* fmt, va_list args)
{
    LogManager& mgr = LogManager::instance();
    if (!mgr.logQueue) {
        // Not initialized yet - fallback to direct Serial
        return vprintf(fmt, args);
    }

    // Allocate buffer on heap
    char* line = (char*)malloc(mgr.maxLineLen);
    if (!line) return 0;

    // Format message
    int len = vsnprintf(line, mgr.maxLineLen, fmt, args);

    // Truncation handling
    if (len >= (int)mgr.maxLineLen) {
        line[mgr.maxLineLen - 4] = '.';
        line[mgr.maxLineLen - 3] = '.';
        line[mgr.maxLineLen - 2] = '.';
        line[mgr.maxLineLen - 1] = '\0';
    }

    // Enqueue message
    if (xQueueSend(mgr.logQueue, &line, 0) != pdTRUE) {
        free(line);
        return 0;
    }

    return len;
}
