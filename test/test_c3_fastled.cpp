/**
 * ESP32-C3 FastLED + AsyncWebServer + MQTT Crash Test
 *
 * Purpose: Incrementally add components to find what causes FastLED crash
 *
 * Test phases:
 * 1. FastLED alone ✓ (works)
 * 2. + AsyncWebServer ✓ (works)
 * 3. + MQTT client (testing now)
 *
 * Expected behavior:
 * - Find which combination causes ESP32-C3 to crash
 *
 * Hardware:
 * - ESP32-C3-mini or ESP32-S3-mini
 * - WS2812B LED strip connected to GPIO 20 (C3) or GPIO 1 (S3)
 * - Common ground
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <FastLED.h>

// Board-specific GPIO configuration
#ifdef BOARD_ESP32C3_MINI
  #define LED_PIN 20
  #define BOARD_NAME "ESP32-C3-mini"
#elif defined(BOARD_ESP32S3_MINI)
  #define LED_PIN 1
  #define BOARD_NAME "ESP32-S3-mini"
#else
  #define LED_PIN 1
  #define BOARD_NAME "Unknown"
#endif

#define NUM_LEDS 30
#define BRIGHTNESS 100
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// MQTT credentials from device_config.h
#include "../src/config/device_config.h"

CRGB leds[NUM_LEDS];
AsyncWebServer server(80);
WiFiClientSecure wifiSecureClient;
PubSubClient mqttClient(wifiSecureClient);

bool webServerStarted = false;
bool mqttConnected = false;

// ============================================================================
// SimplePrinterManager - mimics UART writes to thermal printer
// ============================================================================

class SimplePrinterManager {
private:
  HardwareSerial* uart;
  bool initialized;

  // Private constructor for singleton
  SimplePrinterManager() : uart(nullptr), initialized(false) {}

  // Delete copy constructor and assignment
  SimplePrinterManager(const SimplePrinterManager&) = delete;
  SimplePrinterManager& operator=(const SimplePrinterManager&) = delete;

public:
  static SimplePrinterManager& instance() {
    static SimplePrinterManager instance;
    return instance;
  }

  bool begin(int txPin, int rxPin) {
    if (initialized) return true;

    // Initialize UART1 for printer (like real PrinterManager)
    uart = new HardwareSerial(1);
    uart->begin(19200, SERIAL_8N1, rxPin, txPin);  // 19200 baud for thermal printer

    initialized = true;
    Serial.printf("[SimplePrinterManager] UART1 initialized (TX: %d, RX: %d, 19200 baud)\n", txPin, rxPin);
    return true;
  }

  void printTestMessage(const char* message) {
    if (!uart || !initialized) return;

    Serial.printf("[SimplePrinterManager] Printing: %s\n", message);

    // Simulate thermal printer commands (blocking UART writes)
    uart->write(0x1B);  // ESC
    uart->write(0x40);  // Initialize printer
    delay(10);

    // Write text (BLOCKING operation - could interfere with FastLED timing)
    uart->print(message);
    uart->write('\n');
    uart->write('\n');

    // Feed paper
    uart->write(0x1B);  // ESC
    uart->write(0x64);  // d
    uart->write(0x03);  // Feed 3 lines
    uart->flush();      // Wait for UART TX to complete (BLOCKING)

    Serial.printf("[SimplePrinterManager] Print complete\n");
  }
};

void setup() {
  // Initialize serial for logging
  Serial.begin(115200);
  delay(2000);  // Wait for serial to be ready

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("ESP32-C3 FastLED + Async + MQTT Test");
  Serial.println("========================================");
  Serial.printf("Board: %s\n", BOARD_NAME);
  Serial.printf("LED Pin: GPIO %d\n", LED_PIN);
  Serial.printf("LED Count: %d\n", NUM_LEDS);
  Serial.printf("Brightness: %d\n", BRIGHTNESS);
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("========================================");
  Serial.println();

  // Connect to WiFi (needed for MQTT)
  Serial.printf("[0/7] Connecting to WiFi: %s\n", defaultWifiSSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(defaultWifiSSID, defaultWifiPassword);

  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[0/7] ✓ WiFi connected - IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("[0/7] ✗ WiFi connection failed - MQTT will not work");
  }
  Serial.println();

  // Initialize PrinterManager FIRST (like main app does) - TESTING ORDER DEPENDENCY
  Serial.println("[0.5/7] Initializing SimplePrinterManager BEFORE FastLED...");
  if (!SimplePrinterManager::instance().begin(21, -1)) {  // TX=21, RX=not used
    Serial.println("[0.5/7] ✗ SimplePrinterManager initialization failed!");
  } else {
    Serial.println("[0.5/7] ✓ SimplePrinterManager initialized (UART1) BEFORE FastLED");
  }
  Serial.println();

  // Initialize FastLED
  Serial.println("[1/5] Initializing FastLED...");
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.println("[1/5] ✓ FastLED.addLeds() succeeded");

  // Clear all LEDs (set to black)
  Serial.println("[2/5] Clearing LEDs (all black)...");
  FastLED.clear();
  Serial.println("[2/5] ✓ FastLED.clear() succeeded");

  // First FastLED.show() call - THIS IS WHERE C3 CRASHES
  Serial.println("[3/5] Calling FastLED.show() for the first time...");
  Serial.println("[3/5] If ESP32-C3 has FastLED bug, crash will happen here:");
  Serial.flush();  // Make sure this message is printed before crash

  FastLED.show();  // ← EXPECTED CRASH POINT ON ESP32-C3

  Serial.println("[3/5] ✓ FastLED.show() succeeded (no crash!)");

  // If we get here, FastLED.show() worked
  Serial.println("[4/5] Setting LEDs to blue...");
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Blue;
  }
  Serial.println("[4/5] ✓ LED array updated");

  Serial.println("[5/5] Calling FastLED.show() again...");
  Serial.flush();

  FastLED.show();

  Serial.println("[5/6] ✓ FastLED.show() succeeded again!");
  Serial.println();

  // PrinterManager already initialized BEFORE FastLED (testing order dependency)
  // Print test message while LED effect is running (CRITICAL TEST)
  Serial.println("[5.5/7] Printing test message while LEDs are active...");
  SimplePrinterManager::instance().printTestMessage("BOOT TEST - FastLED active");
  Serial.println();

  // NEW TEST: Call pinMode() AFTER FastLED to see if it breaks RMT
  Serial.println("[5.75/7] TESTING: Calling pinMode() on button GPIOs AFTER FastLED...");
  pinMode(5, INPUT_PULLUP);  // Button 1
  pinMode(6, INPUT_PULLUP);  // Button 2
  pinMode(7, INPUT_PULLUP);  // Button 3
  pinMode(4, INPUT_PULLUP);  // Button 4
  Serial.println("[5.75/7] ✓ pinMode() calls completed");
  Serial.println();

  // NOW START ASYNCWEBSERVER - this is the suspected culprit
  Serial.println("[6/6] Starting AsyncWebServer...");

  // Add a simple test route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello from ESP32-C3!");
  });

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Test endpoint working");
  });

  server.begin();
  webServerStarted = true;

  Serial.println("[6/6] ✓ AsyncWebServer started (no WiFi - testing local task interference)");
  Serial.println();

  // Connect to MQTT broker
  Serial.println("[7/7] Connecting to MQTT broker...");
  Serial.printf("[7/7]   Server: %s:%d\n", defaultMqttServer, defaultMqttPort);
  wifiSecureClient.setInsecure(); // Skip certificate validation for testing
  mqttClient.setServer(defaultMqttServer, defaultMqttPort);
  mqttClient.setBufferSize(512);

  if (mqttClient.connect("ESP32-C3-Test", defaultMqttUsername, defaultMqttPassword)) {
    Serial.println("[7/7] ✓ MQTT connected!");
    mqttConnected = true;
    mqttClient.subscribe("test/c3");
  } else {
    Serial.printf("[7/7] ✗ MQTT connection failed (state: %d)\n", mqttClient.state());
    Serial.println("[7/7]   Continuing without MQTT...");
  }
  Serial.println();

  Serial.println("========================================");
  Serial.println("PHASE 1 COMPLETE: All initialization OK");
  Serial.println("Components running:");
  Serial.println("  - FastLED");
  Serial.println("  - SimplePrinterManager (UART1 @ 19200 baud)");
  Serial.println("  - AsyncWebServer");
  Serial.printf("  - MQTT: %s\n", mqttConnected ? "CONNECTED" : "disconnected");
  Serial.println();
  Serial.println("PHASE 2 STARTING: Running LED effects");
  Serial.println();
  Serial.println("If crash happens, it will be in loop()");
  Serial.println("when FastLED.show() is called while");
  Serial.println("all components are active.");
  Serial.println("========================================");
  Serial.println();
}

void loop() {
  // Keep MQTT connection alive
  if (mqttConnected) {
    if (!mqttClient.connected()) {
      Serial.println("MQTT disconnected, attempting reconnect...");
      if (mqttClient.connect("ESP32-C3-Test", defaultMqttUsername, defaultMqttPassword)) {
        Serial.println("MQTT reconnected!");
        mqttClient.subscribe("test/c3");
      }
    }
    mqttClient.loop();  // Process MQTT messages
  }

  // Simple chase effect to keep testing FastLED.show()
  static int pos = 0;
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 50) {  // Update every 50ms
    // Clear all
    FastLED.clear();

    // Set current position to green
    leds[pos] = CRGB::Green;

    // Show - THIS IS WHERE CRASH IS EXPECTED if component interaction causes issues
    FastLED.show();  // ← EXPECTED CRASH HERE if MQTT/AsyncWebServer interfere

    // Move to next position
    pos = (pos + 1) % NUM_LEDS;
    lastUpdate = millis();

    // Print status every 10 seconds
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint > 10000) {
      Serial.printf("✓ Still running... uptime: %lu sec, free heap: %d, MQTT: %s\n",
                    millis() / 1000, ESP.getFreeHeap(),
                    mqttClient.connected() ? "connected" : "disconnected");
      lastStatusPrint = millis();
    }
  }
}
