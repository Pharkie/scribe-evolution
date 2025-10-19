#include "printer.h"
#include <utils/time_utils.h>
#include <core/logging.h>
#include <core/LogManager.h>
#include <core/config_utils.h>
#include <core/shared_types.h>
#include <core/network.h>
#include <content/content_generators.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <atomic>

// ============================================================================
// PrinterLock RAII Implementation
// ============================================================================

PrinterLock::PrinterLock(SemaphoreHandle_t m, uint32_t timeoutMs)
    : mutex(m), locked(false)
{
    if (mutex != nullptr)
    {
        // Use LogManager to avoid deadlock with logging mutex
        LogManager::instance().logf("[PrinterLock] Attempting to acquire mutex (timeout: %dms)...\n", timeoutMs);
        locked = (xSemaphoreTake(mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE);
        LogManager::instance().logf("[PrinterLock] Mutex acquire result: %s\n", locked ? "SUCCESS" : "TIMEOUT");
    }
}

PrinterLock::~PrinterLock()
{
    if (mutex != nullptr && locked)
    {
        xSemaphoreGive(mutex);
        // Use LogManager to avoid deadlock with logging mutex
        LogManager::instance().logf("[PrinterLock] Mutex released in destructor\n");
    }
}

// ============================================================================
// PrinterManager Implementation
// ============================================================================

// Global PrinterManager singleton
PrinterManager printerManager;

PrinterManager::PrinterManager()
    : uart(1), mutex(nullptr), ready(false)  // CHANGED: Initialize UART(1) directly in initializer list
{
    // Mutex will be created in initialize()
}

PrinterManager::~PrinterManager()
{
    Serial.printf("[EMERGENCY-DESTROY] PrinterManager destructor called on object at %p, mutex=%p\n", this, mutex);

    if (mutex != nullptr)
    {
        Serial.printf("[EMERGENCY-DESTROY] Deleting mutex at %p\n", mutex);
        vSemaphoreDelete(mutex);
        mutex = nullptr;
    }
}

void PrinterManager::initialize()
{
    // EMERGENCY DEBUG: Check object address, field addresses, and core
    Serial.printf("[EMERGENCY-INIT] PrinterManager at %p on CORE %d\n", this, xPortGetCoreID());
    Serial.printf("[EMERGENCY-INIT]   &uart=%p (size=%d), &mutex=%p, &ready=%p, &maxCharsPerLine=%p\n",
                  &uart, sizeof(uart), &mutex, &ready, &maxCharsPerLine);
    Serial.printf("[EMERGENCY-INIT]   mutex value BEFORE create: %p\n", mutex);

    LOG_VERBOSE("PRINTER", "Starting printer initialization...");
    ready = false; // Ensure flag is false during init

    // Create printer mutex for multi-core protection (ESP32-S3)
    if (mutex == nullptr)
    {
        mutex = xSemaphoreCreateMutex();

        // EMERGENCY DEBUG: Check if mutex creation succeeded
        Serial.printf("[EMERGENCY-INIT]   mutex value AFTER create: %p\n", mutex);
        Serial.printf("[EMERGENCY-INIT]   Verifying &mutex still points to correct location: %p\n", &mutex);

        if (mutex == nullptr)
        {
            Serial.printf("[EMERGENCY-INIT] MUTEX CREATION FAILED! Free heap: %d bytes\n", ESP.getFreeHeap());
        }

        // CRITICAL: Force memory barrier to ensure mutex pointer is visible to other core
        // ESP32 has weak memory ordering - without this, the other core may see mutex=NULL
        portMEMORY_BARRIER();
    }
    else
    {
        Serial.printf("[EMERGENCY-INIT] Mutex already exists at %p\n", mutex);
    }

    // Acquire mutex for initialization (prevents concurrent UART access during setup)
    PrinterLock lock(mutex, 10000); // 10 second timeout for init
    if (!lock.isLocked())
    {
        LOG_ERROR("PRINTER", "Failed to acquire mutex during initialization");
        return;
    }

    // Get board-specific printer configuration
    const RuntimeConfig &config = getRuntimeConfig();
    const BoardPinDefaults &boardDefaults = getBoardDefaults();

    // Ensure clean state - call end() first to clear any stale state
    uart.end();
    delay(100);

    // Initialize UART with board-specific RX/TX pins for bidirectional communication
    // RX pin receives printer status and feedback, DTR is configured separately if present
    uart.begin(9600, SERIAL_8N1, boardDefaults.printer.rx, config.printerTxPin);

    // ESP32-S3 requires extra time for UART hardware to fully initialize
    delay(500);

    // Feed watchdog after delay
    esp_task_wdt_reset();

    // Mark printer as ready - the UART is initialized
    ready.store(true, std::memory_order_release); // Force memory barrier for multi-core visibility

    LOG_VERBOSE("PRINTER", "printerReady set to TRUE, value check: %s", ready.load(std::memory_order_acquire) ? "CONFIRMED TRUE" : "ERROR: STILL FALSE!");

    LOG_VERBOSE("PRINTER", "UART initialized (TX=%d, RX=%d, DTR=%d)",
                config.printerTxPin, boardDefaults.printer.rx, boardDefaults.printer.dtr);

    LOG_VERBOSE("PRINTER", "Sending printer initialization commands...");

    // Initialize printer with ESC @ (reset command)
    uart.write(0x1B);
    uart.write('@'); // ESC @
    delay(100);

    // Set printer heating parameters from config
    uart.write(0x1B);
    uart.write('7');
    uart.write(heatingDots);     // Heating dots from config
    uart.write(heatingTime);     // Heating time from config
    uart.write(heatingInterval); // Heating interval from config
    delay(50);

    // Enable 180° rotation (which also reverses the line order)
    uart.write(0x1B);
    uart.write('{');
    uart.write(0x01); // ESC { 1
    delay(50);

    Serial.printf("[EMERGENCY-INIT] END of initialize() - mutex still valid? %p\n", mutex);

    LOG_VERBOSE("PRINTER", "Printer initialized successfully - ready = %s (address: %p)",
                ready.load() ? "TRUE" : "FALSE", (void*)&ready);
}

// ============================================================================
// Internal Methods (assume mutex is already held by caller)
// ============================================================================

void PrinterManager::setInverseInternal(bool enable)
{
    uart.write(0x1D);
    uart.write('B');
    uart.write(enable ? 1 : 0); // GS B n
}

void PrinterManager::advancePaperInternal(int lines)
{
    for (int i = 0; i < lines; i++)
    {
        uart.write(0x0A); // LF
    }
}

void PrinterManager::printWrappedInternal(const String& text)
{
    std::vector<String> lines;
    lines.reserve(20); // Reserve space to avoid frequent reallocations

    int startPos = 0;
    int textLength = text.length();

    // Process text character by character to handle newlines efficiently
    while (startPos < textLength)
    {
        // Find next newline or end of string
        int newlinePos = text.indexOf('\n', startPos);
        if (newlinePos == -1)
            newlinePos = textLength;

        // Extract current line (may be empty)
        String currentLine = text.substring(startPos, newlinePos);

        // Word wrap the current line if needed
        if (currentLine.length() == 0)
        {
            // Empty line - preserve it for spacing
            lines.push_back("");
        }
        else
        {
            // Process line with word wrapping
            int lineStart = 0;
            while (lineStart < currentLine.length())
            {
                int lineEnd = lineStart + maxCharsPerLine;

                if (lineEnd >= currentLine.length())
                {
                    // Rest of line fits
                    lines.push_back(currentLine.substring(lineStart));
                    break;
                }

                // Find word boundary to break at
                int breakPoint = currentLine.lastIndexOf(' ', lineEnd);
                if (breakPoint <= lineStart)
                {
                    // No space found - break at character limit
                    breakPoint = lineEnd;
                }

                lines.push_back(currentLine.substring(lineStart, breakPoint));

                // Skip any leading spaces on next line
                lineStart = breakPoint;
                while (lineStart < currentLine.length() && currentLine.charAt(lineStart) == ' ')
                {
                    lineStart++;
                }
            }
        }

        // Move to next line
        startPos = newlinePos + 1;
    }

    // Print lines in reverse order to compensate for 180° printer rotation
    for (int i = lines.size() - 1; i >= 0; i--)
    {
        uart.println(lines[i]);
    }
}

// ============================================================================
// Public Methods (acquire mutex via PrinterLock)
// ============================================================================

void PrinterManager::printWithHeader(const String& headerText, const String& bodyText)
{
    // CRITICAL: Force memory barrier to ensure we see latest mutex pointer from other core
    // ESP32 has weak memory ordering - this ensures we don't see stale NULL value
    portMEMORY_BARRIER();

    // EMERGENCY DEBUG: Direct Serial write to bypass LogManager
    Serial.printf("[EMERGENCY] printWithHeader() called on CORE %d, mutex=%p, ready=%d\n",
                  xPortGetCoreID(), mutex, ready.load());

    // Acquire printer mutex using RAII lock
    PrinterLock lock(mutex, 5000);

    // EMERGENCY DEBUG: Check lock result
    Serial.printf("[EMERGENCY] PrinterLock created, isLocked=%d\n", lock.isLocked());

    if (!lock.isLocked())
    {
        LOG_ERROR("PRINTER", "Failed to acquire printer mutex - print aborted");
        return;
    }

    // Clean both header and body text before printing
    String cleanHeaderText = cleanString(headerText);
    String cleanBodyText = cleanString(bodyText);

    // Feed watchdog before starting thermal printing
    esp_task_wdt_reset();

    // Print body text first (appears at bottom after rotation)
    printWrappedInternal(cleanBodyText);

    // Feed watchdog between body and header printing
    esp_task_wdt_reset();

    // Print header last (appears at top after rotation)
    setInverseInternal(true);
    printWrappedInternal(cleanHeaderText);
    setInverseInternal(false);

    advancePaperInternal(2);

    // Feed watchdog after printing completes
    esp_task_wdt_reset();

    // Mutex automatically released by PrinterLock destructor
}

void PrinterManager::printStartupMessage()
{
    // Acquire printer mutex using RAII lock
    PrinterLock lock(mutex, 5000);
    if (!lock.isLocked())
    {
        LOG_ERROR("PRINTER", "Failed to acquire printer mutex - startup message aborted");
        return;
    }

    // Feed watchdog after first log (network logging can be slow)
    esp_task_wdt_reset();

    if (isAPMode())
    {
        // In AP mode, print the proper setup message using existing content generator
        String apContent = generateAPDetailsContent();
        if (apContent.length() > 0)
        {
            // Feed watchdog before thermal printing (can be slow)
            esp_task_wdt_reset();

            LOG_VERBOSE("PRINTER", "Printing AP setup message");

            advancePaperInternal(1);

            // Feed watchdog before the actual printing
            esp_task_wdt_reset();

            String timestamp = getFormattedDateTime();

            // Clean text before printing
            String cleanTimestamp = cleanString(timestamp);
            String cleanContent = cleanString(apContent);

            // Print using internal methods (mutex already held)
            printWrappedInternal(cleanContent);
            setInverseInternal(true);
            printWrappedInternal(cleanTimestamp);
            setInverseInternal(false);
            advancePaperInternal(2);

            // Feed watchdog after thermal printing completes
            esp_task_wdt_reset();
        }
    }
    else
    {
        // In STA mode, print normal server info
        String serverInfo = "Web interface: " + String(getMdnsHostname()) + ".local or " + WiFi.localIP().toString();

        // Feed watchdog before thermal printing (can be slow)
        esp_task_wdt_reset();

        LOG_VERBOSE("PRINTER", "Printing startup message");

        advancePaperInternal(1);

        // Feed watchdog before the actual printing
        esp_task_wdt_reset();

        // Format the startup message with datetime in header and SCRIBE READY in body
        String timestamp = getFormattedDateTime();
        String startupMessage = "SCRIBE READY\n\n" + serverInfo;

        // Clean text before printing
        String cleanTimestamp = cleanString(timestamp);
        String cleanMessage = cleanString(startupMessage);

        // Print using internal methods (mutex already held)
        printWrappedInternal(cleanMessage);
        setInverseInternal(true);
        printWrappedInternal(cleanTimestamp);
        setInverseInternal(false);
        advancePaperInternal(2);

        // Feed watchdog after thermal printing completes
        esp_task_wdt_reset();
    }

    // Mutex automatically released by PrinterLock destructor
}

// ============================================================================
// Emergency diagnostic function
void emergencyCheckPrinterMutex()
{
    Serial.printf("[EMERGENCY-MAIN] printerManager mutex=%p, ready=%d\n",
                  printerManager.mutex, printerManager.ready.load());
}

// ============================================================================
// Free Function - printMessage()
// Accesses global currentMessage and calls printerManager
// ============================================================================

void printMessage()
{
    // CRITICAL: Force memory barrier to ensure we see latest printerManager state from other core
    portMEMORY_BARRIER();

    // EMERGENCY DEBUG: Check printerManager address and core
    Serial.printf("[EMERGENCY-PRINT] printMessage() called on CORE %d, printerManager at %p, mutex=%p, ready=%d\n",
                  xPortGetCoreID(), &printerManager, printerManager.mutex, printerManager.ready.load());

    LOG_VERBOSE("PRINTER", "printMessage() called - printerReady = %s",
                printerManager.isReady() ? "TRUE" : "FALSE");

    // Copy message data while holding mutex (to avoid holding mutex during slow print operation)
    // CRITICAL: Must force deep copy since Arduino String uses reference counting
    String timestamp;
    String message;

    if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        // Force deep copy by creating new String from c_str() to avoid shared buffer issues
        timestamp = String(currentMessage.timestamp.c_str());
        message = String(currentMessage.message.c_str());
        xSemaphoreGive(currentMessageMutex);
    }
    else
    {
        LOG_ERROR("PRINTER", "Failed to acquire mutex for currentMessage");
        return;
    }

    // Validate message has valid strings
    if (timestamp.c_str() == nullptr || message.c_str() == nullptr)
    {
        LOG_ERROR("PRINTER", "currentMessage has NULL string buffers - cannot print");
        return;
    }

    LOG_VERBOSE("PRINTER", "Calling printerManager.printWithHeader...");
    printerManager.printWithHeader(timestamp, message);
    LOG_VERBOSE("PRINTER", "printWithHeader() returned");
}
