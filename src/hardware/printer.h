#ifndef PRINTER_H
#define PRINTER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <vector>
#include <config/config.h>
#include <utils/character_mapping.h>
#include <web/web_server.h>

// External printer object and configuration
extern HardwareSerial printer;
extern const int maxCharsPerLine;

// Function declarations
void initializePrinter();
void printMessage();
void printStartupMessage();
void setInverse(bool enable);
void advancePaper(int lines);
void printWrapped(String text);
void printWithHeader(String headerText, String bodyText);

#endif // PRINTER_H
