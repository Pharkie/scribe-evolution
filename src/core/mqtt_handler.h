#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <ESP32MQTTClient.h>
#include <ArduinoJson.h>
#include <config/config.h>

// Forward declare global event handlers required by ESP32MQTTClient
void onMqttConnect(esp_mqtt_client_handle_t client);
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event);
#else
void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
#endif

/**
 * @class MQTTManager
 * @brief Thread-safe singleton for MQTT operations
 *
 * Provides mutex-protected access to ESP32MQTTClient for non-blocking
 * MQTT operations. Uses event-driven architecture via ESP-IDF's esp-mqtt.
 *
 * Thread Safety:
 * - All operations (publish, connect, disconnect, state machine) use mutex protection
 * - Safe for concurrent access from AsyncWebServer, buttons, and main loop
 *
 * Non-Blocking Architecture:
 * - loopStart() runs MQTT in separate FreeRTOS task - no watchdog timeouts
 * - Event callbacks handle connection/message events asynchronously
 *
 * Usage:
 *   MQTTManager::instance().begin();  // Call once in setup()
 *   MQTTManager::instance().publishMessage(topic, header, body);  // Thread-safe publish
 *   MQTTManager::instance().handleConnection();  // Call from loop()
 */
class MQTTManager
{
public:
    static MQTTManager& instance();

    /**
     * @brief Initialize MQTTManager (creates mutex and MQTT clients)
     * Must be called once in setup() before any MQTT operations
     */
    void begin();

    /**
     * @brief Handle MQTT connection state machine (thread-safe)
     * Call from main loop() to process MQTT messages and handle reconnection
     */
    void handleConnection();

    /**
     * @brief Update MQTT subscription to new topic (thread-safe)
     */
    void updateSubscription();

    /**
     * @brief Publish MQTT message (thread-safe)
     * @param topic MQTT topic to publish to
     * @param header Message header
     * @param body Message body
     * @return true if message was published successfully
     */
    bool publishMessage(const String& topic, const String& header, const String& body);

    /**
     * @brief Publish raw MQTT message (thread-safe)
     * For non-print messages like discovery/status that need custom payload format
     * @param topic MQTT topic to publish to
     * @param payload Raw payload (already serialized JSON or plain text)
     * @param retained If true, message is retained by broker for new subscribers
     * @return true if message was published successfully
     */
    bool publishRawMessage(const String& topic, const String& payload, bool retained = false);

    /**
     * @brief Check if MQTT is enabled in config
     * @return true if MQTT is enabled
     */
    bool isEnabled();

    /**
     * @brief Start MQTT client (thread-safe)
     * @param immediate If true, connect immediately; if false, wait normal interval
     */
    void startClient(bool immediate = true);

    /**
     * @brief Stop MQTT client and disconnect (thread-safe)
     */
    void stopClient();

    /**
     * @brief Check if MQTT client is connected
     * @return true if connected to broker
     */
    bool isConnected();

private:
    MQTTManager();
    ~MQTTManager() = default;
    MQTTManager(const MQTTManager&) = delete;
    MQTTManager& operator=(const MQTTManager&) = delete;

    // Internal helpers (mutex already held by caller)
    void setupMQTTInternal();
    void connectToMQTTInternal();
    void handleMQTTMessageInternal(String topic, String message);
    void updateMQTTSubscriptionInternal();
    void handleMQTTConnectionInternal();
    bool publishRawMessageInternal(const String& topic, const String& payload, bool retained = false);

public:
    // Event callbacks (called by global handlers - must be public)
    void onConnectionEstablished();
    void onMessageReceived(String topic, String message);

private:

    // MQTT state machine
    enum MQTTState {
        MQTT_STATE_DISABLED,
        MQTT_STATE_ENABLED_DISCONNECTED,
        MQTT_STATE_CONNECTING,
        MQTT_STATE_CONNECTED,
        MQTT_STATE_DISCONNECTING
    };

    SemaphoreHandle_t mutex = nullptr;
    bool initialized = false;

    // MQTT client (encapsulated in singleton)
    ESP32MQTTClient mqttClient;

    // MQTT state
    MQTTState mqttState = MQTT_STATE_DISABLED;
    unsigned long stateChangeTime = 0;
    unsigned long lastMQTTReconnectAttempt = 0;
    int consecutiveFailures = 0;
    unsigned long lastFailureTime = 0;
    String currentSubscribedTopic = "";
    String caCertificateBuffer = "";
    bool mqttSetupCompleted = false;
    bool needPublishStatus = false; // Flag to publish status after mutex release
    bool mqttLoopStarted = false; // Track if loopStart() has been called

    // Friend declarations for global event handlers
    friend void ::onMqttConnect(esp_mqtt_client_handle_t client);
    #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
    friend esp_err_t ::handleMQTT(esp_mqtt_event_handle_t event);
    #else
    friend void ::handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    #endif
};

// ============================================================================
// BACKWARD-COMPATIBLE WRAPPER FUNCTIONS
// ============================================================================

/**
 * @brief Handle MQTT connection state machine
 * Wrapper for MQTTManager::instance().handleConnection()
 */
inline void handleMQTTConnection() {
    MQTTManager::instance().handleConnection();
}

/**
 * @brief Update MQTT subscription to new topic
 * Wrapper for MQTTManager::instance().updateSubscription()
 */
inline void updateMQTTSubscription() {
    MQTTManager::instance().updateSubscription();
}

/**
 * @brief Publish MQTT message
 * Wrapper for MQTTManager::instance().publishMessage()
 */
inline bool publishMQTTMessage(const String& topic, const String& header, const String& body) {
    return MQTTManager::instance().publishMessage(topic, header, body);
}

/**
 * @brief Check if MQTT is enabled in config
 * Wrapper for MQTTManager::instance().isEnabled()
 */
inline bool isMQTTEnabled() {
    return MQTTManager::instance().isEnabled();
}

/**
 * @brief Start MQTT client
 * Wrapper for MQTTManager::instance().startClient()
 */
inline void startMQTTClient(bool immediate = true) {
    MQTTManager::instance().startClient(immediate);
}

/**
 * @brief Stop MQTT client
 * Wrapper for MQTTManager::instance().stopClient()
 */
inline void stopMQTTClient() {
    MQTTManager::instance().stopClient();
}

#endif // MQTT_HANDLER_H
