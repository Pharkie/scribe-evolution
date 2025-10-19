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
 * Thread-safe for multi-core ESP32-S3 operation
 */
class PrinterManager
{
private:
    HardwareSerial& uart;
    SemaphoreHandle_t mutex;
    alignas(4) std::atomic<bool> ready;
    const int maxCharsPerLine = 32;

    // Private helper methods (must be called with mutex held)
    void setInverseInternal(bool enable);
    void advancePaperInternal(int lines);
    void printWrappedInternal(const String& text);

public:
    PrinterManager(HardwareSerial& serial);
    ~PrinterManager();

    // Initialization
    void initialize();
    bool isReady() const { return ready.load(); }

    // Thread-safe printing operations
    void printWithHeader(const String& headerText, const String& bodyText);
    void printStartupMessage();
    void printMessage();

    // Get mutex for manual locking if needed
    SemaphoreHandle_t getMutex() { return mutex; }
};

// Global printer manager instance
extern PrinterManager printerManager;

// Legacy function declarations for compatibility
void initializePrinter();
bool isPrinterReady();
void printMessage();
void printStartupMessage();
void printWithHeader(String headerText, String bodyText);

#endif // PRINTER_H
