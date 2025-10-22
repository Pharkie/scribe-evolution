#include "printer_discovery.h"
#include "mqtt_handler.h"
#include "config_utils.h"
#include "logging.h"
#include <config/config.h>
#include <utils/time_utils.h>
#include <web/web_server.h>
#include <WiFi.h>
#include <esp_chip_info.h>
#include <esp_task_wdt.h>

std::vector<DiscoveredPrinter> discoveredPrinters;

String getPrinterId()
{
    uint64_t chipid = ESP.getEfuseMac();
    return String((uint32_t)(chipid >> 16), HEX);
}

String getFirmwareVersion()
{
    return FIRMWARE_VERSION;
}

String createOfflinePayload()
{
    // Create simple offline status payload (just name, chipModel and status)
    JsonDocument doc;
    doc["name"] = getLocalPrinterName();
    doc["chipModel"] = ESP.getChipModel();
    doc["status"] = "offline";

    String payload;
    serializeJson(doc, payload);
    return payload;
}

void setupPrinterDiscovery()
{
    LOG_VERBOSE("MQTT", "Printer discovery system initialized");
}

void publishPrinterStatus()
{
    if (!MQTTManager::instance().isConnected())
    {
        LOG_WARNING("MQTT", "MQTT not connected, cannot publish status");
        return;
    }

    String printerId = getPrinterId();
    String statusTopic = MqttTopics::buildStatusTopic(printerId);

    JsonDocument doc;
    doc["printerId"] = printerId;
    doc["name"] = getLocalPrinterName();
    doc["firmwareVersion"] = getFirmwareVersion();
    doc["chipModel"] = ESP.getChipModel();
    doc["mdns"] = String(getMdnsHostname()) + ".local";
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["status"] = "online";
    doc["lastPowerOn"] = getDeviceBootTime();
    doc["timezone"] = getTimezone();

    String payload;
    serializeJson(doc, payload);

    bool published = MQTTManager::instance().publishRawMessage(statusTopic, payload, true);

    // Feed watchdog after potentially blocking MQTT publish operation
    esp_task_wdt_reset();

    if (published)
    {
        LOG_VERBOSE("MQTT", "Published printer status to %s (%d chars, retained)", statusTopic.c_str(), payload.length());
    }
    else
    {
        LOG_ERROR("MQTT", "Failed to publish status to %s", statusTopic.c_str());
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

    // Check for empty payload
    if (payload.length() == 0)
    {
        LOG_WARNING("MQTT", "Received empty status payload from printer %s - ignoring", printerId.c_str());
        return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        LOG_WARNING("MQTT", "Failed to parse printer status JSON: %s", error.c_str());
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
                LOG_VERBOSE("MQTT", "Printer %s went offline (payload: %s)", printer.name.c_str(), payload.c_str());

                // Notify web clients via SSE
                sendPrinterUpdate();
            }
            else
            {
                printer.name = doc["name"] | printer.name;
                printer.firmwareVersion = doc["firmwareVersion"] | printer.firmwareVersion;
                printer.chipModel = doc["chipModel"] | printer.chipModel;
                printer.mdns = doc["mdns"] | printer.mdns;
                printer.ipAddress = doc["ipAddress"] | printer.ipAddress;
                printer.status = "online";
                printer.lastPowerOn = doc["lastPowerOn"] | printer.lastPowerOn;
                printer.timezone = doc["timezone"] | printer.timezone;
                printer.lastSeen = currentTime;

                LOG_VERBOSE("MQTT", "Updated printer %s (%s)", printer.name.c_str(), printer.ipAddress.c_str());

                // Notify web clients via SSE
                sendPrinterUpdate();
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
        newPrinter.firmwareVersion = doc["firmwareVersion"] | "Unknown";
        newPrinter.chipModel = doc["chipModel"] | "Unknown";
        newPrinter.mdns = doc["mdns"] | "";
        newPrinter.ipAddress = doc["ipAddress"] | "";
        newPrinter.status = "online";
        newPrinter.lastPowerOn = doc["lastPowerOn"] | "";
        newPrinter.timezone = doc["timezone"] | "";
        newPrinter.lastSeen = currentTime;

        discoveredPrinters.push_back(newPrinter);
        LOG_VERBOSE("MQTT", "Discovered new printer %s (%s)", newPrinter.name.c_str(), newPrinter.ipAddress.c_str());

        // Notify web clients via SSE
        sendPrinterUpdate();
    }
}

void handlePrinterDiscovery()
{
    // Periodic heartbeat publishing - keeps printer visible to others
    unsigned long currentTime = millis();
    static unsigned long lastStatusPublish = 0;
    const unsigned long STATUS_PUBLISH_INTERVAL = printerDiscoveryHeartbeatIntervalMs;

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
