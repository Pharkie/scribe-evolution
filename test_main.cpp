// ESP32-S3 INCREMENTAL BUILD TEST
// Adding components one by one to find the crash source

#include <Arduino.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <FastLED.h>
#include "config/config.h"
#include "core/config_loader.h"
#include "core/config_utils.h"
#include "hardware/printer.h"

#define NUM_LEDS 1
#define LED_PIN 48

CRGB leds[NUM_LEDS];

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

  // TEST 3: WiFi Initialization
  Serial.println("[TEST 3] Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("  ✓ WiFi initialized in STA mode\n");

  // TEST 4: REAL Printer Initialization (from printer.cpp)
  Serial.println("[TEST 4] Initializing printer (REAL initializePrinter)...");
  initializePrinter();
  Serial.println("  ✓ Printer initialized successfully\n");

  // TEST 5: Test ACTUAL printing with real functions
  Serial.println("[TEST 5] Testing ACTUAL printer output...");
  if (isPrinterReady())
  {
    Serial.println("  Printer is ready - attempting to print...");

    // Use the real printWithHeader function
    printWithHeader("ESP32-S3 TEST", "This is a test print from the real printer functions!");

    Serial.println("  ✓ Print sent successfully\n");
  }
  else
  {
    Serial.println("  ✗ Printer NOT ready!\n");
  }

  // Final Summary
  Serial.println("=================================================");
  Serial.println("   ALL TESTS COMPLETED SUCCESSFULLY!");
  Serial.println("=================================================");
  Serial.println("\nIf printer produced output, UART hardware works!");
  Serial.println("Next: Test with full application code.\n");

  // Setup FastLED for RGB LED on GPIO 48
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  Serial.println("FastLED initialized on GPIO 48");
  Serial.println("RGB LED will cycle: Red -> Green -> Blue...\n");
}

void loop() {
  // RED
  leds[0] = CRGB::Red;
  FastLED.show();
  Serial.println("[RGB] RED");
  delay(1000);

  // GREEN
  leds[0] = CRGB::Green;
  FastLED.show();
  Serial.println("[RGB] GREEN");
  delay(1000);

  // BLUE
  leds[0] = CRGB::Blue;
  FastLED.show();
  Serial.println("[RGB] BLUE");
  delay(1000);

  Serial.println("---");
}
