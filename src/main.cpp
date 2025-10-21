/**
 * @file main.cpp
 * @brief Main application entry point for Scribe Evolution ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe Evolution ESP32-C3 Thermal Printer project.
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/4.0/ or send a letter to
 * Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
 *
 * Commercial use is prohibited without explicit written permission from the author.
 * For commercial licensing inquiries, please contact Adam Knowles.
 *
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <esp_log.h>
#include <ezTime.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include "config/config.h"
#include "core/config_loader.h"
#include "core/config_utils.h"
#include "core/shared_types.h"
#include "web/web_server.h"
#include "core/network.h"
#include "hardware/printer.h"
#include "core/mqtt_handler.h"
#include "core/printer_discovery.h"
#include "utils/time_utils.h"
#include "core/logging.h"
#include "core/LogManager.h"
#include "hardware/hardware_buttons.h"
#include "content/content_generators.h"
#include "utils/api_client.h"
#include "content/unbidden_ink.h"
#if ENABLE_LEDS
#include "leds/LedEffects.h"
#endif

// Stabilize printer pin ASAP, before anything else
// COMMENTED OUT FOR TESTING - Suspected cause of ESP32-S3 printer crash
// class PrinterPinStabilizer
// {
// public:
//   PrinterPinStabilizer()
//   {
//     const RuntimeConfig &config = getRuntimeConfig(); // Fast - returns defaults
//     const BoardPinDefaults &boardDefaults = getBoardDefaults();

//     // Enable printer eFuse if present (custom PCB only)
//     #if BOARD_HAS_PRINTER_EFUSE
//     if (boardDefaults.efuse.printer != -1)
//     {
//       pinMode(boardDefaults.efuse.printer, OUTPUT);
//       digitalWrite(boardDefaults.efuse.printer, HIGH); // Enable printer power
//       delay(10); // Give eFuse time to stabilize
//     }
//     #endif

//     // Set up printer UART TX pin
//     pinMode(config.printerTxPin, OUTPUT);
//     digitalWrite(config.printerTxPin, HIGH); // UART idle state

//     // Set up DTR pin if present (ESP32-S3 variants)
//     if (boardDefaults.printer.dtr != -1)
//     {
//       pinMode(boardDefaults.printer.dtr, OUTPUT);
//       digitalWrite(boardDefaults.printer.dtr, LOW); // DTR ready
//     }
//   }
// };
// static PrinterPinStabilizer pinStabilizer;

// === Web Server ===
AsyncWebServer server(webServerPort);

// === Memory Monitoring Variables ===
unsigned long lastMemCheck = 0;

// === Boot Time Tracking ===
String deviceBootTime = "";

void setup()
{
  // Track boot time
  unsigned long bootStartTime = millis();

  // Initialize serial communication first (USB CDC)
  Serial.begin(115200);

  // Allow USB-C connection to send logs back to monitor on host computer
  // Wait for USB CDC connection
  // Needed due to ARDUINO_USB_CDC_ON_BOOT=1 in platformio.ini
  unsigned long serialStartTime = millis();
  while (!Serial && (millis() - serialStartTime < serialTimeoutMs))
  {
    delay(smallDelayMs); // Wait up to 5 seconds for serial connection
  }

  // Note: We can't use Log.notice() yet as logging isn't initialized
  Serial.printf("\n=== Scribe Evolution v%s ===\n", FIRMWARE_VERSION);
  Serial.printf("[BOOT] Built: %s %s\n", BUILD_DATE, BUILD_TIME);
  Serial.printf("[BOOT] System: %s, %d KB free heap\n", ESP.getChipModel(), ESP.getFreeHeap() / mediumJsonBuffer);

  // Initialize LittleFS so config loading works
  if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) // true = format if mount fails
  {
    Serial.println("LittleFS Mount Failed");
  }

  // Validate configuration
  validateConfig();

  // Initialize printer configuration lookup functions
  initializePrinterConfig();

  // Initialize status LED
  initializeStatusLED();

  // DISABLED: WiFi - minimizing components for ESP32-C3 crash debugging
  // currentWiFiMode = connectToWiFi();
  currentWiFiMode = WIFI_MODE_DISCONNECTED;
  WiFi.mode(WIFI_OFF);
  Serial.println("[BOOT] WiFi: ❌ Disabled (debugging)");

  // CHECKPOINT 1: Re-enable all managers (needed for getRuntimeConfig)
  LogManager::instance().begin(115200, 256, 512);
  Serial.println("[BOOT] LogManager: ✅ Enabled");
  APIClient::instance().begin();
  Serial.println("[BOOT] APIClient: ✅ Enabled");
  ConfigManager::instance().begin();
  Serial.println("[BOOT] ConfigManager: ✅ Enabled");
  MQTTManager::instance().begin();
  Serial.println("[BOOT] Managers: ✅ Enabled (Checkpoint 1)");

  // Configure ESP32 system component log levels
  esp_log_level_set("WebServer", espLogLevel);
#ifdef RELEASE_BUILD
  // Suppress VFS errors in production (AsyncWebServer checks for uncompressed files before serving .gz)
  esp_log_level_set("vfs", ESP_LOG_NONE);
#endif

  // Log logging configuration (LogManager is now ready)
  LOG_VERBOSE("BOOT", "Logging configured - Level: %s (serial output only)",
              getLogLevelString(logLevel).c_str());

  // Initialize mutex for currentMessage (protects against multi-core race conditions)
  currentMessageMutex = xSemaphoreCreateMutex();
  if (currentMessageMutex == nullptr)
  {
    LOG_ERROR("BOOT", "Failed to create currentMessage mutex!");
  }

  // Enable watchdog timer
  esp_task_wdt_init(watchdogTimeoutSeconds, true);
  esp_task_wdt_add(NULL);
  LOG_VERBOSE("BOOT", "Watchdog timer enabled (%ds timeout)", watchdogTimeoutSeconds);

  // Initialize timezone with conditional NTP sync (only in STA mode)
  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED)
  {
    setupTime();
  }
  else
  {
    LOG_VERBOSE("BOOT", "Skipping NTP sync - no internet connection (AP-STA mode)");
  }

  // Record boot time for consistent reporting (after timezone is set)
  deviceBootTime = getISOTimestamp();
  LOG_VERBOSE("BOOT", "Device boot time recorded: %s", deviceBootTime.c_str());

  // Log initial memory status
  LOG_VERBOSE("BOOT", "Free heap: %d bytes", ESP.getFreeHeap());

  // Log detailed GPIO summary in verbose mode (now that logging is available)
  logGPIOUsageSummary();

  // CHECKPOINT 2: Re-enable config system and printer
  if (!initializeConfigSystem())
  {
    LOG_ERROR("BOOT", "Configuration system initialization failed");
  }
  else
  {
    LOG_VERBOSE("BOOT", "Configuration system initialized successfully");
  }
  printerManager.initialize();
  Serial.println("[BOOT] Config/Printer: ✅ Enabled (Checkpoint 2)");

#if ENABLE_LEDS
  // DISABLED: Direct FastLED initialization (testing if ledEffects().begin() can do it alone)
  // extern CRGB staticLEDs[];  // Defined in LedEffects.cpp
  // Serial.println("[BOOT] Initializing FastLED directly (pinMode disabled in buttons)...");
  // FastLED.addLeds<WS2812B, 20, GRB>(staticLEDs, 30);
  // FastLED.setBrightness(100);
  // FastLED.clear();
  // FastLED.show();
  // Serial.println("[BOOT] ✅ FastLED initialized directly");

  // TESTING: ledEffects().begin() WITHOUT direct FastLED init first
  Serial.println("[BOOT] Testing ledEffects().begin() initialization (no direct FastLED)...");
  if (ledEffects().begin()) {
    Serial.println("[BOOT] ✅ ledEffects().begin() succeeded");
  } else {
    Serial.println("[BOOT] ❌ ledEffects().begin() FAILED");
  }
#endif

  // TESTING: Re-enable button initialization (with pinMode DISABLED)
  if (!isAPMode())
  {
    initializeHardwareButtons();
  }
  else
  {
    LOG_NOTICE("BOOT", "Buttons: ❌ Disabled (AP mode)");
  }
  Serial.println("[BOOT] Buttons: ✅ Enabled (testing with pinMode disabled)");

  // Setup mDNS
  setupmDNS();

  // DISABLED: MQTT client - minimizing components for ESP32-C3 crash debugging
  // if (!isAPMode() && isMQTTEnabled())
  // {
  //   startMQTTClient(true); // true = immediate connection on boot (WiFi is already stable)
  //   LOG_NOTICE("BOOT", "MQTT: Connecting to broker...");
  // }
  // else
  // {
  //   if (isAPMode())
  //   {
  //     LOG_NOTICE("BOOT", "MQTT: ❌ Disabled (AP mode)");
  //   }
  //   else
  //   {
  //     LOG_NOTICE("BOOT", "MQTT: ❌ Disabled");
  //   }
  // }
  LOG_NOTICE("BOOT", "MQTT: ❌ Disabled (debugging)");

  // DISABLED: Web server - minimizing components for ESP32-C3 crash debugging
  // setupWebServerRoutes(maxCharacters);
  // server.begin();
  // LOG_NOTICE("BOOT", "Web UI: ✅ http://%s", WiFi.localIP().toString().c_str());
  LOG_NOTICE("BOOT", "Web UI: ❌ Disabled (debugging)");

  // DISABLED: Unbidden Ink - minimizing components for ESP32-C3 crash debugging
  // initializeUnbiddenInk();
  LOG_NOTICE("BOOT", "Unbidden Ink: ❌ Disabled (debugging)");

  // Calculate boot time
  unsigned long bootDuration = millis() - bootStartTime;
  float bootSeconds = bootDuration / 1000.0;

  // Get device name from config
  const RuntimeConfig &config = getRuntimeConfig();
  String deviceName = config.deviceOwner;
  if (deviceName.length() == 0)
  {
    deviceName = "Unknown";
  }

  if (isAPMode())
  {
    LOG_NOTICE("BOOT", "=== %s Ready (Setup Mode) in %.1f seconds ===", deviceName.c_str(), bootSeconds);
  }
  else
  {
    LOG_NOTICE("BOOT", "=== %s Ready in %.1f seconds ===", deviceName.c_str(), bootSeconds);
  }
}

/**
 * @brief Post-setup initialization - runs once at the start of first loop() iteration
 *
 * This function handles initialization tasks that should happen AFTER setup() completes,
 * allowing setup() to finish quickly without blocking delays. This prevents race conditions
 * where non-blocking effects (like LED boot animations) are still running when loop() starts.
 */
void postSetup()
{
#if ENABLE_LEDS
  // TESTING: Trigger boot LED chase effect (1 cycle)
  Serial.println("[POST_SETUP] Triggering boot LED chase effect...");
  ledEffects().startEffectCycles("chase_single", 1);
  Serial.println("[POST_SETUP] ✅ Boot LED effect started");
#endif

  // DISABLED: Print startup message - minimizing components for ESP32-C3 crash debugging
  // printerManager.printStartupMessage();
  // LOG_VERBOSE("POST_SETUP", "Startup message printed");
}

void loop()
{
  // Run post-setup tasks once on first loop iteration
  static bool firstRun = true;
  if (firstRun)
  {
    postSetup();
    firstRun = false;
  }
  // Feed the watchdog
  esp_task_wdt_reset();

  // Process ezTime events (for timezone updates)
  events();

  // Check WiFi connection and reconnect if needed
  handleWiFiReconnection();

  // Handle DNS server for captive portal in AP mode
  handleDNSServer();

  // Check hardware buttons (only if not in AP mode - buttons disabled in AP mode)
  if (!isAPMode())
  {
    checkHardwareButtons();
  }

#if ENABLE_LEDS
  // DISABLED: Direct FastLED chase (keep for comparison with ledEffects)
  // static unsigned long lastLedUpdate = 0;
  // static int ledPos = 0;
  // extern CRGB staticLEDs[];
  //
  // if (millis() - lastLedUpdate > 50) {
  //   // Simple chase effect
  //   for (int i = 0; i < 30; i++) {
  //     staticLEDs[i] = CRGB::Black;
  //   }
  //   staticLEDs[ledPos] = CRGB::Blue;
  //
  //   Serial.println("[LOOP] Calling FastLED.show() directly...");
  //   FastLED.show();
  //   Serial.println("[LOOP] ✓ FastLED.show() succeeded!");
  //
  //   ledPos = (ledPos + 1) % 30;
  //   lastLedUpdate = millis();
  // }

  // TESTING: Try ledEffects().update() instead of direct FastLED
  Serial.println("[LOOP] Calling ledEffects().update()...");
  ledEffects().update();
  Serial.println("[LOOP] ✓ ledEffects().update() succeeded!");
#endif

  // Handle web server requests - AsyncWebServer handles this automatically
  // No need to call server.handleClient() with async server

  // DISABLED: MQTT handling - minimizing components for ESP32-C3 crash debugging
  // if (currentWiFiMode == WIFI_MODE_STA_CONNECTED && isMQTTEnabled())
  // {
  //   // Handle MQTT connection and messages (only in STA mode when MQTT enabled)
  //   handleMQTTConnection();
  //
  //   // Handle printer discovery (only in STA mode when MQTT enabled)
  //   handlePrinterDiscovery();
  // }
  // Check if we have a new message to print (protected by mutex for multi-core safety)
  bool shouldPrint = false;
  if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(10)) == pdTRUE)
  {
    shouldPrint = currentMessage.shouldPrintLocally;
    xSemaphoreGive(currentMessageMutex);
  }

  if (shouldPrint)
  {
    printMessage(); // printMessage() will acquire mutex internally to read currentMessage

    // Clear the flag after printing
    if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      currentMessage.shouldPrintLocally = false;
      xSemaphoreGive(currentMessageMutex);
    }
  }

  // Monitor memory usage periodically
  if (millis() - lastMemCheck > memCheckIntervalMs)
  {
    LOG_VERBOSE("SYSTEM", "Free heap: %d bytes", ESP.getFreeHeap());
    lastMemCheck = millis();
  }

  // Check Unbidden Ink schedule (only if WiFi connected for API calls)
  if (WiFi.status() == WL_CONNECTED)
  {
    checkUnbiddenInk();
  }
  delay(smallDelayMs); // Small delay to prevent excessive CPU usage
}
