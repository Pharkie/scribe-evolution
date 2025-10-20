/**
 * ESP32-C3 MAXIMAL STRESS TEST - USING REAL LED & LOG MANAGERS
 *
 * Purpose: Test ESP32-C3 with REAL LedEffects and REAL LogManager from main app
 *
 * Components (ALL ACTIVE):
 * 1. FastLED - WS2812B LED strip
 * 2. **REAL LedEffects** - Full EffectRegistry + EffectBase system
 * 3. **REAL LogManager** - FreeRTOS queue + dedicated high-priority writer task
 * 4. SimpleConfigManager - Mutex-protected NVS read/write operations
 * 5. SimplePrinterManager - UART1 operations
 * 6. AsyncWebServer - Multiple routes with heavy concurrent operations
 * 7. MQTT - TLS connection with PubSubClient
 * 8. WiFi/NTP - ezTime with timezone sync
 * 9. LittleFS - Filesystem operations
 * 10. Button interrupts - 4 GPIO interrupts (FALLING edge)
 * 11. Watchdog timer - 8 second timeout
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <ezTime.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <Preferences.h>

// Include REAL LED system and LogManager
#include <core/logging.h>
#include <leds/LedEffects.h>
#include <leds/effects/EffectRegistry.h>

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
// Use REAL LED system from main app (singleton pattern)
// ============================================================================

// LedEffects is accessed via ledEffects() helper function (singleton)


// ============================================================================
// Simplified PrinterManager - mimics UART writes to thermal printer
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
    LogManager::instance().logf("[SimplePrinterManager] UART1 initialized (TX: %d, RX: %d, 19200 baud)\n", txPin, rxPin);
    return true;
  }

  void printTestMessage(const char* message) {
    if (!uart || !initialized) return;

    LogManager::instance().logf("[SimplePrinterManager] Printing: %s\n", message);

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

    LogManager::instance().logf("[SimplePrinterManager] Print complete\n");
  }
};

// ============================================================================
// Button Interrupt Handlers - mimics hardware_buttons.cpp
// ============================================================================

volatile unsigned long lastButtonPress[4] = {0, 0, 0, 0};
const unsigned long debounceDelay = 200; // ms

void IRAM_ATTR button1ISR() {
  unsigned long now = millis();
  if (now - lastButtonPress[0] > debounceDelay) {
    lastButtonPress[0] = now;
    // Trigger LED effect change via SimpleLedEffects
    // This simulates button press interfering with LED updates
  }
}

void IRAM_ATTR button2ISR() {
  unsigned long now = millis();
  if (now - lastButtonPress[1] > debounceDelay) {
    lastButtonPress[1] = now;
  }
}

void IRAM_ATTR button3ISR() {
  unsigned long now = millis();
  if (now - lastButtonPress[2] > debounceDelay) {
    lastButtonPress[2] = now;
  }
}

void IRAM_ATTR button4ISR() {
  unsigned long now = millis();
  if (now - lastButtonPress[3] > debounceDelay) {
    lastButtonPress[3] = now;
  }
}



// ============================================================================
// Simplified ConfigManager - mimics real ConfigManager NVS operations
// ============================================================================

class SimpleConfigManager {
private:
  SemaphoreHandle_t mutex;
  Preferences preferences;
  bool initialized;

  // Private constructor for singleton
  SimpleConfigManager() : mutex(nullptr), initialized(false) {}

  // Delete copy constructor and assignment
  SimpleConfigManager(const SimpleConfigManager&) = delete;
  SimpleConfigManager& operator=(const SimpleConfigManager&) = delete;

public:
  static SimpleConfigManager& instance() {
    static SimpleConfigManager instance;
    return instance;
  }

  bool begin() {
    if (initialized) return true;

    // Create mutex for thread-safe NVS operations
    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
      Serial.println("[SimpleConfigManager] Failed to create mutex!");
      return false;
    }

    initialized = true;
    Serial.println("[SimpleConfigManager] Initialized with mutex protection");
    return true;
  }

  bool saveTestConfig(const char* key, const char* value) {
    if (!initialized) return false;

    // Acquire mutex for thread-safe NVS write
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
      LogManager::instance().logf("[SimpleConfigManager] Mutex timeout in saveTestConfig!\n");
      return false;
    }

    // Perform NVS write (blocking operation)
    bool success = false;
    if (preferences.begin("scribe-test", false)) {  // false = read/write
      success = preferences.putString(key, value);
      preferences.end();
    }

    xSemaphoreGive(mutex);

    if (success) {
      LogManager::instance().logf("[SimpleConfigManager] Saved: %s = %s\n", key, value);
    } else {
      LogManager::instance().logf("[SimpleConfigManager] Failed to save: %s\n", key);
    }

    return success;
  }

  String loadTestConfig(const char* key, const char* defaultValue) {
    if (!initialized) return String(defaultValue);

    // Acquire mutex for thread-safe NVS read
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
      LogManager::instance().logf("[SimpleConfigManager] Mutex timeout in loadTestConfig!\n");
      return String(defaultValue);
    }

    // Perform NVS read (blocking operation)
    String value = defaultValue;
    if (preferences.begin("scribe-test", true)) {  // true = read-only
      value = preferences.getString(key, defaultValue);
      preferences.end();
    }

    xSemaphoreGive(mutex);

    LogManager::instance().logf("[SimpleConfigManager] Loaded: %s = %s\n", key, value.c_str());
    return value;
  }
};

void setup() {
  // Initialize serial for logging BEFORE LogManager
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

  // Initialize REAL LogManager EARLY (for LedEffects to use)
  Serial.println("[-2/9] Initializing REAL LogManager...");
  LogManager::instance().begin(115200, 256, 512);
  LOG_NOTICE("TEST", "[-2/9] ✓ REAL LogManager initialized (queue + writer task)");
  Serial.println();

  // Initialize ConfigManager (creates mutex for NVS operations)
  Serial.println("[-1.5/9] Initializing SimpleConfigManager...");
  if (!SimpleConfigManager::instance().begin()) {
    Serial.println("[-1.5/9] ✗ SimpleConfigManager initialization failed!");
  } else {
    LogManager::instance().logf("[-1.5/9] ✓ SimpleConfigManager initialized (mutex-protected NVS)\n");

    // Test NVS write operation
    SimpleConfigManager::instance().saveTestConfig("test_key", "test_value");

    // Test NVS read operation
    String value = SimpleConfigManager::instance().loadTestConfig("test_key", "default");
  }
  Serial.println();

  // Initialize LittleFS with correct partition name
  Serial.println("[-1/7] Initializing LittleFS...");
  if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
    Serial.println("[-1/7] ✗ LittleFS mount failed!");
  } else {
    Serial.printf("[-1/7] ✓ LittleFS mounted - Total: %d KB, Used: %d KB\n",
                  LittleFS.totalBytes() / 1024, LittleFS.usedBytes() / 1024);
  }
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

    // Setup NTP and timezone (like main app does)
    Serial.println("[0.5/7] Setting up NTP time sync...");
    setServer("time.cloudflare.com");
    setInterval(3600); // Sync every hour
    waitForSync(30); // Wait up to 30 seconds

    if (timeStatus() == timeSet) {
      Serial.printf("[0.5/7] ✓ NTP synced - Current time: %s\n", dateTime().c_str());

      // Set timezone like main app
      Timezone myTZ;
      myTZ.setLocation("Europe/London");
      Serial.printf("[0.5/7] ✓ Timezone set to Europe/London - Local time: %s\n", myTZ.dateTime().c_str());
    } else {
      Serial.println("[0.5/7] ✗ NTP sync failed (continuing anyway)");
    }
  } else {
    Serial.println("[0/7] ✗ WiFi connection failed - MQTT will not work");
  }
  Serial.println();

  // Initialize button GPIOs with interrupt handlers (like main app)
  Serial.println("[0.7/7] Initializing button GPIOs with interrupts...");
  const int buttonPins[] = {5, 6, 7, 4}; // Same as main app for C3
  void (*buttonISRs[])() = {button1ISR, button2ISR, button3ISR, button4ISR};

  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(buttonPins[i]), buttonISRs[i], FALLING);
    Serial.printf("[0.7/7]   GPIO %d configured with interrupt\n", buttonPins[i]);
  }
  Serial.println("[0.7/7] ✓ 4 button interrupts attached");
  Serial.println();

  // Enable watchdog timer (like main app)
  Serial.println("[0.8/8] Enabling watchdog timer...");
  esp_task_wdt_init(8, true);  // 8 second timeout, panic on timeout
  esp_task_wdt_add(NULL);      // Add current task to watchdog
  Serial.println("[0.8/8] ✓ Watchdog timer enabled (8s timeout)");
  Serial.println();

  // Initialize FastLED
  Serial.println("[1/6] Initializing FastLED...");
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  Serial.println("[1/6] ✓ FastLED.addLeds() succeeded");

  // Clear all LEDs (set to black)
  Serial.println("[2/6] Clearing LEDs (all black)...");
  FastLED.clear();
  Serial.println("[2/6] ✓ FastLED.clear() succeeded");

  // First FastLED.show() call - test without class wrapper first
  Serial.println("[3/6] Calling FastLED.show() for the first time...");
  Serial.flush();  // Make sure this message is printed before crash

  FastLED.show();  // ← Test basic FastLED operation

  Serial.println("[3/6] ✓ FastLED.show() succeeded (no crash!)");

  // Initialize REAL LedEffects singleton (EXACTLY like main app)
  Serial.println("[4/6] Initializing REAL LedEffects singleton...");
  if (!ledEffects().begin()) {
    Serial.println("[4/6] ✗ LedEffects initialization failed!");
  } else {
    Serial.println("[4/6] ✓ LedEffects initialized (with EffectRegistry)");
  }

  // Start a real LED effect (chase_single with 10 cycles for testing)
  Serial.println("[5/7] Starting chase_single effect via REAL ledEffects()...");
  if (ledEffects().startEffectCycles("chase_single", 10, CRGB::Green, CRGB::Black, CRGB::Black)) {
    Serial.println("[5/7] ✓ Chase single effect started (10 cycles)");
  } else {
    Serial.println("[5/7] ✗ Failed to start chase effect!");
  }
  Serial.println();

  // Initialize PrinterManager (UART1 on GPIO 21)
  Serial.println("[5.5/7] Initializing SimplePrinterManager...");
  if (!SimplePrinterManager::instance().begin(21, -1)) {  // TX=21, RX=not used
    Serial.println("[5.5/7] ✗ SimplePrinterManager initialization failed!");
  } else {
    LogManager::instance().logf("[5.5/7] ✓ SimplePrinterManager initialized (UART1)\n");

    // Print test message while LED effect is running (CRITICAL TEST)
    LogManager::instance().logf("[5.5/7] Printing test message while LEDs are updating...\n");
    SimplePrinterManager::instance().printTestMessage("BOOT TEST - LED effects running");
  }
  Serial.println();

  // NOW START ASYNCWEBSERVER with heavy route handlers (like main app)
  Serial.println("[6/6] Starting AsyncWebServer...");

  // Simple route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello from ESP32-C3!");
  });

  // Route that uses LogManager (concurrent logging from web handler task)
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    LogManager::instance().logf("[WebServer] /test endpoint called\n");
    request->send(200, "text/plain", "Test endpoint working");
  });

  // Route that uses ConfigManager (concurrent NVS access from web handler task)
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    LogManager::instance().logf("[WebServer] /config endpoint called\n");
    String value = SimpleConfigManager::instance().loadTestConfig("test_key", "default");
    request->send(200, "text/plain", "Config value: " + value);
  });

  // POST route that uses both LogManager and ConfigManager (heavy concurrent operations)
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    LogManager::instance().logf("[WebServer] /save POST called\n");

    if (request->hasParam("key", true) && request->hasParam("value", true)) {
      String key = request->getParam("key", true)->value();
      String value = request->getParam("value", true)->value();

      // Concurrent NVS write from web handler while LED effects are running
      bool success = SimpleConfigManager::instance().saveTestConfig(key.c_str(), value.c_str());

      if (success) {
        request->send(200, "text/plain", "Saved successfully");
      } else {
        request->send(500, "text/plain", "Save failed");
      }
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });

  // Route that triggers LED effect change (concurrent LED manipulation from web handler)
  server.on("/led", HTTP_GET, [](AsyncWebServerRequest *request) {
    LogManager::instance().logf("[WebServer] /led endpoint called - restarting chase effect\n");
    ledEffects().stopEffect();
    delay(100);  // Simulate heavy operation
    ledEffects().startEffectCycles("chase_single", 10, CRGB::Blue, CRGB::Black, CRGB::Black);
    request->send(200, "text/plain", "LED effect restarted (blue)");
  });

  server.begin();
  webServerStarted = true;

  Serial.printf("[6/6] ✓ AsyncWebServer started on http://%s\n", WiFi.localIP().toString().c_str());
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

  // Use LogManager for final summary (tests queue during critical init phase)
  LogManager::instance().logf("========================================\n");
  LogManager::instance().logf("PHASE 1 COMPLETE: All initialization OK\n");
  LogManager::instance().logf("Components running:\n");
  LogManager::instance().logf("  - SimpleLogManager (queue + writer task)\n");
  LogManager::instance().logf("  - SimpleConfigManager (mutex + NVS)\n");
  LogManager::instance().logf("  - SimplePrinterManager (UART1 @ 19200 baud)\n");
  LogManager::instance().logf("  - LittleFS\n");
  LogManager::instance().logf("  - ezTime/NTP\n");
  LogManager::instance().logf("  - Button GPIOs (4 interrupts)\n");
  LogManager::instance().logf("  - Watchdog timer (8s)\n");
  LogManager::instance().logf("  - FastLED\n");
  LogManager::instance().logf("  - REAL LedEffects singleton (EffectRegistry + ChaseSingle)\n");
  LogManager::instance().logf("  - AsyncWebServer\n");
  LogManager::instance().logf("  - MQTT: %s\n", mqttConnected ? "CONNECTED" : "disconnected");
  LogManager::instance().logf("\n");
  LogManager::instance().logf("PHASE 2 STARTING: Running REAL LED effects\n");
  LogManager::instance().logf("\n");
  LogManager::instance().logf("If crash happens, it will be in loop()\n");
  LogManager::instance().logf("when ledEffects().update() calls FastLED.show()\n");
  LogManager::instance().logf("via EffectRegistry -> ChaseSingle -> LedEffects -> FastLED.\n");
  LogManager::instance().logf("========================================\n");
  LogManager::instance().logf("\n");
}

void loop() {
  // Feed the watchdog (like main app)
  esp_task_wdt_reset();

  // Process ezTime events (for NTP sync)
  events();

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

  // Update LED effects via REAL LedEffects singleton (EXACTLY like main app)
  // This calls FastLED.show() from within the real effect system
  ledEffects().update();  // ← THIS IS IDENTICAL to ledEffects().update() in main.cpp:309

  // Print status every 10 seconds via LogManager (tests queue during loop)
  static unsigned long lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 10000) {
    LogManager::instance().logf("✓ Still running... uptime: %lu sec, free heap: %d, MQTT: %s\n",
                                      millis() / 1000, ESP.getFreeHeap(),
                                      mqttClient.connected() ? "connected" : "disconnected");
    lastStatusPrint = millis();
  }
}
