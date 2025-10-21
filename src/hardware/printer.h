#ifndef PRINTER_H
#define PRINTER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#include <atomic>
#include <config/config.h>
#include <utils/character_mapping.h>
#include <web/web_server.h>
#include <core/ManagerLock.h>

/**
 * @brief Printer manager class - encapsulates printer hardware and synchronization
 * Thread-safe for multi-core ESP32-S3 operation using RAII locking
 *
 * Design pattern:
 * - Public methods acquire mutex using ManagerLock (RAII)
 * - Internal methods assume mutex is already held by caller
 * - This prevents double-locking while ensuring thread-safety
 */
class PrinterManager
{
private:
    SemaphoreHandle_t mutex;
    alignas(4) std::atomic<bool> ready;  // 4-byte aligned for ESP32-S3 atomic operations
    const int maxCharsPerLine = 32;
    HardwareSerial* uart;  // CHANGED: Use pointer to avoid global constructor issues

    // Private helper methods - MUST be called with mutex already held
    void setInverseInternal(bool enable);
    void advancePaperInternal(int lines);
    void printWrappedInternal(const String& text);

    // Friend declarations for debugging access
    friend void printMessage();
    friend void emergencyCheckPrinterMutex();

public:
    PrinterManager();  // CHANGED: No parameter needed, constructs own UART
    ~PrinterManager();

    // Delete copy and move to prevent accidental copying
    PrinterManager(const PrinterManager&) = delete;
    PrinterManager& operator=(const PrinterManager&) = delete;
    PrinterManager(PrinterManager&&) = delete;
    PrinterManager& operator=(PrinterManager&&) = delete;

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
