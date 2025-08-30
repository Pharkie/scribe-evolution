#include "mqtt_handler.h"
#include "../utils/time_utils.h"
#include "../hardware/printer.h"
#include "logging.h"
#include "config_utils.h"
#include "config_loader.h"
#include "printer_discovery.h"
#include <WiFi.h>
#include <esp_task_wdt.h>

// MQTT objects
WiFiClientSecure wifiSecureClient;
PubSubClient mqttClient(wifiSecureClient);
unsigned long lastMQTTReconnectAttempt = 0;
const unsigned long mqttReconnectInterval = 5000; // 5 seconds

// Track current subscription
String currentSubscribedTopic = "";

// Persistent CA certificate buffer (must outlive wifiSecureClient)
static String caCertificateBuffer;

// === MQTT Functions ===
void setupMQTT()
{
    // Load CA certificate once and store in persistent buffer
    LOG_NOTICE("MQTT", "Loading CA certificate from /resources/isrg-root-x1.pem");
    File certFile = LittleFS.open("/resources/isrg-root-x1.pem", "r");
    if (!certFile) {
        LOG_ERROR("MQTT", "Failed to open CA certificate file");
        return;
    }
    
    caCertificateBuffer = certFile.readString();
    certFile.close();
    
    LOG_NOTICE("MQTT", "CA certificate loaded, length: %d bytes", caCertificateBuffer.length());
    
    if (caCertificateBuffer.length() == 0) {
        LOG_ERROR("MQTT", "CA certificate file is empty");
        return;
    }
    
    // Validate certificate format
    bool validCert = caCertificateBuffer.indexOf("-----BEGIN CERTIFICATE-----") != -1 && 
                     caCertificateBuffer.indexOf("-----END CERTIFICATE-----") != -1 &&
                     caCertificateBuffer.length() > 100;
    
    if (!validCert) {
        LOG_ERROR("MQTT", "CA certificate file format is invalid");
        return;
    }
    
    LOG_NOTICE("MQTT", "CA certificate validation passed, configuring WiFiClientSecure");
    
    wifiSecureClient.setCACert(caCertificateBuffer.c_str()); // Static buffer guarantees lifetime
    wifiSecureClient.setHandshakeTimeout(10000);
    
    // Configure MQTT client
    const RuntimeConfig &config = getRuntimeConfig();
    mqttClient.setServer(config.mqttServer.c_str(), config.mqttPort);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(4096);

    // Initial connection attempt
    connectToMQTT();

    const char* tlsMode = mqttClient.connected() ? "Secure (TLS with CA verification)" : "Secure (TLS configured, connection pending)";
    LOG_NOTICE("MQTT", "MQTT server configured: %s:%d | Inbox topic: %s | TLS mode: %s | Buffer size: 4096 bytes", config.mqttServer.c_str(), config.mqttPort, getLocalPrinterTopic(), tlsMode);
}

void connectToMQTT()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        // WiFi not connected, skipping MQTT connection
        return;
    }

    String printerId = getPrinterId();
    String clientId = "ScribePrinter-" + printerId; // Use consistent ID based on printer ID

    // Feed watchdog before potentially blocking MQTT connection
    esp_task_wdt_reset();

    bool connected = false;

    // Set up LWT for printer discovery
    String statusTopic = "scribe/printer-status/" + printerId;

    // Use the same offline payload format as graceful shutdown
    String lwtPayload = createOfflinePayload();

    // Try connection with or without credentials, including LWT
    const RuntimeConfig &config = getRuntimeConfig();
    if (config.mqttUsername.length() > 0 && config.mqttPassword.length() > 0)
    {
        connected = mqttClient.connect(clientId.c_str(),
                                       config.mqttUsername.c_str(),
                                       config.mqttPassword.c_str(),
                                       statusTopic.c_str(),
                                       0,
                                       true,
                                       lwtPayload.c_str());
    }
    else
    {
        connected = mqttClient.connect(clientId.c_str(),
                                       statusTopic.c_str(),
                                       0,
                                       true,
                                       lwtPayload.c_str());
    }
    
    // Feed watchdog again after connection attempt
    esp_task_wdt_reset();

    if (connected)
    {
        // Subscribe to the inbox topic
        String newTopic = String(getLocalPrinterTopic());
        if (!mqttClient.subscribe(newTopic.c_str()))
        {
            LOG_ERROR("MQTT", "MQTT connected. Failed to subscribe to topic: %s", newTopic.c_str());
        }
        else
        {
            currentSubscribedTopic = newTopic;
            LOG_VERBOSE("MQTT", "Successfully subscribed to topic: %s", newTopic.c_str());
        }

        // Subscribe to printer discovery topics to immediately process retained messages
        if (!mqttClient.subscribe("scribe/printer-status/+"))
        {
            LOG_WARNING("MQTT", "Failed to subscribe to printer status topics");
        }
        else
        {
            LOG_VERBOSE("MQTT", "Subscribed to printer discovery topics. Should receive retained messages immediately");
        }

        // Publish initial online status immediately after connection
        LOG_NOTICE("MQTT", "Publishing initial online status after connection");
        publishPrinterStatus();
    }
    else
    {
        LOG_WARNING("MQTT", "MQTT connection failed, state: %d - Will retry in %d seconds", mqttClient.state(), mqttReconnectInterval / 1000);
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    LOG_VERBOSE("MQTT", "MQTT message received on topic: %s", topic);

    // Convert payload to string
    String message = "";
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }

    LOG_VERBOSE("MQTT", "MQTT payload: %s", message.c_str());

    String topicStr = String(topic);

    // Check if this is a printer status message
    if (topicStr.startsWith("scribe/printer-status/"))
    {
        onPrinterStatusMessage(topicStr, message);
    }
    else
    {
        // Handle regular print messages - pass topic to extract sender
        handleMQTTMessage(topicStr, message);
    }
}

void handleMQTTMessage(String topic, String message)
{
    // Parse JSON message
    DynamicJsonDocument doc(4096); // Increased to match MQTT buffer size
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        LOG_ERROR("MQTT", "Failed to parse MQTT JSON: %s", error.c_str());
        return;
    }

    // Handle text messages (content should already be formatted with headers)
    if (doc.containsKey("message"))
    {
        String printMessage = doc["message"].as<String>();
        String timestamp = getFormattedDateTime();

        // Extract sender name from JSON payload (if provided)
        String senderName = "";
        if (doc.containsKey("sender"))
        {
            senderName = doc["sender"].as<String>();
        }

        // Add "from {sender}" to message headers if sender identified
        // Note: Check if message already has "from" to avoid duplicates
        if (senderName.length() > 0 && printMessage.indexOf(" from ") == -1)
        {
            // Find the header end (first newline or end of string)
            int headerEnd = printMessage.indexOf('\n');
            if (headerEnd == -1) headerEnd = printMessage.length();
            
            // Insert " from {sender}" before the newline or at the end
            String modifiedMessage = printMessage.substring(0, headerEnd) + 
                                   " from " + senderName;
            if (headerEnd < printMessage.length())
            {
                modifiedMessage += printMessage.substring(headerEnd);
            }
            printMessage = modifiedMessage;
            
            LOG_VERBOSE("MQTT", "Added sender to header: %s", senderName.c_str());
        }
        else if (senderName.length() > 0)
        {
            LOG_VERBOSE("MQTT", "Sender header already present in message, skipping duplicate");
        }

        // Print immediately using the existing printWithHeader function
        printWithHeader(timestamp, printMessage);
    }
    else
    {
        LOG_WARNING("MQTT", "MQTT JSON missing 'message' field");
    }
}

// === MQTT Connection Handler ===
void handleMQTTConnection()
{
    // Handle MQTT connection and messages
    if (!mqttClient.connected())
    {
        if (millis() - lastMQTTReconnectAttempt > mqttReconnectInterval)
        {
            LOG_VERBOSE("MQTT", "MQTT disconnected, attempting reconnection...");

            // Feed watchdog before potentially blocking MQTT operation
            esp_task_wdt_reset();

            connectToMQTT();
            lastMQTTReconnectAttempt = millis();
        }
    }
    else
    {
        mqttClient.loop(); // Handle incoming MQTT messages
    }
}

void updateMQTTSubscription()
{
    if (!mqttClient.connected())
    {
        LOG_VERBOSE("MQTT", "MQTT not connected, subscription will be updated on next connection");
        return;
    }

    String newTopic = String(getLocalPrinterTopic());

    // Check if we need to update subscription
    if (currentSubscribedTopic == newTopic)
    {
        LOG_VERBOSE("MQTT", "MQTT subscription already up to date: %s", newTopic.c_str());
        return;
    }

    // Unsubscribe from old topic if we were subscribed to something
    if (currentSubscribedTopic.length() > 0)
    {
        if (mqttClient.unsubscribe(currentSubscribedTopic.c_str()))
        {
            LOG_NOTICE("MQTT", "Unsubscribed from old topic: %s", currentSubscribedTopic.c_str());
        }
        else
        {
            LOG_WARNING("MQTT", "Failed to unsubscribe from old topic: %s", currentSubscribedTopic.c_str());
        }
    }

    // Subscribe to new topic
    if (mqttClient.subscribe(newTopic.c_str()))
    {
        currentSubscribedTopic = newTopic;
        LOG_NOTICE("MQTT", "Successfully subscribed to new topic: %s", newTopic.c_str());
    }
    else
    {
        LOG_ERROR("MQTT", "Failed to subscribe to new topic: %s", newTopic.c_str());
        currentSubscribedTopic = ""; // Clear since subscription failed
    }
}

void setupMQTTWithDiscovery()
{
    setupMQTT();
    setupPrinterDiscovery();
}

// Dynamic MQTT control functions
bool isMQTTEnabled()
{
    const RuntimeConfig &config = getRuntimeConfig();
    return config.mqttEnabled;
}

void startMQTTClient()
{
    if (!isMQTTEnabled())
    {
        LOG_VERBOSE("MQTT", "MQTT is disabled, not starting client");
        return;
    }
    
    LOG_NOTICE("MQTT", "Starting MQTT client");
    LOG_NOTICE("MQTT", "Note: Initial SSL handshake errors are normal for cloud MQTT brokers");
    
    // Ensure WiFi is stable before attempting MQTT connection
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_WARNING("MQTT", "WiFi not connected, delaying MQTT startup");
        return; // Will retry on next handleMQTTConnection() call
    }
    
    setupMQTT();
}

void stopMQTTClient()
{
    if (mqttClient.connected())
    {
        LOG_NOTICE("MQTT", "Stopping MQTT client");
        mqttClient.disconnect();
    }
    currentSubscribedTopic = "";
}
