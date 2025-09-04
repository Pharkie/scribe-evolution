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

// MQTT State Machine
enum MQTTState {
    MQTT_STATE_DISABLED,
    MQTT_STATE_ENABLED_DISCONNECTED, 
    MQTT_STATE_CONNECTING,
    MQTT_STATE_CONNECTED,
    MQTT_STATE_DISCONNECTING
};

static MQTTState mqttState = MQTT_STATE_DISABLED;
static unsigned long stateChangeTime = 0;

// MQTT connection management (using config.h values)
unsigned long lastMQTTReconnectAttempt = 0;
static int consecutiveFailures = 0;
static unsigned long lastFailureTime = 0;

// Track current subscription
String currentSubscribedTopic = "";

// Persistent CA certificate buffer (must outlive wifiSecureClient)
static String caCertificateBuffer;

// Guard to prevent duplicate MQTT initialization
static bool mqttSetupCompleted = false;

// === MQTT Functions ===
void setupMQTT()
{
    // Prevent duplicate initialization
    if (mqttSetupCompleted) {
        LOG_VERBOSE("MQTT", "MQTT already configured, skipping setup");
        return;
    }
    // Load CA certificate exactly like test - local String variable
    LOG_VERBOSE("MQTT", "Loading CA certificate from /resources/isrg-root-x1.pem");
    File certFile = LittleFS.open("/resources/isrg-root-x1.pem", "r");
    if (!certFile) {
        LOG_ERROR("MQTT", "Failed to open CA certificate file");
        return;
    }
    
    String certContent = certFile.readString();
    certFile.close();
    
    LOG_VERBOSE("MQTT", "CA certificate loaded, length: %d bytes", certContent.length());
    
    if (certContent.length() == 0) {
        LOG_ERROR("MQTT", "CA certificate file is empty");
        return;
    }
    
    // Validate certificate format
    bool validCert = certContent.indexOf("-----BEGIN CERTIFICATE-----") != -1 && 
                     certContent.indexOf("-----END CERTIFICATE-----") != -1 &&
                     certContent.length() > 100;
    
    if (!validCert) {
        LOG_ERROR("MQTT", "CA certificate file format is invalid");
        return;
    }
    
    LOG_VERBOSE("MQTT", "CA certificate validation passed, configuring WiFiClientSecure");
    
    // Store in static buffer to ensure lifetime during connection attempts
    caCertificateBuffer = certContent;
    
    // Configure exactly like test - local string passed to setCACert
    wifiSecureClient.setCACert(caCertificateBuffer.c_str());
    wifiSecureClient.setHandshakeTimeout(mqttTlsHandshakeTimeoutMs);
    
    // Ensure MQTT client uses the refreshed secure client
    mqttClient.setClient(wifiSecureClient);
    
    // Configure MQTT client
    const RuntimeConfig &config = getRuntimeConfig();
    mqttClient.setServer(config.mqttServer.c_str(), config.mqttPort);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(mqttBufferSize);

    // Don't call connectToMQTT() here anymore - let state machine handle it

    const char* tlsMode = mqttClient.connected() ? "Secure (TLS with CA verification)" : "Secure (TLS configured, connection pending)";
    LOG_NOTICE("MQTT", "MQTT server configured: %s:%d | Inbox topic: %s | TLS mode: %s | Buffer size: %d bytes", config.mqttServer.c_str(), config.mqttPort, getLocalPrinterTopic(), tlsMode, mqttBufferSize);
    
    // Mark setup as completed
    mqttSetupCompleted = true;
}

void connectToMQTT()
{
    LOG_VERBOSE("MQTT", "=== connectToMQTT() ENTRY ===");
    
    // State machine handles duplicate prevention - no static flags needed
    if (mqttState != MQTT_STATE_CONNECTING) {
        LOG_ERROR("MQTT", "connectToMQTT called in wrong state: %d", mqttState);
        return;
    }
    
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_WARNING("MQTT", "WiFi not connected, aborting MQTT connection");
        mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
        return;
    }
    
    // Check if we should skip connection due to consecutive failures
    if (consecutiveFailures >= mqttMaxConsecutiveFailures)
    {
        if (millis() - lastFailureTime < mqttFailureCooldownMs)
        {
            LOG_VERBOSE("MQTT", "Still in cooldown period, returning to disconnected state");
            mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
            return;
        }
        else
        {
            LOG_NOTICE("MQTT", "Cooldown period expired, resetting failure count and attempting reconnection");
            consecutiveFailures = 0;
        }
    }

    // Debug socket state before connection attempt
    LOG_VERBOSE("MQTT", "Connection attempt - WiFi status: %d, wifiSecureClient.connected(): %d", 
               WiFi.status(), wifiSecureClient.connected());
    
    // Always clean the client state like test does with fresh object
    if (wifiSecureClient.connected()) {
        LOG_VERBOSE("MQTT", "Stopping existing WiFiClientSecure connection");
        wifiSecureClient.stop();
    }
    
    // Force clean state by recreating the secure client (match test behavior)
    wifiSecureClient = WiFiClientSecure();
    wifiSecureClient.setCACert(caCertificateBuffer.c_str());
    wifiSecureClient.setHandshakeTimeout(mqttTlsHandshakeTimeoutMs);
    wifiSecureClient.setTimeout(mqttTlsHandshakeTimeoutMs);
    
    LOG_VERBOSE("MQTT", "WiFiClientSecure reset to clean state");

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
    
    // Set timeout to prevent blocking (already set above, but ensuring consistency)
    wifiSecureClient.setTimeout(mqttTlsHandshakeTimeoutMs);
    
    try {
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
    } catch (...) {
        LOG_ERROR("MQTT", "MQTT connection threw exception, treating as failed");
        connected = false;
    }
    
    // Feed watchdog again after connection attempt
    esp_task_wdt_reset();

    if (connected)
    {
        // Connection successful - update state
        mqttState = MQTT_STATE_CONNECTED;
        consecutiveFailures = 0;
        
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
        // Connection failed - return to disconnected state
        mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
        consecutiveFailures++;
        lastFailureTime = millis();
        
        int state = mqttClient.state();
        LOG_WARNING("MQTT", "MQTT connection failed (attempt %d/%d), state: %d - Will retry in %lums", 
                   consecutiveFailures, mqttMaxConsecutiveFailures, state, mqttReconnectIntervalMs);
        
        if (consecutiveFailures >= mqttMaxConsecutiveFailures)
        {
            LOG_ERROR("MQTT", "Too many consecutive MQTT failures (%d), entering cooldown mode for %lums to prevent system instability", 
                     mqttMaxConsecutiveFailures, mqttFailureCooldownMs);
        }
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
    switch(mqttState)
    {
        case MQTT_STATE_DISABLED:
            // Nothing to do
            return;
            
        case MQTT_STATE_ENABLED_DISCONNECTED:
            // Check if it's time to reconnect
            if (millis() - lastMQTTReconnectAttempt > mqttReconnectIntervalMs)
            {
                LOG_VERBOSE("MQTT", "Starting connection attempt");
                mqttState = MQTT_STATE_CONNECTING;
                stateChangeTime = millis();
                
                // Setup MQTT if not already done, then connect
                if (WiFi.status() == WL_CONNECTED)
                {
                    setupMQTT();  // Load certs, configure client
                    connectToMQTT();  // Actually connect
                }
                else
                {
                    LOG_WARNING("MQTT", "WiFi not connected, returning to disconnected state");
                    mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
                }
                
                lastMQTTReconnectAttempt = millis();
            }
            break;
            
        case MQTT_STATE_CONNECTING:
            // Check for timeout
            if (millis() - stateChangeTime > mqttConnectionTimeoutMs)
            {
                LOG_ERROR("MQTT", "Connection timeout after %lums", mqttConnectionTimeoutMs);
                mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
                
                // Clean up stuck connection
                if (wifiSecureClient.connected())
                {
                    wifiSecureClient.stop();
                }
            }
            // Note: connectToMQTT() will change state when connection completes
            break;
            
        case MQTT_STATE_CONNECTED:
            // Process MQTT messages
            mqttClient.loop();
            
            // Check if still connected
            if (!mqttClient.connected())
            {
                LOG_WARNING("MQTT", "Connection lost");
                mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
            }
            break;
            
        case MQTT_STATE_DISCONNECTING:
            // Transitional state - should be brief
            break;
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


// Dynamic MQTT control functions
bool isMQTTEnabled()
{
    const RuntimeConfig &config = getRuntimeConfig();
    return config.mqttEnabled;
}

void startMQTTClient(bool immediate)
{
    if (!isMQTTEnabled())
    {
        LOG_VERBOSE("MQTT", "MQTT is disabled in config, not starting client");
        return;
    }
    
    if (mqttState == MQTT_STATE_DISABLED)
    {
        LOG_NOTICE("MQTT", "Enabling MQTT client (immediate=%s)", immediate ? "true" : "false");
        mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
        
        if (immediate)
        {
            // Force immediate connection on next loop iteration
            lastMQTTReconnectAttempt = 0;
        }
        else
        {
            // Wait normal reconnect interval
            lastMQTTReconnectAttempt = millis();
        }
    }
}

void stopMQTTClient()
{
    LOG_NOTICE("MQTT", "Stopping MQTT client");
    mqttState = MQTT_STATE_DISCONNECTING;
    
    // Disconnect MQTT
    if (mqttClient.connected())
    {
        mqttClient.disconnect();
    }
    
    // Clean up SSL connection
    if (wifiSecureClient.connected())
    {
        wifiSecureClient.stop();
    }
    
    // Reset ALL state variables
    mqttState = MQTT_STATE_DISABLED;
    currentSubscribedTopic = "";
    consecutiveFailures = 0;
    lastMQTTReconnectAttempt = 0;
    lastFailureTime = 0;
}

