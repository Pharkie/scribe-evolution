/**
 * @file main.cpp
 * @brief Main application entry point for Scribe Evolution ESP32 Thermal Printer
 * @author Adam Knowles
 * @date 2025
 * @copyright Copyright (c) 2025 Adam Knowles. All rights reserved.
 * @license Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
 *
 * This file is part of the Scribe Evolution ESP32 Thermal Printer project.
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

  // Wait for USB CDC connection (needed due to ARDUINO_USB_CDC_ON_BOOT=1 in platformio.ini)
  unsigned long serialStartTime = millis();
  while (!Serial && (millis() - serialStartTime < serialTimeoutMs))
  {
    delay(smallDelayMs); // Wait up to 5 seconds for serial connection
  }

  // Initial boot message (can't use LogManager yet - not initialized)
  Serial.printf("\n=== Scribe Evolution v%s ===\n", FIRMWARE_VERSION);
  Serial.printf("[BOOT] Built: %s %s\n", BUILD_DATE, BUILD_TIME);
  Serial.printf("[BOOT] System: %s, %d KB free heap\n", ESP.getChipModel(), ESP.getFreeHeap() / mediumJsonBuffer);

  // Initialize LittleFS for config and web file storage
  if (!LittleFS.begin(true, "/littlefs", 10, "littlefs")) // true = format if mount fails
  {
    Serial.println("[BOOT] LittleFS Mount Failed");
  }

  // Validate configuration
  validateConfig();

  // Initialize printer configuration lookup functions
  initializePrinterConfig();

  // Initialize status LED
  initializeStatusLED();

  // Connect to WiFi
  currentWiFiMode = connectToWiFi();

  // Initialize thread-safe singleton managers (MUST be called before any singleton usage)
  LogManager::instance().begin(115200, 256, 512);
  ConfigManager::instance().begin();

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
  // NOTE: Must run BEFORE APIClient/MQTTManager init to ensure correct timestamps in their logs
  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED)
  {
    setupTime();
  }
  else
  {
    LOG_VERBOSE("BOOT", "Skipping NTP sync - no internet connection (AP mode)");
  }

  // Initialize remaining singleton managers (after time is set for correct timestamps)
  APIClient::instance().begin();
  MQTTManager::instance().begin();
  LOG_NOTICE("BOOT", "Thread-safe singleton managers initialized");

  // Record boot time for consistent reporting (after timezone is set)
  deviceBootTime = getISOTimestamp();
  LOG_VERBOSE("BOOT", "Device boot time recorded: %s", deviceBootTime.c_str());

  // Log initial memory status
  LOG_VERBOSE("BOOT", "Free heap: %d bytes", ESP.getFreeHeap());

  // Log detailed GPIO summary in verbose mode (now that logging is available)
  logGPIOUsageSummary();

  // Initialize configuration system and load config from NVS
  if (!initializeConfigSystem())
  {
    LOG_ERROR("BOOT", "Configuration system initialization failed");
  }
  else
  {
    LOG_VERBOSE("BOOT", "Configuration system initialized successfully");
  }

  // Initialize printer hardware
  printerManager.initialize();
  LOG_NOTICE("BOOT", "Printer initialized");

  // Setup mDNS
  setupmDNS();

  // Start MQTT client (if enabled and not in AP mode)
  if (!isAPMode() && isMQTTEnabled())
  {
    startMQTTClient(true); // true = immediate connection on boot (WiFi is already stable)
    LOG_NOTICE("BOOT", "MQTT: Connecting to broker...");
  }
  else
  {
    if (isAPMode())
    {
      LOG_NOTICE("BOOT", "MQTT: Disabled (AP mode)");
    }
    else
    {
      LOG_NOTICE("BOOT", "MQTT: Disabled");
    }
  }

  // Start web server
  setupWebServerRoutes(maxCharacters);
  server.begin();

  // Initialize Unbidden Ink (AI-generated content scheduler)
  initializeUnbiddenInk();

  // Initialize hardware buttons (GPIOs, mutex, state array)
  initializeHardwareButtons();

  // Calculate boot time
  unsigned long bootDuration = millis() - bootStartTime;
  float bootSeconds = bootDuration / 1000.0;

#if ENABLE_LEDS
  // Initialize LED effects system
  if (ledEffects().begin())
  {
    LOG_VERBOSE("BOOT", "LED effects initialized");
  }
  else
  {
    LOG_ERROR("BOOT", "LED effects initialization failed");
  }
#endif

  // Get device name from config
  const RuntimeConfig &config = getRuntimeConfig();
  String deviceName = config.deviceOwner;
  if (deviceName.length() == 0)
  {
    deviceName = "Unknown";
  }

  // Build Web UI URLs
  String webUILine = "    Web UI: ";
  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED)
  {
    const char* mdnsHostname = getMdnsHostname();
    if (mdnsHostname != nullptr && mdnsHostname[0] != '\0')
    {
      // mDNS successfully registered
      webUILine += "http://" + String(mdnsHostname) + ".local | http://" + WiFi.localIP().toString();
    }
    else
    {
      // mDNS registration failed (ERROR already logged by setupmDNS)
      webUILine += "http://" + WiFi.localIP().toString() + " (mDNS failed)";
    }
  }
  else
  {
    // AP mode (mDNS intentionally skipped)
    webUILine += "http://" + WiFi.softAPIP().toString() + " (AP mode)";
  }

  // Print final boot banner
  if (isAPMode())
  {
    LOG_NOTICE("BOOT", "========================================\n    %s Ready (Setup Mode) in %.1f seconds\n%s\n========================================",
               deviceName.c_str(), bootSeconds, webUILine.c_str());
  }
  else
  {
    LOG_NOTICE("BOOT", "========================================\n    %s Ready in %.1f seconds\n%s\n========================================",
               deviceName.c_str(), bootSeconds, webUILine.c_str());
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
  // Trigger boot LED chase effect (1 cycle)
  ledEffects().startEffectCycles("chase_single", 1);
  LOG_VERBOSE("POST_SETUP", "Boot LED effect started");
#endif

  // Print startup message
  printerManager.printStartupMessage();
  LOG_VERBOSE("POST_SETUP", "Startup message printed");
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

  // Check hardware buttons (disabled in AP mode)
  if (!isAPMode())
  {
    checkHardwareButtons();
  }

#if ENABLE_LEDS
  // Update LED effects
  ledEffects().update();
#endif

  // Handle MQTT connection and messages (only in STA mode when MQTT enabled)
  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED && isMQTTEnabled())
  {
    handleMQTTConnection();
    handlePrinterDiscovery();
  }

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
