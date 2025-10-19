#ifndef PRINTER_H
#define PRINTER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#include <atomic>
#include <config/config.h>
#include <utils/character_mapping.h>
#include <web/web_server.h>

/**
 * @brief RAII lock guard for printer mutex
 * Automatically releases mutex when it goes out of scope
 * Prevents mutex leaks and ensures thread-safety on multi-core ESP32-S3
 */
class PrinterLock
{
private:
    SemaphoreHandle_t mutex;
    bool locked;

public:
    PrinterLock(SemaphoreHandle_t m, uint32_t timeoutMs = 5000);
    ~PrinterLock();
    bool isLocked() const { return locked; }

    // Prevent copying
    PrinterLock(const PrinterLock&) = delete;
    PrinterLock& operator=(const PrinterLock&) = delete;
};

/**
 * @brief Printer manager class - encapsulates printer hardware and synchronization
 * Thread-safe for multi-core ESP32-S3 operation using RAII locking
 *
 * Design pattern:
 * - Public methods acquire mutex using PrinterLock (RAII)
 * - Internal methods assume mutex is already held by caller
 * - This prevents double-locking while ensuring thread-safety
 */
class PrinterManager
{
private:
    HardwareSerial& uart;
    SemaphoreHandle_t mutex;
    alignas(4) std::atomic<bool> ready;  // 4-byte aligned for ESP32-S3 atomic operations
    const int maxCharsPerLine = 32;

    // Private helper methods - MUST be called with mutex already held
    void setInverseInternal(bool enable);
    void advancePaperInternal(int lines);
    void printWrappedInternal(const String& text);

public:
    PrinterManager(HardwareSerial& serial);
    ~PrinterManager();

    // Initialization
    void initialize();
    bool isReady() const { return ready.load(std::memory_order_acquire); }

    // Thread-safe printing operations - all acquire mutex internally
    void printWithHeader(const String& headerText, const String& bodyText);
    void printStartupMessage();
};

// Global printer manager instance
extern PrinterManager printerManager;

// Free function for printing messages from global currentMessage queue
// This accesses external globals (currentMessage, currentMessageMutex) and calls printerManager
void printMessage();

#endif // PRINTER_H
