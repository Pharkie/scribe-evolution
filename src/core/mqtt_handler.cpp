#include "mqtt_handler.h"
#include <utils/time_utils.h>
#include <hardware/printer.h>
#include "logging.h"
#include "config_utils.h"
#include "config_loader.h"
#include "printer_discovery.h"
#include "ManagerLock.h"
#include <content/memo_handler.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

// ============================================================================
// GLOBAL EVENT HANDLERS (Required by ESP32MQTTClient)
// ============================================================================

// Connection callback - called when MQTT connection is established
void onMqttConnect(esp_mqtt_client_handle_t client) {
    if (MQTTManager::instance().mqttClient.isMyTurn(client)) {
        LOG_NOTICE("MQTT", "Connected to MQTT broker");
        MQTTManager::instance().onConnectionEstablished();
    }
}

// Message callback wrapper - called by ESP32MQTTClient for incoming messages
// Signature must match MessageReceivedCallbackWithTopic: void(const std::string&, const std::string&)
void onMqttMessageCallback(const std::string &topic, const std::string &payload) {
    // Convert std::string to Arduino String for internal handling
    MQTTManager::instance().onMessageReceived(String(topic.c_str()), String(payload.c_str()));
}

// Event handler - routes all MQTT events to ESP32MQTTClient
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event) {
    MQTTManager::instance().mqttClient.onEventCallback(event);
    return ESP_OK;
}
#else  // IDF CHECK
void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
    MQTTManager::instance().mqttClient.onEventCallback(event);
}
#endif // IDF CHECK

// ============================================================================
// MQTTMANAGER SINGLETON IMPLEMENTATION
// ============================================================================

// Singleton instance (Meyer's singleton - thread-safe in C++11+)
MQTTManager& MQTTManager::instance() {
    static MQTTManager instance;
    return instance;
}

// Constructor
MQTTManager::MQTTManager() {
    // ESP32MQTTClient initialization happens in begin()
}

// Initialize (must be called in setup)
void MQTTManager::begin() {
    if (initialized) {
        LOG_VERBOSE("MQTT", "MQTTManager already initialized");
        return;
    }

    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        LOG_ERROR("MQTT", "Failed to create MQTTManager mutex!");
        return;
    }

    initialized = true;
    LOG_NOTICE("MQTT", "MQTTManager initialized (thread-safe singleton)");
}

// Event callback: Connection established
void MQTTManager::onConnectionEstablished() {
    // Note: This is called from global onMqttConnect which runs in ESP-MQTT task
    // We need thread-safe access

    ManagerLock lock(mutex, "MQTT-Connect");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire mutex in onConnectionEstablished");
        return;
    }

    // Update state
    mqttState = MQTT_STATE_CONNECTED;
    consecutiveFailures = 0;

    // Subscribe to the print topic
    String newTopic = String(getLocalPrinterTopic());
    std::string newTopicStd = newTopic.c_str();
    bool printTopicOk = mqttClient.subscribe(newTopicStd, onMqttMessageCallback, 0);
    if (printTopicOk)
    {
        currentSubscribedTopic = newTopic;
    }
    else
    {
        LOG_ERROR("MQTT", "Failed to subscribe to topic: %s", newTopic.c_str());
    }

    // Subscribe to printer discovery topics
    String statusSubscription = MqttTopics::buildStatusSubscription();
    std::string statusSubscriptionStd = statusSubscription.c_str();
    bool statusTopicOk = mqttClient.subscribe(statusSubscriptionStd, onMqttMessageCallback, 0);

    if (!statusTopicOk)
    {
        LOG_WARNING("MQTT", "Failed to subscribe to printer status topics");
    }

    // Log success summary for both subscriptions
    if (printTopicOk && statusTopicOk)
    {
        LOG_VERBOSE("MQTT", "Subscribed to MQTT topics: %s, discovery", currentSubscribedTopic.c_str());
    }

    // Set flag to publish initial online status after mutex is released
    needPublishStatus = true;
}

// Event callback: Message received
void MQTTManager::onMessageReceived(String topic, String message) {
    // Note: This is called from ESP-MQTT task via onMqttMessageCallback
    // We need thread-safe access

    ManagerLock lock(mutex, "MQTT-Message");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire mutex in onMessageReceived");
        return;
    }

    LOG_VERBOSE("MQTT", "MQTT message received: topic=%s, payload=%s", topic.c_str(), message.c_str());

    // Check if this is a printer status message
    if (MqttTopics::isStatusTopic(topic))
    {
        onPrinterStatusMessage(topic, message);
    }
    else
    {
        // Handle regular print messages - pass topic to extract sender
        handleMQTTMessageInternal(topic, message);
    }
}

// Internal helper: handleMQTTMessageInternal (mutex already held)
void MQTTManager::handleMQTTMessageInternal(String topic, String message)
{
    // Parse JSON message
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        LOG_ERROR("MQTT", "Failed to parse MQTT JSON: %s", error.c_str());
        return;
    }

    String timestamp = getFormattedDateTime();

    // Only handle structured messages (header + body + sender)
    if (!doc["header"].is<JsonVariant>() || !doc["body"].is<JsonVariant>() || !doc["sender"].is<JsonVariant>())
    {
        LOG_ERROR("MQTT", "MQTT JSON must contain 'header', 'body', and 'sender' fields");
        return;
    }

    String header = doc["header"].as<String>();
    String body = doc["body"].as<String>();
    String senderName = doc["sender"].as<String>();

    // Expand memo placeholders at print time (if this is a memo)
    if (header.startsWith("MEMO"))
    {
        body = processMemoPlaceholders(body);
        LOG_VERBOSE("MQTT", "Expanded memo placeholders for: %s", header.c_str());
    }

    // Construct header with sender information
    String finalHeader = header;
    if (senderName.length() > 0)
    {
        finalHeader += " from " + senderName;
    }

    // Format final message: header + body
    String printMessage = finalHeader + "\n\n" + body;

    // Print immediately using printerManager
    printerManager.printWithHeader(timestamp, printMessage);

    LOG_VERBOSE("MQTT", "Processed structured message: %s (%d chars)",
               finalHeader.c_str(), printMessage.length());
}

// Internal helper: setupMQTTInternal (mutex already held)
void MQTTManager::setupMQTTInternal()
{
    // Prevent duplicate initialization
    if (mqttSetupCompleted) {
        LOG_VERBOSE("MQTT", "MQTT already configured, skipping setup");
        return;
    }

    // Load CA certificate
    File certFile = LittleFS.open("/resources/isrg-root-x1.pem", "r");
    if (!certFile) {
        LOG_ERROR("MQTT", "Failed to open CA certificate file");
        return;
    }

    String certContent = certFile.readString();
    certFile.close();

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

    LOG_VERBOSE("MQTT", "CA certificate loaded and validated (%d bytes)", certContent.length());

    // Store in buffer to ensure lifetime during connection attempts
    caCertificateBuffer = certContent;

    // Configure ESP32MQTTClient with TLS
    const RuntimeConfig &config = getRuntimeConfig();

    // Set broker URL (port 8883 automatically uses mqtts://)
    mqttClient.setURL(config.mqttServer.c_str(), config.mqttPort,
                      config.mqttUsername.c_str(), config.mqttPassword.c_str());

    // Set CA certificate for TLS
    mqttClient.setCaCert(caCertificateBuffer.c_str());

    // Set buffer size for large messages
    mqttClient.setMaxPacketSize(mqttBufferSize);

    // Set keepalive timeout (30 seconds default)
    mqttClient.setKeepAlive(30);

    LOG_VERBOSE("MQTT", "ESP32MQTTClient configured: %s:%d | Inbox topic: %s | TLS: Enabled | Buffer: %d bytes",
                config.mqttServer.c_str(), config.mqttPort, getLocalPrinterTopic(), mqttBufferSize);

    // Mark setup as completed
    mqttSetupCompleted = true;
}

// Internal helper: connectToMQTTInternal (mutex already held)
void MQTTManager::connectToMQTTInternal()
{
    // State machine handles duplicate prevention
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

    // Get printer ID for client ID and LWT
    String printerId = getPrinterId();
    String clientId = "ScribePrinter-" + printerId;
    mqttClient.setMqttClientName(clientId.c_str());

    // Set up Last Will and Testament (LWT) for printer discovery
    String statusTopic = MqttTopics::buildStatusTopic(printerId);
    String lwtPayload = createOfflinePayload();
    mqttClient.enableLastWillMessage(statusTopic.c_str(), lwtPayload.c_str(), true);

    LOG_VERBOSE("MQTT", "MQTT client configured: ID=%s, LWT=%s", clientId.c_str(), statusTopic.c_str());

    // Start MQTT client (non-blocking!)
    // This creates a separate FreeRTOS task for MQTT operations
    if (mqttLoopStarted) {
        LOG_WARNING("MQTT", "loopStart already called, skipping duplicate call");
        // Connection will be handled by onMqttConnect callback when ready
        return;
    }

    LOG_VERBOSE("MQTT", "MQTT connecting asynchronously (loopStart)");
    mqttClient.loopStart();
    mqttLoopStarted = true;

    // Note: State will be updated to MQTT_STATE_CONNECTED by onConnectionEstablished callback
    // No need to handle connection success/failure here - it's all event-driven now
}

// Internal helper: handleMQTTConnectionInternal (mutex already held)
void MQTTManager::handleMQTTConnectionInternal()
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
                mqttState = MQTT_STATE_CONNECTING;
                stateChangeTime = millis();

                // Setup MQTT if not already done, then connect
                if (WiFi.status() == WL_CONNECTED)
                {
                    setupMQTTInternal();  // Load certs, configure client
                    connectToMQTTInternal();  // Start non-blocking connection
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
            // With ESP32MQTTClient, connection happens asynchronously
            // onMqttConnect callback will update state to CONNECTED when ready
            // Just check for timeout here
            if (millis() - stateChangeTime > mqttConnectionTimeoutMs)
            {
                LOG_ERROR("MQTT", "Connection timeout after %lums", mqttConnectionTimeoutMs);
                mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
                consecutiveFailures++;
                lastFailureTime = millis();

                // Disable auto-reconnect to stop connection attempts
                if (mqttLoopStarted) {
                    mqttClient.disableAutoReconnect();
                    mqttLoopStarted = false;
                    LOG_VERBOSE("MQTT", "Disabled auto-reconnect after timeout");
                }
            }
            break;

        case MQTT_STATE_CONNECTED:
            // ESP32MQTTClient handles message processing in its own task
            // No need to call loop() - it's non-blocking!

            // Check if still connected (ESP32MQTTClient provides isConnected())
            if (!mqttClient.isConnected())
            {
                LOG_WARNING("MQTT", "Connection lost");
                mqttState = MQTT_STATE_ENABLED_DISCONNECTED;
                mqttLoopStarted = false; // Reset flag for next connection attempt
            }
            break;

        case MQTT_STATE_DISCONNECTING:
            // Transitional state - should be brief
            break;
    }
}

// Internal helper: updateMQTTSubscriptionInternal (mutex already held)
void MQTTManager::updateMQTTSubscriptionInternal()
{
    if (!mqttClient.isConnected())
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
        std::string oldTopicStd = currentSubscribedTopic.c_str();
        if (mqttClient.unsubscribe(oldTopicStd))
        {
            LOG_NOTICE("MQTT", "Unsubscribed from old topic: %s", currentSubscribedTopic.c_str());
        }
        else
        {
            LOG_WARNING("MQTT", "Failed to unsubscribe from old topic: %s", currentSubscribedTopic.c_str());
        }
    }

    // Subscribe to new topic with callback
    std::string newTopicStd = newTopic.c_str();
    if (mqttClient.subscribe(newTopicStd, onMqttMessageCallback, 0))
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

// ============================================================================
// PUBLIC METHODS (THREAD-SAFE WITH MUTEX)
// ============================================================================

// Public method: handleConnection (thread-safe)
void MQTTManager::handleConnection()
{
    if (!initialized) {
        LOG_ERROR("MQTT", "MQTTManager not initialized - call begin() first!");
        return;
    }

    bool shouldPublish = false;

    // Nested scope to ensure mutex is released before publishPrinterStatus()
    {
        // Acquire mutex using RAII
        ManagerLock lock(mutex, "MQTT");
        if (!lock.isLocked()) {
            LOG_ERROR("MQTT", "Failed to acquire MQTT mutex!");
            return;
        }

        handleMQTTConnectionInternal();

        // Check if we need to publish status (set by connectToMQTTInternal on successful connection)
        shouldPublish = needPublishStatus;
        if (shouldPublish) {
            needPublishStatus = false;  // Clear flag before releasing mutex
        }

        // Mutex released by ManagerLock destructor when this scope exits
    }

    // Publish status AFTER releasing mutex to avoid deadlock
    if (shouldPublish) {
        LOG_VERBOSE("MQTT", "Publishing initial online status after connection");
        publishPrinterStatus();

        // Feed watchdog after potentially long MQTT connection and publish sequence
        esp_task_wdt_reset();
    }
}

// Public method: updateSubscription (thread-safe)
void MQTTManager::updateSubscription()
{
    if (!initialized) {
        LOG_ERROR("MQTT", "MQTTManager not initialized - call begin() first!");
        return;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "MQTT");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire MQTT mutex!");
        return;
    }

    updateMQTTSubscriptionInternal();

    // Mutex automatically released by ManagerLock destructor
}

// Public method: publishMessage (thread-safe)
bool MQTTManager::publishMessage(const String& topic, const String& header, const String& body)
{
    if (!initialized) {
        LOG_ERROR("MQTT", "MQTTManager not initialized - call begin() first!");
        return false;
    }

    // Validate inputs
    if (topic.length() == 0) {
        LOG_ERROR("MQTT", "publishMQTTMessage: topic cannot be empty");
        return false;
    }

    if (header.length() == 0) {
        LOG_ERROR("MQTT", "publishMQTTMessage: header cannot be empty");
        return false;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "MQTT");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire MQTT mutex!");
        return false;
    }

    // Check if MQTT is enabled and connected
    if (!isEnabled()) {
        LOG_WARNING("MQTT", "MQTT is disabled, cannot publish to topic: %s", topic.c_str());
        return false;
    }

    if (!mqttClient.isConnected()) {
        LOG_WARNING("MQTT", "MQTT not connected, cannot publish to topic: %s", topic.c_str());
        return false;
    }

    // Create standardized JSON payload
    JsonDocument payloadDoc;
    payloadDoc["header"] = header;
    payloadDoc["body"] = body;
    payloadDoc["timestamp"] = getFormattedDateTime();

    // Add sender information (device owner)
    const RuntimeConfig &config = getRuntimeConfig();
    if (config.deviceOwner.length() > 0) {
        payloadDoc["sender"] = config.deviceOwner;
    }

    // Serialize payload
    String payload;
    serializeJson(payloadDoc, payload);

    // Publish message (convert to std::string)
    std::string topicStd = topic.c_str();
    std::string payloadStd = payload.c_str();
    bool success = mqttClient.publish(topicStd, payloadStd, 0, false);

    if (success) {
        LOG_VERBOSE("MQTT", "Published message to topic: %s (%d characters)",
                   topic.c_str(), payload.length());
    } else {
        LOG_ERROR("MQTT", "Failed to publish message to topic: %s", topic.c_str());
    }

    // Mutex automatically released by ManagerLock destructor
    return success;
}

// Internal method: publishRawMessageInternal (mutex already held)
bool MQTTManager::publishRawMessageInternal(const String& topic, const String& payload, bool retained)
{
    // Validate inputs
    if (topic.length() == 0) {
        LOG_ERROR("MQTT", "publishRawMessageInternal: topic cannot be empty");
        return false;
    }

    if (payload.length() == 0) {
        LOG_ERROR("MQTT", "publishRawMessageInternal: payload cannot be empty");
        return false;
    }

    // Check if MQTT is enabled and connected
    if (!isEnabled()) {
        LOG_WARNING("MQTT", "MQTT is disabled, cannot publish to topic: %s", topic.c_str());
        return false;
    }

    if (!mqttClient.isConnected()) {
        LOG_WARNING("MQTT", "MQTT not connected, cannot publish to topic: %s", topic.c_str());
        return false;
    }

    // Publish raw message with optional retained flag
    // ESP32MQTTClient publish signature: publish(topic, payload, qos, retain)
    // Convert Arduino String to std::string
    std::string topicStd = topic.c_str();
    std::string payloadStd = payload.c_str();
    int qos = 0; // QoS 0 = at most once
    bool success = mqttClient.publish(topicStd, payloadStd, qos, retained);

    // Note: Success is logged by the caller (e.g., publishPrinterStatus) for better context
    if (!success) {
        LOG_ERROR("MQTT", "Failed to publish raw message to topic: %s", topic.c_str());
    }

    return success;
}

// Public method: publishRawMessage (thread-safe)
bool MQTTManager::publishRawMessage(const String& topic, const String& payload, bool retained)
{
    if (!initialized) {
        LOG_ERROR("MQTT", "MQTTManager not initialized - call begin() first!");
        return false;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "MQTT");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire MQTT mutex!");
        return false;
    }

    // Call internal version (mutex already held)
    bool success = publishRawMessageInternal(topic, payload, retained);

    // Mutex automatically released by ManagerLock destructor
    return success;
}

// Public method: isEnabled
bool MQTTManager::isEnabled()
{
    const RuntimeConfig &config = getRuntimeConfig();
    return config.mqttEnabled;
}

// Public method: startClient (thread-safe)
void MQTTManager::startClient(bool immediate)
{
    if (!initialized) {
        LOG_ERROR("MQTT", "MQTTManager not initialized - call begin() first!");
        return;
    }

    if (!isEnabled())
    {
        LOG_VERBOSE("MQTT", "MQTT is disabled in config, not starting client");
        return;
    }

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "MQTT");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire MQTT mutex!");
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

    // Mutex automatically released by ManagerLock destructor
}

// Public method: stopClient (thread-safe)
void MQTTManager::stopClient()
{
    if (!initialized) {
        LOG_ERROR("MQTT", "MQTTManager not initialized - call begin() first!");
        return;
    }

    LOG_NOTICE("MQTT", "Stopping MQTT client");

    // Acquire mutex using RAII
    ManagerLock lock(mutex, "MQTT");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire MQTT mutex!");
        return;
    }

    mqttState = MQTT_STATE_DISCONNECTING;

    // Disable auto-reconnect so client doesn't try to reconnect
    if (mqttLoopStarted)
    {
        mqttClient.disableAutoReconnect();
        mqttLoopStarted = false;
        LOG_VERBOSE("MQTT", "MQTT auto-reconnect disabled, client will disconnect");
    }

    // Reset ALL state variables
    mqttState = MQTT_STATE_DISABLED;
    currentSubscribedTopic = "";
    consecutiveFailures = 0;
    lastMQTTReconnectAttempt = 0;
    lastFailureTime = 0;
    mqttSetupCompleted = false; // Allow reconfiguration on next start

    // Mutex automatically released by ManagerLock destructor
}

// Public method: isConnected
bool MQTTManager::isConnected()
{
    if (!initialized) {
        return false;
    }

    // Acquire mutex for safe access using RAII
    ManagerLock lock(mutex, "MQTT");
    if (!lock.isLocked()) {
        LOG_ERROR("MQTT", "Failed to acquire MQTT mutex!");
        return false;
    }

    bool connected = mqttClient.isConnected();

    // Mutex automatically released by ManagerLock destructor
    return connected;
}
