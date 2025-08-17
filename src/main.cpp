/**
 * @file main.cpp
 * @brief Main application entry point for Scribe ESP32-C3 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe ESP32-C3 Thermal Printer project.
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
#include "core/config.h"
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
#ifdef ENABLE_LEDS
#include "leds/LedEffects.h"
#endif

// === Web Server ===
AsyncWebServer server(webServerPort);

// === Memory Monitoring Variables ===
unsigned long lastMemCheck = 0;

// === Boot Time Tracking ===
String deviceBootTime = "";

void setup()
{
  // Stabilize printer pin as early as possible
  stabilizePrinterPin();

  // Initialize serial communication first (USB CDC)
  Serial.begin(115200);

  // Wait for USB CDC connection (ESP32-C3 with USB CDC enabled)
  // This is needed because of ARDUINO_USB_CDC_ON_BOOT=1 in platformio.ini
  unsigned long startTime = millis();
  while (!Serial && (millis() - startTime < 5000))
  {
    delay(10); // Wait up to 5 seconds for serial connection
  }

  // Note: We can't use Log.notice() yet as logging isn't initialized
  Serial.println("\n=== Scribe Starting ===");

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

  // Log main startup message immediately after logging is ready
  LOG_NOTICE("BOOT", "=== Scribe Starting ===");

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
  setupTime();

  // Record boot time for consistent reporting (after timezone is set)
  deviceBootTime = getISOTimestamp();
  LOG_VERBOSE("BOOT", "Device boot time recorded: %s", deviceBootTime.c_str());

  // Log initial memory status
  LOG_VERBOSE("BOOT", "Free heap: %d bytes", ESP.getFreeHeap());

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

  // Initialize hardware buttons
  initializeHardwareButtons();

#ifdef ENABLE_LEDS
  // Initialize LED effects system
  if (ledEffects.begin()) {
    LOG_VERBOSE("BOOT", "LED effects system initialized successfully");
  } else {
    LOG_WARNING("BOOT", "LED effects system initialization failed");
  }
#endif

  // Setup mDNS
  setupmDNS();

  // Setup MQTT with printer discovery
  setupMQTTWithDiscovery();

  // Setup web server routes
  setupWebServerRoutes(maxCharacters);

  // Start the server
  server.begin();
  String webServerInfo = "Web server started: " + String(getMdnsHostname()) + ".local or " + WiFi.localIP().toString();
  LOG_VERBOSE("BOOT", "%s", webServerInfo.c_str());

  // Print server info
  printServerInfo();

  // Initialize Unbidden Ink schedule
  initializeUnbiddenInk();

  LOG_NOTICE("BOOT", "=== Scribe Ready ===");
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

  // Check hardware buttons (work without WiFi)
  checkHardwareButtons();

#ifdef ENABLE_LEDS
  // Update LED effects (non-blocking)
  ledEffects.update();
#endif

  // Handle web server requests - AsyncWebServer handles this automatically
  // No need to call server.handleClient() with async server

  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED)
  {
    // Handle MQTT connection and messages (only in STA mode)
    handleMQTTConnection();

    // Handle printer discovery (only in STA mode)
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

  delay(10); // Small delay to prevent excessive CPU usage
}
