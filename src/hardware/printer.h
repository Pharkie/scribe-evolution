#ifndef PRINTER_H
#define PRINTER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#include <atomic>
#include <config/config.h>
#include <utils/character_mapping.h>
#include <web/web_server.h>

// External printer object and configuration
// Note: Printer supports bidirectional communication (CSN-A4L has realtime status),
// but currently configured for TX-only to avoid RX buffer overflow issues.
// RX can be enabled when printer status reading is implemented.
extern HardwareSerial printer;
extern const int maxCharsPerLine;
extern std::atomic<bool> printerReady; // Thread-safe ready flag

// Function declarations
void initializePrinter();
bool isPrinterReady(); // Check if printer is ready for writes
void printMessage();
void printStartupMessage();
void setInverse(bool enable);
void advancePaper(int lines);
void printWrapped(String text);
void printWithHeader(String headerText, String bodyText);

#endif // PRINTER_H
