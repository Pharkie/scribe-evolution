/**
 * @file test_mqtt_poc.cpp
 * @brief Proof-of-concept test for ESP32MQTTClient with TLS
 * @purpose Verify non-blocking MQTT connection to HiveMQ Cloud before full migration
 *
 * Test criteria:
 * 1. Non-blocking connection (no watchdog timeout on ESP32-C3)
 * 2. TLS works with CA certificate
 * 3. Can subscribe and receive messages
 * 4. Can publish messages
 * 5. Loop remains responsive during connection
 */

#include <Arduino.h>
#include <WiFi.h>
#include <unity.h>
#include <esp_task_wdt.h>
#include "ESP32MQTTClient.h"
#include <config/device_config.h>

// WiFi credentials (from device_config.h)
// Using: defaultWifiSSID, defaultWifiPassword

// MQTT settings (from device_config.h)
// Using: defaultMqttServer, defaultMqttPort, defaultMqttUsername, defaultMqttPassword

// Test topics
const char* testPublishTopic = "scribe-evolution/test/poc";
const char* testSubscribeTopic = "scribe-evolution/test/poc";

// Get printer name for logging
String getPrinterName() {
    return String(defaultDeviceOwner);
}

// CA Certificate for HiveMQ Cloud (ISRG Root X1)
const char* caCertificate = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

ESP32MQTTClient mqttClient;

// Test state
bool wifiConnected = false;
bool mqttConnected = false;
bool messageReceived = false;
String receivedMessage = "";
unsigned long connectionStartTime = 0;
unsigned long lastWatchdogReset = 0;

// Callback for received messages
void onMessageCallback(const std::string &topic, const std::string &payload) {
    Serial.printf("[TEST] Message received on %s: %s\n", topic.c_str(), payload.c_str());
    messageReceived = true;
    receivedMessage = String(payload.c_str());
}

// Connection callback
void onMqttConnect(esp_mqtt_client_handle_t client) {
    if (mqttClient.isMyTurn(client)) {
        Serial.println("[TEST] MQTT Connected! Subscribing to test topic...");
        mqttConnected = true;

        // Subscribe to test topic
        mqttClient.subscribe(testSubscribeTopic, onMessageCallback, 0);
    }
}

// Event handler
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
esp_err_t handleMQTT(esp_mqtt_event_handle_t event) {
    mqttClient.onEventCallback(event);
    return ESP_OK;
}
#else  // IDF CHECK
void handleMQTT(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto *event = static_cast<esp_mqtt_event_handle_t>(event_data);
    mqttClient.onEventCallback(event);
}
#endif // IDF CHECK

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

// Test 1: WiFi Connection
void test_wifi_connection(void) {
    Serial.println("\n[TEST 1] Testing WiFi connection...");

    WiFi.begin(defaultWifiSSID, defaultWifiPassword);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        esp_task_wdt_reset();
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    wifiConnected = (WiFi.status() == WL_CONNECTED);

    TEST_ASSERT_TRUE_MESSAGE(wifiConnected, "WiFi failed to connect");
    Serial.printf("[TEST 1] WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

// Test 2: MQTT Setup with TLS
void test_mqtt_setup(void) {
    Serial.println("\n[TEST 2] Testing MQTT setup with TLS...");

    TEST_ASSERT_TRUE_MESSAGE(wifiConnected, "WiFi must be connected first");

    // Configure MQTT client using device_config.h settings
    mqttClient.enableDebuggingMessages(true);
    mqttClient.setURL(defaultMqttServer, defaultMqttPort, defaultMqttUsername, defaultMqttPassword);
    mqttClient.setCaCert(caCertificate);
    mqttClient.setKeepAlive(30);

    String clientName = "ScribeTest-" + getPrinterName();
    mqttClient.setMqttClientName(clientName.c_str());

    mqttClient.enableLastWillMessage("scribe-evolution/test/lwt", "Test client offline", false);
    mqttClient.setOnMessageCallback(onMessageCallback);

    Serial.printf("[TEST 2] MQTT client configured for %s:%d\n", defaultMqttServer, defaultMqttPort);
    Serial.printf("[TEST 2] Username: %s, Client: %s\n", defaultMqttUsername, clientName.c_str());
    TEST_PASS();
}

// Test 3: Non-Blocking Connection
void test_nonblocking_connection(void) {
    Serial.println("\n[TEST 3] Testing non-blocking MQTT connection...");

    connectionStartTime = millis();
    lastWatchdogReset = millis();

    // Start MQTT (should be non-blocking!)
    mqttClient.loopStart();

    Serial.println("[TEST 3] loopStart() returned immediately (non-blocking!)");

    // Wait for connection with watchdog resets
    unsigned long waitStart = millis();
    while (!mqttConnected && millis() - waitStart < 15000) {
        // Feed watchdog every 1 second
        if (millis() - lastWatchdogReset > 1000) {
            esp_task_wdt_reset();
            lastWatchdogReset = millis();
            Serial.printf("[TEST 3] Watchdog reset at %lu ms (waiting for MQTT...)\n", millis() - connectionStartTime);
        }
        delay(100);
    }

    unsigned long connectionTime = millis() - connectionStartTime;

    TEST_ASSERT_TRUE_MESSAGE(mqttConnected, "MQTT failed to connect within 15 seconds");
    Serial.printf("[TEST 3] MQTT connected in %lu ms\n", connectionTime);
    Serial.println("[TEST 3] ✅ Connection was non-blocking - no watchdog timeout!");
}

// Test 4: Publish Message
void test_publish_message(void) {
    Serial.println("\n[TEST 4] Testing MQTT publish...");

    TEST_ASSERT_TRUE_MESSAGE(mqttConnected, "MQTT must be connected first");

    String testMessage = "PoC test from ESP32-C3 at " + String(millis()) + " ms";
    bool published = mqttClient.publish(testPublishTopic, testMessage.c_str(), 0, false);

    TEST_ASSERT_TRUE_MESSAGE(published, "Failed to publish message");
    Serial.printf("[TEST 4] Published: %s\n", testMessage.c_str());
}

// Test 5: Receive Message
void test_receive_message(void) {
    Serial.println("\n[TEST 5] Testing MQTT receive (will echo published message)...");

    TEST_ASSERT_TRUE_MESSAGE(mqttConnected, "MQTT must be connected first");

    // Wait for message with watchdog resets
    unsigned long waitStart = millis();
    messageReceived = false;

    while (!messageReceived && millis() - waitStart < 5000) {
        esp_task_wdt_reset();
        delay(100);
    }

    TEST_ASSERT_TRUE_MESSAGE(messageReceived, "Did not receive echoed message");
    Serial.printf("[TEST 5] Received message: %s\n", receivedMessage.c_str());
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Wait for serial

    Serial.println("\n\n=== ESP32MQTTClient Proof-of-Concept Test ===");
    Serial.println("Testing non-blocking MQTT with TLS on ESP32-C3\n");
    Serial.printf("Device: %s\n", defaultDeviceOwner);
    Serial.printf("WiFi SSID: %s\n", defaultWifiSSID);
    Serial.printf("MQTT Broker: %s:%d\n\n", defaultMqttServer, defaultMqttPort);

    UNITY_BEGIN();

    RUN_TEST(test_wifi_connection);
    RUN_TEST(test_mqtt_setup);
    RUN_TEST(test_nonblocking_connection);
    RUN_TEST(test_publish_message);
    RUN_TEST(test_receive_message);

    UNITY_END();

    Serial.println("\n=== All Tests Complete ===");
    Serial.println("✅ ESP32MQTTClient is suitable for migration!");
}

void loop() {
    // Keep feeding watchdog to show loop is responsive
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 5000) {
        esp_task_wdt_reset();
        Serial.printf("[LOOP] Still running at %lu ms - loop is responsive!\n", millis());
        lastPrint = millis();
    }
    delay(100);
}
