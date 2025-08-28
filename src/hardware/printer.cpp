#include "printer.h"
#include "../utils/time_utils.h"
#include "../core/logging.h"
#include "../core/config_utils.h"
#include "../core/shared_types.h"
#include "../core/network.h"
#include "../content/content_generators.h"
#include <WiFi.h>
#include <esp_task_wdt.h>

// Printer object and configuration
HardwareSerial printer(1); // Use UART1 on ESP32-C3
const int maxCharsPerLine = 32;

// === Printer Functions ===
void stabilizePrinterPin()
{
    // Set TX pin to HIGH (idle state) before UART initialization
    // This prevents spurious data from being sent to the printer during boot
    const RuntimeConfig &config = getRuntimeConfig();
    pinMode(config.printerTxPin, OUTPUT);
    digitalWrite(config.printerTxPin, HIGH); // UART idle state is HIGH

    // Small delay to ensure pin is stable
    delay(100);
}

void initializePrinter()
{
    LOG_VERBOSE("PRINTER", "Starting printer initialization...");

    // The TX pin was manually set to HIGH in stabilizePrinterPin()
    // Now initialize UART1 which will take over pin control
    const RuntimeConfig &config = getRuntimeConfig();
    printer.begin(9600, SERIAL_8N1, -1, config.printerTxPin); // baud, config, RX pin (-1 = not used), TX pin

    // Give printer and UART time to settle after pin transition
    delay(100);

    // Feed watchdog after delay
    esp_task_wdt_reset();

    LOG_VERBOSE("PRINTER", "UART initialized, sending printer commands...");

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

    LOG_VERBOSE("PRINTER", "Printer initialized successfully");
}

void printMessage()
{
    LOG_VERBOSE("PRINTER", "Printing message...");

    printWithHeader(currentMessage.timestamp, currentMessage.message);

    LOG_VERBOSE("PRINTER", "Message printed successfully");
}

void printStartupMessage()
{
    // Feed watchdog after first log (network logging can be slow)
    esp_task_wdt_reset();

    if (isAPMode()) {
        // In AP mode, print the proper setup message using existing content generator
        String apContent = generateAPDetailsContent();
        if (apContent.length() > 0) {
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
    } else {
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
}
