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
 * Based on the original Project Scribe by UrbanCircles.
 *
 *  TODO:
 * - Move many items from config.h to web interface and file so people can enter API
 *   keys and use a pre-compiled firmware. Also allows them to configure
 *   0+ printer names and their own MQTT server.
 * - Unbidden Ink >> link to ChatGPT API
 * - FastLED support for LED strip
 * - Hardware button testing
 * - OTA flash capability?
 * - AP fallback
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <ezTime.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include "core/config.h"
#include "core/config_loader.h"
#include "core/config_utils.h"
#include "core/shared_types.h"

// Local module includes
#include "utils/character_mapping.h"
#include "web/web_server.h"
#include "core/network.h"
#include "hardware/printer.h"
#include "core/mqtt_handler.h"
#include "utils/time_utils.h"
#include "core/logging.h"
#include "hardware/hardware_buttons.h"
#include "content/content_generators.h"
#include "utils/api_client.h"
#include "content/unbidden_ink.h"

// === Web Server ===
WebServer server(webServerPort);

// === Memory Monitoring Variables ===
unsigned long lastMemCheck = 0;

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
  Serial.println("\n=== Scribe Starting === (Pre-NTP sync)");

  // Validate configuration
  validateConfig();

  // Initialize printer configuration lookup functions
  initializePrinterConfig();

  // Initialize status LED
  initializeStatusLED();

  // Connect to WiFi (required for NTP sync) - may fallback to AP mode
  currentWiFiMode = connectToWiFi();

  // Initialize logging system (before other components that use logging)
  setupLogging();

  // Log main startup message immediately after logging is ready
  LOG_NOTICE("BOOT", "=== Scribe Starting === (Pre-NTP sync)");

  // Log logging system configuration
  LOG_VERBOSE("BOOT", "Logging system initialized (Pre-NTP sync) - Level: %s, Serial: %s, File: %s, MQTT: %s, BetterStack: %s",
              getLogLevelString(logLevel).c_str(),
              enableSerialLogging ? "ON" : "OFF",
              enableFileLogging ? "ON" : "OFF",
              enableMQTTLogging ? "ON" : "OFF",
              enableBetterStackLogging ? "ON" : "OFF");

  // Enable watchdog timer
  esp_task_wdt_init(watchdogTimeoutSeconds, true);
  esp_task_wdt_add(NULL);
  LOG_VERBOSE("BOOT", "Watchdog timer enabled (%ds timeout) (Pre-NTP sync)", watchdogTimeoutSeconds);

  // Initialize timezone with automatic DST handling (must be after WiFi for NTP sync)
  setupTimezone();

  // Log initial memory status
  LOG_VERBOSE("BOOT", "Free heap: %d bytes", ESP.getFreeHeap());

  // Initialize LittleFS for file system access
  if (!LittleFS.begin(true)) // true = format if mount fails
  {
    LOG_ERROR("BOOT", "LittleFS Mount Failed");
  }
  else
  {
    LOG_VERBOSE("BOOT", "LittleFS mounted successfully");
  }

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

  // Setup mDNS
  setupmDNS();

  // Setup MQTT
  setupMQTT();

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

  // Check hardware buttons (works even without WiFi)
  checkHardwareButtons();

  // Handle web server requests
  // In AP mode: only serve settings for configuration
  // In STA mode: serve all content and handle MQTT
  server.handleClient();

  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED)
  {
    // Handle MQTT connection and messages (only in STA mode)
    handleMQTTConnection();
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
