#ifndef PRINTER_DISCOVERY_H
#define PRINTER_DISCOVERY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "shared_types.h"

void setupPrinterDiscovery();
void handlePrinterDiscovery();
void publishPrinterStatus();
void onPrinterStatusMessage(const String &topic, const String &payload);
std::vector<DiscoveredPrinter> getDiscoveredPrinters();
String getPrinterId();
String getFirmwareVersion();
String createOfflinePayload();

extern std::vector<DiscoveredPrinter> discoveredPrinters;

#endif
