#include "logging.h"
#include "LogManager.h"
#include <utils/time_utils.h>
#include "config_utils.h"
#include <esp_task_wdt.h>

// Safe device owner accessor for logging - avoids recursive calls during initialization
static bool firstCall = true;
const char *getSafeDeviceOwner()
{
    if (firstCall)
    {
        // On first call during initialization, just use the default to avoid recursion
        firstCall = false;
        return defaultDeviceOwner;
    }
    // After first call, safe to use the dynamic version
    return getDeviceOwnerKey();
}

void logToFileSystem(const String &message)
{
    // LittleFS is already mounted in main.cpp, no need to call begin() again
    // Check if log rotation is needed
    if (LittleFS.exists(logFileName))
    {
        File logFile = LittleFS.open(logFileName, "r");
        if (logFile && logFile.size() > maxLogFileSize)
        {
            logFile.close();
            rotateLogFile();
        }
        else if (logFile)
        {
            logFile.close();
        }
    }
    // Append to log file
    File logFile = LittleFS.open(logFileName, "a");
    if (logFile)
    {
        logFile.print(message);
        logFile.close();
    }
}

void logToMQTT(const String &message, const String &level, const String &component)
{
    if (mqttClient.connected() && message.length() > 0)
    {
        String topic = String(getDeviceOwnerKey()) + "/logs";
        StaticJsonDocument<512> logDoc;
        logDoc["level"] = level;
        logDoc["component"] = component;
        logDoc["message"] = message;
        logDoc["device"] = getDeviceOwnerKey();
        logDoc["timestamp"] = getISOTimestamp();

        String jsonString;
        serializeJson(logDoc, jsonString);
        mqttClient.publish(topic.c_str(), jsonString.c_str());
    }
}

void logToBetterStack(const String &message, const String &level, const String &component)
{
    // Only send to BetterStack if enabled and configured
    if (!enableBetterStackLogging || strlen(betterStackToken) == 0)
    {
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate validation

    HTTPClient https;
    if (!https.begin(client, betterStackEndpoint))
    {
        return; // Failed to begin HTTPS connection
    }

    // Build JSON payload
    StaticJsonDocument<1024> doc;
    doc["dt"] = getISOTimestamp();
    doc["level"] = level;
    doc["message"] = message;
    doc["component"] = component;
    doc["device"] = getDeviceOwnerKey();

    String payload;
    serializeJson(doc, payload);

    // Set headers
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", String("Bearer ") + betterStackToken);

    // Send POST request
    int httpCode = https.POST(payload);

    // Don't log the response to avoid infinite recursion
    // Just cleanup
    https.end();
}

void rotateLogFile()
{
    // Delete old backup if it exists
    if (LittleFS.exists("/logs/scribe.old.log"))
    {
        LittleFS.remove("/logs/scribe.old.log");
    }

    // Rename current log to backup
    if (LittleFS.exists(logFileName))
    {
        LittleFS.rename(logFileName, "/logs/scribe.old.log");
    }
}

String getLogLevelString(int level)
{
    switch (level)
    {
    case LOG_LEVEL_ERROR:
        return "ERROR";
    case LOG_LEVEL_WARNING:
        return "WARNING";
    case LOG_LEVEL_NOTICE:
        return "NOTICE";
    case LOG_LEVEL_VERBOSE:
        return "VERBOSE";
    default:
        return "UNKNOWN";
    }
}

void logToFile(const char *message)
{
    if (enableFileLogging && message != nullptr)
    {
        logToFileSystem(String(message));
    }
}

void logToMQTT(const char *message, const char *level, const char *component)
{
    if (enableMqttLogging && message != nullptr && level != nullptr)
    {
        logToMQTT(String(message), String(level), String(component ? component : ""));
    }
}

void logToBetterStack(const char *message, const char *level, const char *component)
{
    if (enableBetterStackLogging && message != nullptr && level != nullptr)
    {
        logToBetterStack(String(message), String(level), String(component ? component : ""));
    }
}
