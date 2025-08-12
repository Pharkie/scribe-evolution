#include "printer_discovery.h"
#include "mqtt_handler.h"
#include "config_utils.h"
#include "logging.h"
#include "config.h"
#include "../utils/time_utils.h"
#include <WiFi.h>
#include <esp_chip_info.h>

std::vector<DiscoveredPrinter> discoveredPrinters;

const unsigned long PRINTER_OFFLINE_TIMEOUT = 300000; // 5 minutes
const unsigned long STATUS_PUBLISH_INTERVAL = 60000;  // 1 minute
unsigned long lastStatusPublish = 0;

String getPrinterId()
{
    uint64_t chipid = ESP.getEfuseMac();
    return String((uint32_t)(chipid >> 16), HEX);
}

String getFirmwareVersion()
{
    return "1.0.0";
}

void setupPrinterDiscovery()
{
    LOG_VERBOSE("DISCOVERY", "Printer discovery system initialized");
}

void publishPrinterOfflineStatus()
{
    if (!mqttClient.connected())
    {
        return;
    }

    String printerId = getPrinterId();
    String statusTopic = "scribe/printer-status/" + printerId;

    // Create complete offline status payload
    DynamicJsonDocument doc(512);
    doc["name"] = getLocalPrinterName();
    doc["firmware_version"] = getFirmwareVersion();
    doc["mdns"] = String(getMdnsHostname()) + ".local";
    doc["ip_address"] = WiFi.localIP().toString();
    doc["status"] = "offline";
    doc["last_power_on"] = getISOTimestamp();
    doc["timezone"] = getTimezone();

    String payload;
    serializeJson(doc, payload);

    bool published = mqttClient.publish(statusTopic.c_str(), payload.c_str(), true);
    if (published)
    {
        LOG_VERBOSE("DISCOVERY", "Published offline status to %s", statusTopic.c_str());
    }
    else
    {
        LOG_WARNING("DISCOVERY", "Failed to publish offline status to %s", statusTopic.c_str());
    }
}

void publishPrinterStatus()
{
    if (!mqttClient.connected())
    {
        return;
    }

    String printerId = getPrinterId();
    String statusTopic = "scribe/printer-status/" + printerId;

    DynamicJsonDocument doc(512);
    doc["name"] = getLocalPrinterName();
    doc["firmware_version"] = getFirmwareVersion();
    doc["mdns"] = String(getMdnsHostname()) + ".local";
    doc["ip_address"] = WiFi.localIP().toString();
    doc["status"] = "online";
    doc["last_power_on"] = getISOTimestamp();
    doc["timezone"] = getTimezone();

    String payload;
    serializeJson(doc, payload);

    bool published = mqttClient.publish(statusTopic.c_str(), payload.c_str(), true);
    if (published)
    {
        LOG_VERBOSE("DISCOVERY", "Published status to %s", statusTopic.c_str());
    }
    else
    {
        LOG_WARNING("DISCOVERY", "Failed to publish status to %s", statusTopic.c_str());
    }
}

void setupPrinterLWT()
{
    if (!mqttClient.connected())
    {
        return;
    }

    String printerId = getPrinterId();
    String statusTopic = "scribe/printer-status/" + printerId;

    // Create complete LWT payload matching the full status format
    DynamicJsonDocument lwtDoc(512);
    lwtDoc["name"] = getLocalPrinterName();
    lwtDoc["firmware_version"] = getFirmwareVersion();
    lwtDoc["mdns"] = String(getMdnsHostname()) + ".local";
    lwtDoc["ip_address"] = WiFi.localIP().toString();
    lwtDoc["status"] = "offline";
    lwtDoc["last_power_on"] = getISOTimestamp();
    lwtDoc["timezone"] = getTimezone();

    String lwtPayload;
    serializeJson(lwtDoc, lwtPayload);

    LOG_VERBOSE("DISCOVERY", "LWT configured for %s with payload: %s", statusTopic.c_str(), lwtPayload.c_str());
}

void onPrinterStatusMessage(const String &topic, const String &payload)
{
    String printerId = topic.substring(topic.lastIndexOf('/') + 1);
    String ourPrinterId = getPrinterId();

    // Allow our own printer to appear in the discovered list for testing
    // if (printerId == ourPrinterId)
    // {
    //     return; // Ignore our own status messages
    // }

    LOG_VERBOSE("DISCOVERY", "Received status from printer %s: %s", printerId.c_str(), payload.c_str());

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        LOG_WARNING("DISCOVERY", "Failed to parse printer status JSON: %s", error.c_str());
        return;
    }

    String status = doc["status"] | "unknown";
    unsigned long currentTime = millis();

    // Find existing printer or create new entry
    bool found = false;
    for (auto &printer : discoveredPrinters)
    {
        if (printer.printerId == printerId)
        {
            // Update existing printer
            if (status == "offline")
            {
                printer.status = "offline";
                LOG_NOTICE("DISCOVERY", "Printer %s went offline", printer.name.c_str());
                broadcastPrinterEvent(printer, "offline");
            }
            else
            {
                printer.name = doc["name"] | printer.name;
                printer.firmwareVersion = doc["firmware_version"] | printer.firmwareVersion;
                printer.mdns = doc["mdns"] | printer.mdns;
                printer.ipAddress = doc["ip_address"] | printer.ipAddress;
                printer.status = "online";
                printer.lastPowerOn = doc["last_power_on"] | printer.lastPowerOn;
                printer.timezone = doc["timezone"] | printer.timezone;
                printer.lastSeen = currentTime;

                LOG_NOTICE("DISCOVERY", "Updated printer %s (%s)", printer.name.c_str(), printer.ipAddress.c_str());
                broadcastPrinterEvent(printer, "discovered");
            }
            found = true;
            break;
        }
    }

    if (!found && status != "offline")
    {
        // Add new printer
        DiscoveredPrinter newPrinter;
        newPrinter.printerId = printerId;
        newPrinter.name = doc["name"] | "Unknown";
        newPrinter.firmwareVersion = doc["firmware_version"] | "Unknown";
        newPrinter.mdns = doc["mdns"] | "";
        newPrinter.ipAddress = doc["ip_address"] | "";
        newPrinter.status = "online";
        newPrinter.lastPowerOn = doc["last_power_on"] | "";
        newPrinter.timezone = doc["timezone"] | "";
        newPrinter.lastSeen = currentTime;

        discoveredPrinters.push_back(newPrinter);
        LOG_NOTICE("DISCOVERY", "Discovered new printer %s (%s)", newPrinter.name.c_str(), newPrinter.ipAddress.c_str());
        broadcastPrinterEvent(newPrinter, "discovered");
    }
}

void clearOfflinePrinters()
{
    unsigned long currentTime = millis();

    auto it = discoveredPrinters.begin();
    while (it != discoveredPrinters.end())
    {
        if (it->status == "offline" || (currentTime - it->lastSeen > PRINTER_OFFLINE_TIMEOUT))
        {
            LOG_NOTICE("DISCOVERY", "Removing offline printer %s", it->name.c_str());
            it = discoveredPrinters.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void handlePrinterDiscovery()
{
    unsigned long currentTime = millis();

    // Publish our status periodically
    if (currentTime - lastStatusPublish > STATUS_PUBLISH_INTERVAL)
    {
        publishPrinterStatus();
        lastStatusPublish = currentTime;
    }

    // Clean up offline printers periodically
    static unsigned long lastCleanup = 0;
    if (currentTime - lastCleanup > PRINTER_OFFLINE_TIMEOUT)
    {
        clearOfflinePrinters();
        lastCleanup = currentTime;
    }
}

std::vector<DiscoveredPrinter> getDiscoveredPrinters()
{
    return discoveredPrinters;
}

void broadcastPrinterEvent(const DiscoveredPrinter &printer, const String &eventType)
{
    // Create JSON payload for the event
    DynamicJsonDocument doc(512);
    doc["printerId"] = printer.printerId;
    doc["name"] = printer.name;
    doc["firmwareVersion"] = printer.firmwareVersion;
    doc["mdns"] = printer.mdns;
    doc["ipAddress"] = printer.ipAddress;
    doc["status"] = printer.status;
    doc["lastPowerOn"] = printer.lastPowerOn;
    doc["timezone"] = printer.timezone;
    doc["lastSeen"] = (millis() - printer.lastSeen) / 1000; // seconds ago
    doc["eventType"] = eventType;
    doc["timestamp"] = millis();

    String payload;
    serializeJson(doc, payload);

    // Note: Real-time updates now handled by smart polling in frontend
    // No need for server-side event broadcasting

    LOG_VERBOSE("DISCOVERY", "Printer %s event: %s", eventType.c_str(), printer.name.c_str());
}
