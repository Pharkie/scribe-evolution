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
 * Inspired by Project Scribe by UrbanCircles.
 *
 *  TODO:
 * - FastLED support for LED strip
 *
 *  IDEAS:
 * - Allow disabling of MQTT for purely local printing
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
#include "hardware/hardware_buttons.h"
#include "content/content_generators.h"
#include "utils/api_client.h"
#include "content/unbidden_ink.h"
#if ENABLE_LEDS
#include "leds/LedEffects.h"
#else
// Forward declaration of LED stub
extern class LedEffects
{
public:
  bool begin();
  bool startEffectCycles(const String &effectName, int cycles = 1,
                         unsigned long color1 = 0, unsigned long color2 = 0, unsigned long color3 = 0);
  void update();
} ledEffects;
#endif

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
  
  // Stabilize printer pin as early as possible
  stabilizePrinterPin();

  // Initialize serial communication first (USB CDC)
  Serial.begin(115200);

  // Wait for USB CDC connection (ESP32-C3 with USB CDC enabled)
  // This is needed because of ARDUINO_USB_CDC_ON_BOOT=1 in platformio.ini
  unsigned long serialStartTime = millis();
  while (!Serial && (millis() - serialStartTime < serialTimeout))
  {
    delay(smallDelay); // Wait up to 5 seconds for serial connection
  }

  // Note: We can't use Log.notice() yet as logging isn't initialized
  Serial.printf("\n=== Scribe Evolution v%s ===\n", FIRMWARE_VERSION);
  Serial.printf("[BOOT] Built: %s %s\n", BUILD_DATE, BUILD_TIME);
  Serial.printf("[BOOT] System: ESP32-C3, %d KB free heap\n", ESP.getFreeHeap() / mediumJsonBuffer);

  // Initialize LittleFS early so config loading works
  if (!LittleFS.begin(true)) // true = format if mount fails
  {
    Serial.println("LittleFS Mount Failed");
  }

  // Validate configuration
  validateConfig();

  // Initialize printer configuration lookup functions
  initializePrinterConfig();

  // Initialize status LED
  initializeStatusLED();

  // Connect to WiFi (may fallback to AP mode)
  currentWiFiMode = connectToWiFi();

  // Initialize logging system before other components that use logging
  setupLogging();

  // Configure ESP32 system component log levels
  esp_log_level_set("WebServer", espLogLevel);

  // Log logging system configuration
  LOG_VERBOSE("BOOT", "Logging system initialized - Level: %s, Serial: %s, File: %s, MQTT: %s, BetterStack: %s",
              getLogLevelString(logLevel).c_str(),
              enableSerialLogging ? "ON" : "OFF",
              enableFileLogging ? "ON" : "OFF",
              enableMQTTLogging ? "ON" : "OFF",
              enableBetterStackLogging ? "ON" : "OFF");

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

  // Initialize configuration system
  if (!initializeConfigSystem())
  {
    LOG_ERROR("BOOT", "Configuration system initialization failed");
  }
  else
  {
    LOG_VERBOSE("BOOT", "Configuration system initialized successfully");
  }

  // Initialize printer
  initializePrinter();

  // Initialize hardware buttons (only in STA mode)
  if (!isAPMode())
  {
    initializeHardwareButtons();
  }
  else
  {
    LOG_NOTICE("BOOT", "Buttons: ❌ Disabled (AP mode)");
  }

#if ENABLE_LEDS
  // Initialize LED effects system
  if (ledEffects.begin())
  {
    LOG_VERBOSE("BOOT", "LED effects system initialized successfully");
    // Trigger boot LED effect (chase single for 1 cycle)
    ledEffects.startEffectCycles("chase_single", 1, 0x0000FF);
    LOG_VERBOSE("BOOT", "Boot LED effect started (chase_single, 1 cycle)");
  }
  else
  {
    LOG_WARNING("BOOT", "LED effects system initialization failed");
  }
#endif

  // Setup mDNS
  setupmDNS();

  // Setup MQTT client (only in STA mode and when MQTT enabled)
  if (!isAPMode() && isMQTTEnabled())
  {
    startMQTTClient(true);    // true = immediate connection on boot (WiFi is already stable)
    LOG_NOTICE("BOOT", "MQTT: Connecting to broker...");
  }
  else
  {
    if (isAPMode())
    {
      LOG_NOTICE("BOOT", "MQTT: ❌ Disabled (AP mode)");
    }
    else
    {
      LOG_NOTICE("BOOT", "MQTT: ❌ Disabled");
    }
  }

  // Setup web server routes
  setupWebServerRoutes(maxCharacters);

  // Start the server
  server.begin();
  LOG_NOTICE("BOOT", "Web UI: ✅ http://%s", WiFi.localIP().toString().c_str());

  // Print startup message (handles both AP mode and normal mode)
  printStartupMessage();

  // Initialize Unbidden Ink schedule
  initializeUnbiddenInk();

  // Calculate boot time
  unsigned long bootDuration = millis() - bootStartTime;
  float bootSeconds = bootDuration / 1000.0;
  
  // Get device name from config
  const RuntimeConfig &config = getRuntimeConfig();
  String deviceName = config.deviceOwner;
  if (deviceName.length() == 0) {
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

void loop()
{
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
  // Update LED effects (non-blocking)
  ledEffects.update();
#endif

  // Handle web server requests - AsyncWebServer handles this automatically
  // No need to call server.handleClient() with async server

  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED && isMQTTEnabled())
  {
    // Handle MQTT connection and messages (only in STA mode when MQTT enabled)
    handleMQTTConnection();

    // Handle printer discovery (only in STA mode when MQTT enabled)
    handlePrinterDiscovery();
  }

  // Check if we have a new message to print
  if (currentMessage.shouldPrintLocally)
  {
    LOG_VERBOSE("MAIN", "Printing message from main loop");
    printMessage();
    currentMessage.shouldPrintLocally = false; // Reset flag
  }

  // Monitor memory usage periodically
  if (millis() - lastMemCheck > memCheckInterval)
  {
    LOG_VERBOSE("SYSTEM", "Free heap: %d bytes", ESP.getFreeHeap());
    lastMemCheck = millis();
  }

  // Check Unbidden Ink schedule (only if WiFi connected for API calls)
  if (WiFi.status() == WL_CONNECTED)
  {
    checkUnbiddenInk();
  }

  delay(smallDelay); // Small delay to prevent excessive CPU usage
}
