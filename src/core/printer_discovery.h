#ifndef PRINTER_DISCOVERY_H
#define PRINTER_DISCOVERY_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "shared_types.h"

void setupPrinterDiscovery();
void setupPrinterLWT();
void handlePrinterDiscovery();
void publishPrinterStatus();
void publishPrinterOfflineStatus();
void onPrinterStatusMessage(const String &topic, const String &payload);
std::vector<DiscoveredPrinter> getDiscoveredPrinters();
void clearOfflinePrinters();
String getPrinterId();
String getFirmwareVersion();

/**
 * @brief Broadcast printer discovery events to SSE clients
 * @param printer The printer that was discovered/updated
 * @param eventType "discovered" or "offline"
 */
void broadcastPrinterEvent(const DiscoveredPrinter &printer, const String &eventType);

extern std::vector<DiscoveredPrinter> discoveredPrinters;

#endif
