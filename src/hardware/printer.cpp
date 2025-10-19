#include "printer.h"
#include <utils/time_utils.h>
#include <core/logging.h>
#include <core/config_utils.h>
#include <core/shared_types.h>
#include <core/network.h>
#include <content/content_generators.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <atomic>

// Printer object and configuration
// Use UART1 on all ESP32 variants
HardwareSerial printer(1);
const int maxCharsPerLine = 32;

// Use atomic bool for printerReady to ensure thread-safe access without mutex overhead
// CRITICAL: Must be aligned to 4-byte boundary on ESP32-S3 for atomic operations
alignas(4) std::atomic<bool> printerReady(false);

// Mutex to protect printer hardware access on multi-core ESP32-S3
// Prevents race conditions when AsyncWebServer handlers on Core 0 and main loop on Core 1
// both try to access the printer UART simultaneously
SemaphoreHandle_t printerMutex = nullptr;

// === Printer Functions ===

// Check if printer is ready for writes
bool isPrinterReady()
{
    return printerReady;
}

void initializePrinter()
{
    LOG_VERBOSE("PRINTER", "Starting printer initialization...");
    printerReady = false; // Ensure flag is false during init

    // Create printer mutex for multi-core protection (ESP32-S3)
    if (printerMutex == nullptr)
    {
        printerMutex = xSemaphoreCreateMutex();
    }

    // Get board-specific printer configuration
    const RuntimeConfig &config = getRuntimeConfig();
    const BoardPinDefaults &boardDefaults = getBoardDefaults();

    // Ensure clean state - call end() first to clear any stale state
    printer.end();
    delay(100);

    // Initialize UART with board-specific RX/TX pins for bidirectional communication
    // RX pin receives printer status and feedback, DTR is configured separately if present
    printer.begin(9600, SERIAL_8N1, boardDefaults.printer.rx, config.printerTxPin);

    // ESP32-S3 requires extra time for UART hardware to fully initialize
    delay(500);

    // Feed watchdog after delay
    esp_task_wdt_reset();

    // Mark printer as ready - the UART is initialized
    printerReady = true;

    LOG_VERBOSE("PRINTER", "printerReady set to TRUE, value check: %s", printerReady ? "CONFIRMED TRUE" : "ERROR: STILL FALSE!");

    LOG_VERBOSE("PRINTER", "UART initialized (TX=%d, RX=%d, DTR=%d)",
                config.printerTxPin, boardDefaults.printer.rx, boardDefaults.printer.dtr);

    LOG_VERBOSE("PRINTER", "Sending printer initialization commands...");

    // Initialize printer with ESC @ (reset command)
    printer.write(0x1B);
    printer.write('@'); // ESC @
    delay(100);

    // Set printer heating parameters from config
    printer.write(0x1B);
    printer.write('7');
    printer.write(heatingDots);     // Heating dots from config
    printer.write(heatingTime);     // Heating time from config
    printer.write(heatingInterval); // Heating interval from config
    delay(50);

    // Enable 180° rotation (which also reverses the line order)
    printer.write(0x1B);
    printer.write('{');
    printer.write(0x01); // ESC { 1
    delay(50);

    LOG_VERBOSE("PRINTER", "Printer initialized successfully - printerReady = %s (address: %p)",
                printerReady ? "TRUE" : "FALSE", (void*)&printerReady);
}

void printMessage()
{
    LOG_VERBOSE("PRINTER", "printMessage() called - printerReady = %s",
                printerReady.load() ? "TRUE" : "FALSE");

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

    LOG_VERBOSE("PRINTER", "Calling printWithHeader...");
    printWithHeader(timestamp, message);

    LOG_VERBOSE("PRINTER", "Message printed successfully");
}

void printStartupMessage()
{
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

            advancePaper(1);

            // Feed watchdog before the actual printing
            esp_task_wdt_reset();

            String timestamp = getFormattedDateTime();
            printWithHeader(timestamp, apContent);

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

        advancePaper(1);

        // Feed watchdog before the actual printing
        esp_task_wdt_reset();

        // Format the startup message with datetime in header and SCRIBE READY in body
        String timestamp = getFormattedDateTime();
        String startupMessage = "SCRIBE READY\n\n" + serverInfo;
        printWithHeader(timestamp, startupMessage);

        // Feed watchdog after thermal printing completes
        esp_task_wdt_reset();
    }
}

void setInverse(bool enable)
{
    printer.write(0x1D);
    printer.write('B');
    printer.write(enable ? 1 : 0); // GS B n
}

void advancePaper(int lines)
{
    for (int i = 0; i < lines; i++)
    {
        printer.write(0x0A); // LF
    }
}

void printWrapped(String text)
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
        printer.println(lines[i]);
    }
}

void printWithHeader(String headerText, String bodyText)
{
    // Acquire printer mutex for multi-core protection (ESP32-S3)
    // Prevents race conditions when AsyncWebServer handlers and main loop both try to print
    if (printerMutex != nullptr && xSemaphoreTake(printerMutex, pdMS_TO_TICKS(5000)) != pdTRUE)
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
    printWrapped(cleanBodyText);

    // Feed watchdog between body and header printing
    esp_task_wdt_reset();

    // Print header last (appears at top after rotation)
    setInverse(true);
    printWrapped(cleanHeaderText);
    setInverse(false);

    advancePaper(2);

    // Feed watchdog after printing completes
    esp_task_wdt_reset();

    // Release printer mutex
    if (printerMutex != nullptr)
    {
        xSemaphoreGive(printerMutex);
    }
}
