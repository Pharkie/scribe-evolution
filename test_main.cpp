// ESP32-S3 INCREMENTAL BUILD TEST
// Adding components one by one to find the crash source

#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <FastLED.h>
#include <ESPmDNS.h>
#include "config/config.h"
#include "core/config_loader.h"
#include "core/config_utils.h"
#include "hardware/printer.h"
#include "web/web_server.h"  // Add web server components
#include "core/shared_types.h"

// ============================================================================
// TEST: Add PrinterPinStabilizer (from main.cpp)
// This runs BEFORE setup() as a static global initializer
// ============================================================================
class PrinterPinStabilizer
{
public:
  PrinterPinStabilizer()
  {
    const RuntimeConfig &config = getRuntimeConfig(); // Fast - returns defaults
    const BoardPinDefaults &boardDefaults = getBoardDefaults();

    // Enable printer eFuse if present (custom PCB only)
    #if BOARD_HAS_PRINTER_EFUSE
    if (boardDefaults.efuse.printer != -1)
    {
      pinMode(boardDefaults.efuse.printer, OUTPUT);
      digitalWrite(boardDefaults.efuse.printer, HIGH); // Enable printer power
      delay(10); // Give eFuse time to stabilize
    }
    #endif

    // Set up printer UART TX pin
    pinMode(config.printerTxPin, OUTPUT);
    digitalWrite(config.printerTxPin, HIGH); // UART idle state

    // Set up DTR pin if present (ESP32-S3 variants)
    if (boardDefaults.printer.dtr != -1)
    {
      pinMode(boardDefaults.printer.dtr, OUTPUT);
      digitalWrite(boardDefaults.printer.dtr, LOW); // DTR ready
    }
  }
};
static PrinterPinStabilizer pinStabilizer;

#define NUM_LEDS 1
#define LED_PIN 48

CRGB leds[NUM_LEDS];

// Add the web server globals that the main app uses
extern Message currentMessage;
extern SemaphoreHandle_t currentMessageMutex;

// Create web server instance (like main.cpp does)
AsyncWebServer server(webServerPort);

// Add time utils global
String deviceBootTime = "2025-10-19 00:00:00";

void setup() {
  // Serial for debug output
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=================================================");
  Serial.println("   ESP32-S3 INCREMENTAL BUILD TEST");
  Serial.println("=================================================\n");

  // TEST 1: LittleFS Initialization
  Serial.println("[TEST 1] Initializing LittleFS...");
  if (!LittleFS.begin(true, "/littlefs", 10, "littlefs"))
  {
    Serial.println("  ✗ LittleFS Mount Failed!");
  }
  else
  {
    Serial.println("  ✓ LittleFS mounted successfully\n");
  }

  // TEST 2: Config System Initialization
  Serial.println("[TEST 2] Initializing config system...");
  validateConfig();
  initializePrinterConfig();
  Serial.println("  ✓ Config system initialized\n");

  // TEST 3: WiFi Initialization and Connection
  Serial.println("[TEST 3] Connecting to WiFi...");
  WiFi.mode(WIFI_STA);

  const RuntimeConfig &config = getRuntimeConfig();
  WiFi.begin(config.wifiSSID.c_str(), config.wifiPassword.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n  ✓ WiFi connected: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n  ✗ WiFi connection failed - continuing anyway\n");
  }
  Serial.println();

  // TEST 4: REAL Printer Initialization (from printer.cpp)
  Serial.println("[TEST 4] Initializing printer (REAL printerManager.initialize)...");
  printerManager.initialize();
  Serial.println("  ✓ Printer initialized successfully\n");

  // TEST 5: Test ACTUAL printing with real functions
  Serial.println("[TEST 5] Testing ACTUAL printer output...");
  if (printerManager.isReady())
  {
    Serial.println("  Printer is ready - attempting to print...");

    // Use the real printerManager.printWithHeader function
    printerManager.printWithHeader("ESP32-S3 TEST", "This is a test print from the real printer functions!");

    Serial.println("  ✓ Print sent successfully\n");
  }
  else
  {
    Serial.println("  ✗ Printer NOT ready!\n");
  }

  // TEST 6: Initialize mutex and message system (like main app)
  Serial.println("[TEST 6] Initializing mutex and message system...");

  // Initialize mutex if not already done
  if (currentMessageMutex == nullptr) {
    currentMessageMutex = xSemaphoreCreateMutex();
    Serial.println("  Created currentMessage mutex");
  }

  Serial.println("  ✓ Mutex system initialized\n");

  // TEST 7: Multiple print operations to see if printer BECOMES not ready
  Serial.println("[TEST 7] Testing multiple prints (watching for printerReady changes)...");

  for (int i = 0; i < 5; i++) {
    Serial.printf("  [Iteration %d] Printer ready before: %s\n", i, printerManager.isReady() ? "TRUE" : "FALSE");

    // Set a message using mutex (like web server does)
    if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      currentMessage.message = String("TEST ") + i + "\n\nThis is test iteration " + i;
      currentMessage.timestamp = "2025-10-19 01:56";
      currentMessage.shouldPrintLocally = true;
      xSemaphoreGive(currentMessageMutex);
    }

    // Call printMessage like main loop does
    printMessage();

    Serial.printf("  [Iteration %d] Printer ready after: %s\n", i, printerManager.isReady() ? "TRUE" : "FALSE");

    // Check if printer became not ready
    if (!printerManager.isReady()) {
      Serial.println("  ⚠️  WARNING: Printer became NOT READY during operation!");
      Serial.println("  This is the condition that causes the crash in main app!");
      break;
    }

    delay(100);
  }

  Serial.println("  ✓ Multiple print test completed\n");

  // TEST 8: Setup mDNS and start web server (like main app)
  Serial.println("[TEST 8] Starting mDNS and AsyncWebServer...");

  // Start mDNS
  if (!MDNS.begin(getMdnsHostname())) {
    Serial.println("  ✗ mDNS initialization failed!");
  } else {
    Serial.printf("  ✓ mDNS started: %s.local\n", getMdnsHostname());
  }

  setupWebServerRoutes(maxCharacters);
  server.begin();

  Serial.printf("  ✓ Web server started on: http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("  ✓ mDNS URL: http://%s.local\n", getMdnsHostname());
  Serial.println();

  // Final Summary
  Serial.println("=================================================");
  Serial.println("   ALL TESTS COMPLETED SUCCESSFULLY!");
  Serial.println("=================================================");
  Serial.println("\nIf printer produced output, UART hardware works!");
  Serial.println("Next: Running in loop mode with web server active...\n");

  // Setup FastLED for RGB LED on GPIO 48
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  Serial.println("FastLED initialized on GPIO 48");

  // TEST 9: Simulate ONE web print request
  Serial.println("\n[TEST 9] Simulating web print request...");

  Serial.printf("  Printer ready BEFORE: %s\n", printerManager.isReady() ? "TRUE" : "FALSE");

  // Queue a message (like web API does)
  if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    currentMessage.message = "WEB TEST\n\nThis simulates a web-triggered print request";
    currentMessage.timestamp = "2025-10-19 02:00";
    currentMessage.shouldPrintLocally = true;
    xSemaphoreGive(currentMessageMutex);
    Serial.println("  Message queued");
  }

  // Process the print in next loop iteration (like main app)
  Serial.println("  Print will be processed in loop...\n");

  Serial.println("Entering loop mode...\n");
}

int loopCount = 0;
unsigned long lastLedChange = 0;
unsigned long lastAutoPrint = 0;
int testLedState = 0;  // 0=Red, 1=Green, 2=Blue
int autoPrintCount = 0;

void loop() {
  unsigned long currentMillis = millis();

  // Auto-trigger a print every 10 seconds (simulates repeated web requests)
  if (currentMillis - lastAutoPrint >= 10000) {
    lastAutoPrint = currentMillis;
    autoPrintCount++;

    Serial.printf("\n[AUTO-PRINT #%d] Queueing simulated web print...\n", autoPrintCount);
    Serial.printf("[AUTO-PRINT #%d] Printer ready BEFORE queue: %s\n", autoPrintCount, printerManager.isReady() ? "TRUE" : "FALSE");

    // Queue a message (like web API does)
    if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      currentMessage.message = String("AUTO TEST #") + autoPrintCount + "\n\nSimulated web print number " + autoPrintCount;
      currentMessage.timestamp = "2025-10-19 02:00";
      currentMessage.shouldPrintLocally = true;
      xSemaphoreGive(currentMessageMutex);
      Serial.printf("[AUTO-PRINT #%d] Message queued\n", autoPrintCount);
    }
  }

  // Check for messages to print (like main app loop)
  bool shouldPrint = false;
  if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    shouldPrint = currentMessage.shouldPrintLocally;
    xSemaphoreGive(currentMessageMutex);
  }

  if (shouldPrint) {
    Serial.printf("[LOOP] Processing AUTO-PRINT #%d...\n", autoPrintCount);
    Serial.printf("[LOOP] Printer ready BEFORE print: %s\n", printerManager.isReady() ? "TRUE" : "FALSE");

    printMessage();

    // Clear the flag
    if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      currentMessage.shouldPrintLocally = false;
      xSemaphoreGive(currentMessageMutex);
    }

    Serial.printf("[LOOP] Printer ready AFTER print: %s\n", printerManager.isReady() ? "TRUE" : "FALSE");

    // Check if printer became not ready
    if (!printerManager.isReady()) {
      Serial.println("\n⚠️⚠️⚠️ PRINTER BECAME NOT READY! This may cause crash! ⚠️⚠️⚠️\n");
    }
    Serial.println();
  }

  // Cycle LED colors every 1 second
  if (currentMillis - lastLedChange >= 1000) {
    lastLedChange = currentMillis;

    switch(testLedState) {
      case 0:
        leds[0] = CRGB::Red;
        break;
      case 1:
        leds[0] = CRGB::Green;
        break;
      case 2:
        leds[0] = CRGB::Blue;
        break;
    }
    FastLED.show();

    testLedState = (testLedState + 1) % 3;
  }

  loopCount++;
  delay(10);  // Small delay like main app
}
