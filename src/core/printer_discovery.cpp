#include "printer_discovery.h"
#include "mqtt_handler.h"
#include "config_utils.h"
#include "logging.h"
#include "config.h"
#include "../utils/time_utils.h"
#include <WiFi.h>
#include <esp_chip_info.h>

std::vector<DiscoveredPrinter> discoveredPrinters;

String getPrinterId()
{
    uint64_t chipid = ESP.getEfuseMac();
    return String((uint32_t)(chipid >> 16), HEX);
}

String getFirmwareVersion()
{
#ifdef FIRMWARE_VERSION
    return FIRMWARE_VERSION;
#else
#error "FIRMWARE_VERSION must be defined at build time in platformio.ini"
#endif
}

String createOfflinePayload()
{
    // Create simple offline status payload (just name and status)
    DynamicJsonDocument doc(128);
    doc["name"] = getLocalPrinterName();
    doc["status"] = "offline";

    String payload;
    serializeJson(doc, payload);
    return payload;
}

void setupPrinterDiscovery()
{
    LOG_VERBOSE("DISCOVERY", "Printer discovery system initialized");
}

void publishPrinterStatus()
{
    LOG_VERBOSE("DISCOVERY", "publishPrinterStatus() called");

    if (!mqttClient.connected())
    {
        LOG_WARNING("DISCOVERY", "MQTT not connected, cannot publish status");
        return;
    }

    String printerId = getPrinterId();
    String statusTopic = "scribe/printer-status/" + printerId;
    LOG_VERBOSE("DISCOVERY", "Publishing status to topic: %s", statusTopic.c_str());

    DynamicJsonDocument doc(512);
    doc["name"] = getLocalPrinterName();
    doc["firmware_version"] = getFirmwareVersion();
    doc["mdns"] = String(getMdnsHostname()) + ".local";
    doc["ip_address"] = WiFi.localIP().toString();
    doc["status"] = "online";
    doc["last_power_on"] = getDeviceBootTime();
    doc["timezone"] = getTimezone();

    String payload;
    serializeJson(doc, payload);
    LOG_VERBOSE("DISCOVERY", "Status payload: %s", payload.c_str());

    bool published = mqttClient.publish(statusTopic.c_str(), payload.c_str(), true);
    if (published)
    {
        LOG_NOTICE("DISCOVERY", "Published status to %s", statusTopic.c_str());
        delay(3000); // 3 second debug delay to observe timing
    }
    else
    {
        LOG_ERROR("DISCOVERY", "Failed to publish status to %s", statusTopic.c_str());
    }
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
                LOG_NOTICE("DISCOVERY", "Printer %s went offline (payload: %s)", printer.name.c_str(), payload.c_str());
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
    }
}

void handlePrinterDiscovery()
{
    // Periodic heartbeat publishing - keeps printer visible to others
    unsigned long currentTime = millis();
    static unsigned long lastStatusPublish = 0;
    const unsigned long STATUS_PUBLISH_INTERVAL = 60000; // 1 minute

    if (currentTime - lastStatusPublish > STATUS_PUBLISH_INTERVAL)
    {
        publishPrinterStatus();
        lastStatusPublish = currentTime;
    }
}

std::vector<DiscoveredPrinter> getDiscoveredPrinters()
{
    return discoveredPrinters;
}
