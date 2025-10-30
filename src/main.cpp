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
#include <esp_bt.h>
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
  // === Disable Bluetooth/BLE to try and free heap ===
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM); // Release both BT Classic + BLE

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
  Serial.printf("[BOOT] Built: %s %s for %s\n", BUILD_DATE, BUILD_TIME, BOARD_NAME);

  // Early heap fragmentation check
  size_t freeHeap = ESP.getFreeHeap();
  size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  Serial.printf("[BOOT] System: %s, %d KB free heap, %d KB largest block\n",
                ESP.getChipModel(), freeHeap / 1024, largestBlock / 1024);

  // Initialize LittleFS for config and web file storage
  // CRITICAL: ESP32-Arduino uses "spiffs" partition type (0x82) but we name it "littlefs"
  // This is standard ESP32 practice - partition subtype is always "spiffs" for any filesystem

  // First attempt: Try mounting existing filesystem (from uploadfs)
  Serial.println("[BOOT] Attempting to mount LittleFS partition 'littlefs'...");
  bool littlefsOk = LittleFS.begin(false, "/littlefs", 10, "littlefs");

  if (!littlefsOk)
  {
    Serial.println("[BOOT] Mount failed - filesystem may not be initialized");
    Serial.println("[BOOT] This is normal on first boot or after erase");
    Serial.println("[BOOT] Attempting to format partition...");

    // Format the partition from scratch
    if (LittleFS.format())
    {
      Serial.println("[BOOT] Format successful - retrying mount...");
      littlefsOk = LittleFS.begin(false, "/littlefs", 10, "littlefs");

      if (littlefsOk)
      {
        Serial.println("[BOOT] ✅ LittleFS mounted after format");
        Serial.println("[BOOT] ⚠️  Web interface files missing - run 'pio run --target uploadfs -e s3-pcb-dev'");
      }
      else
      {
        Serial.println("[BOOT] ❌ Mount failed even after format - partition may be corrupted");
      }
    }
    else
    {
      Serial.println("[BOOT] ❌ Format failed - possible hardware or partition table issue");
      Serial.println("[BOOT] Try: pio run --target erase -e s3-pcb-dev (erases entire flash)");
    }
  }
  else
  {
    Serial.println("[BOOT] ✅ LittleFS mounted successfully (files present)");
  }

  // Continue boot regardless of filesystem status (device still works, just no web UI)
  if (!littlefsOk)
  {
    Serial.println("[BOOT] ⚠️  Continuing without filesystem - API-only mode");
  }

  // Initialize ConfigManager first (required for loading NVS config)
  ConfigManager::instance().begin();

  // Validate configuration (loads from NVS - requires ConfigManager to be initialized)
  validateConfig();

  // Initialize printer configuration lookup functions
  initializePrinterConfig();

  // Initialize status LED
  initializeStatusLED();

  // Connect to WiFi (uses config loaded from NVS above)
  currentWiFiMode = connectToWiFi();

  // Initialize LogManager after WiFi (needs to know connection mode for logging context)
  LogManager::instance().begin(115200, 8, 512); // Queue: 256→8 entries (saves 124KB)

  // Configure ESP32 system component log levels
  // CORE_DEBUG_LEVEL is set in platformio.ini (varies by build type)
  esp_log_level_set("WebServer", static_cast<esp_log_level_t>(CORE_DEBUG_LEVEL));
#ifdef RELEASE_BUILD
  // Suppress VFS errors in production (AsyncWebServer checks for uncompressed files before serving .gz)
  esp_log_level_set("vfs", ESP_LOG_NONE);
#endif

  // Log logging configuration (LogManager is now ready)
  // APP_LOG_LEVEL is set in platformio.ini (varies by build type)
  LOG_VERBOSE("BOOT", "Logging configured - Level: %s (serial output only)",
              getLogLevelString(APP_LOG_LEVEL).c_str());

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
  // Run BEFORE APIClient/MQTTManager init to ensure correct timestamps in their logs
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

  // Log initial memory status with fragmentation
  size_t heapAfterInit = ESP.getFreeHeap();
  size_t largestAfterInit = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  LOG_NOTICE("BOOT", "Heap after init: %d KB free, %d KB largest block",
             heapAfterInit / 1024, largestAfterInit / 1024);

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

  // Check heap after web server setup (route handlers fragment heap)
  size_t heapAfterWeb = ESP.getFreeHeap();
  size_t largestAfterWeb = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  LOG_NOTICE("BOOT", "Heap after web server: %d KB free, %d KB largest block",
             heapAfterWeb / 1024, largestAfterWeb / 1024);

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
    const char *mdnsHostname = getMdnsHostname();
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
    LOG_NOTICE("BOOT", "\n========================================\n    %s Ready (Setup Mode) in %.1f seconds\n%s\n========================================",
               deviceName.c_str(), bootSeconds, webUILine.c_str());
  }
  else
  {
    LOG_NOTICE("BOOT", "\n========================================\n    %s Ready in %.1f seconds\n%s\n========================================",
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
