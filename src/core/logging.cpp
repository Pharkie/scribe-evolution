#include "logging.h"
#include <utils/time_utils.h>
#include "config_utils.h"
#include <esp_task_wdt.h>

// Global instance of multi-output printer
MultiOutputPrint multiOutput;

size_t MultiOutputPrint::write(uint8_t c)
{
    String message = String((char)c);
    return write((const uint8_t *)message.c_str(), message.length());
}

#include "config_utils.h"

// Safe device owner accessor for logging - avoids recursive calls during initialization
static const char *getSafeDeviceOwner()
{
    static bool firstCall = true;
    if (firstCall)
    {
        // On first call during initialization, just use the default to avoid recursion
        firstCall = false;
        return defaultDeviceOwner;
    }
    // After first call, safe to use the dynamic version
    return getDeviceOwnerKey();
}

size_t MultiOutputPrint::write(const uint8_t *buffer, size_t size)
{
    String message;
    for (size_t i = 0; i < size; i++)
    {
        message += (char)buffer[i];
    }

    // Output to Serial if enabled
    if (enableSerialLogging)
    {
        Serial.print(message);
    }

    // Output to file if enabled
    if (enableFileLogging && message.length() > 0)
    {
        logToFile(message.c_str());
    }

    return size;
}

void setupLogging()
{
    // Initialize ArduinoLog with our custom multi-output
    Log.begin(logLevel, &multiOutput);

    // Set prefixes and suffixes for clean formatting
    Log.setPrefix([](Print *_logOutput, int logLevel)
                  {
        String timestamp = getFormattedDateTime();
        String levelStr = getLogLevelString(logLevel);
        _logOutput->print("[");
        _logOutput->print(timestamp);
        _logOutput->print("] [");
        _logOutput->print(levelStr);
        _logOutput->print("] "); });

    Log.setSuffix([](Print *_logOutput, int logLevel)
                  {
        // Add newline at end of each log message
        _logOutput->println(); });

    // Create logs directory if logging to file
    if (enableFileLogging)
    {
        // LittleFS is already mounted in main.cpp
        LittleFS.mkdir("/logs");
    }
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
        // Feed watchdog before potentially slow network operation
        esp_task_wdt_reset();

        // Create JSON log entry
        DynamicJsonDocument doc(1024);
        doc["device_timestamp"] = getFormattedDateTime();
        doc["device"] = String(getMdnsHostname());
        doc["device_owner"] = getSafeDeviceOwner();
        doc["level"] = level;
        doc["message"] = message;

        // Add component if provided
        if (component.length() > 0)
        {
            doc["component"] = component;
        }

        String payload;
        serializeJson(doc, payload);

        mqttClient.publish(mqttLogTopic, payload.c_str());
    }
}

void logToBetterStack(const String &message, const String &level, const String &component)
{
    if (strlen(betterStackToken) == 0 || WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    http.begin(client, betterStackEndpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + String(betterStackToken));

    String cleanMessage = message;
    String finalComponent = component;

    // If no component provided, try to extract from message (format: [COMPONENT] message)
    if (component.length() == 0 && message.startsWith("[") && message.indexOf("] ") > 0)
    {
        int endBracket = message.indexOf("] ");
        finalComponent = message.substring(1, endBracket);
        cleanMessage = message.substring(endBracket + 2);
    }

    // Create BetterStack log entry with structured tags
    DynamicJsonDocument doc(1024);
    doc["device_owner"] = getSafeDeviceOwner();
    doc["level"] = level;
    doc["message"] = cleanMessage;
    doc["device_timestamp"] = getFormattedDateTime();

    // Add component as a structured tag if present
    if (finalComponent.length() > 0)
    {
        doc["component"] = finalComponent;
    }

    String payload;
    serializeJson(doc, payload);

    http.POST(payload);
    http.end();
}

void rotateLogFile()
{
    // LittleFS is already mounted in main.cpp, no need to call begin() again

    // Remove old backup if it exists
    String backupName = String(logFileName) + ".old";
    if (LittleFS.exists(backupName))
    {
        LittleFS.remove(backupName);
    }

    // Rename current log to backup
    if (LittleFS.exists(logFileName))
    {
        LittleFS.rename(logFileName, backupName);
    }
}

String getLogLevelString(int level)
{
    switch (level)
    {
    case LOG_LEVEL_SILENT:
        return "SILENT";
    case LOG_LEVEL_FATAL:
        return "FATAL";
    case LOG_LEVEL_ERROR:
        return "ERROR";
    case LOG_LEVEL_WARNING:
        return "WARNING";
    case LOG_LEVEL_NOTICE:
        return "NOTICE";
    case LOG_LEVEL_TRACE:
        return "TRACE";
    case LOG_LEVEL_VERBOSE:
        return "VERBOSE";
    default:
        return "UNKNOWN";
    }
}

void structuredLog(const char *component, int level, const char *format, ...)
{
    // Check if this log level should be processed
    if (level > logLevel)
    {
        return; // Skip logging if level is higher than configured threshold
    }

    // Format the message
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    String message = String(buffer);
    String levelStr = getLogLevelString(level);

    // Log to Serial/File with component tag (if enabled)
    if (enableSerialLogging || enableFileLogging)
    {
        String formattedMessage = "[" + String(getSafeDeviceOwner()) + "] [" + String(component) + "] " + message;

        // Use ArduinoLog for Serial/File outputs
        switch (level)
        {
        case LOG_LEVEL_ERROR:
            Log.error(formattedMessage.c_str());
            break;
        case LOG_LEVEL_WARNING:
            Log.warning(formattedMessage.c_str());
            break;
        case LOG_LEVEL_VERBOSE:
            Log.verbose(formattedMessage.c_str());
            break;
        default:
            Log.notice(formattedMessage.c_str());
            break;
        }
    }

    // Send structured logs to MQTT/BetterStack
    if (enableMQTTLogging && mqttClient.connected())
    {
        logToMQTT(message, levelStr, component);
    }

    if (enableBetterStackLogging && WiFi.status() == WL_CONNECTED)
    {
        logToBetterStack(message, levelStr, component);
    }
}
