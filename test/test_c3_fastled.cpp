/**
 * ESP32-C3 MINIMAL TEST - MATCH MAIN APP EXACTLY
 *
 * Purpose: Replicate main.cpp initialization sequence to trigger the same crash
 *
 * Strategy: Use ALL REAL components from main app in the SAME ORDER:
 * 1. REAL ConfigManager (not Simple*)
 * 2. REAL PrinterManager (not Simple*)
 * 3. REAL LedEffects
 * 4. REAL LogManager
 * 5. REAL web server with 40+ routes
 * 6. REAL Unbidden Ink
 * 7. Everything else from main app
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <ezTime.h>
#include <esp_task_wdt.h>
#include <esp_log.h>

// Include ALL the real systems from main.cpp
#include <config/config.h>
#include <core/config_loader.h>
#include <core/config_utils.h>
#include <core/shared_types.h>
#include <web/web_server.h>
#include <core/network.h>
#include <hardware/printer.h>
#include <core/mqtt_handler.h>
#include <core/printer_discovery.h>
#include <utils/time_utils.h>
#include <core/logging.h>
#include <core/LogManager.h>
#include <hardware/hardware_buttons.h>
#include <content/content_generators.h>
#include <utils/api_client.h>
#include <content/unbidden_ink.h>
#include <leds/LedEffects.h>

// === Web Server === (matches main.cpp)
AsyncWebServer server(webServerPort);

// === Memory Monitoring Variables === (matches main.cpp)
unsigned long lastMemCheck = 0;

// === Boot Time Tracking === (matches main.cpp)
String deviceBootTime = "";

void setup() {
  // === EXACT COPY OF main.cpp setup() ===

  // Track boot time
  unsigned long bootStartTime = millis();

  // Initialize serial communication first (USB CDC)
  Serial.begin(115200);

  // Wait for USB CDC connection (like main app)
  unsigned long serialStartTime = millis();
  while (!Serial && (millis() - serialStartTime < serialTimeoutMs))
  {
    delay(smallDelayMs);
  }

  // Note: We can't use Log.notice() yet as logging isn't initialized
  Serial.printf("\n=== Scribe Evolution v%s ===\n", FIRMWARE_VERSION);
  Serial.printf("[BOOT] Built: %s %s\n", BUILD_DATE, BUILD_TIME);
  Serial.printf("[BOOT] System: %s, %d KB free heap\n", ESP.getChipModel(), ESP.getFreeHeap() / mediumJsonBuffer);

  // Initialize LittleFS so config loading works
  if (!LittleFS.begin(true, "/littlefs", 10, "littlefs"))
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

  // Initialize LogManager - provides thread-safe single-writer logging
  LogManager::instance().begin(115200, 256, 512);

  // Initialize APIClient - provides thread-safe HTTP operations
  APIClient::instance().begin();

  // Initialize ConfigManager - provides thread-safe NVS/LittleFS operations
  ConfigManager::instance().begin();

  // Initialize MQTTManager - provides thread-safe MQTT operations
  MQTTManager::instance().begin();

  // Configure ESP32 system component log levels
  esp_log_level_set("WebServer", espLogLevel);
#ifdef RELEASE_BUILD
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
  printerManager.initialize();

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
  if (ledEffects().begin())
  {
    LOG_VERBOSE("BOOT", "LED effects system initialized successfully");

    // PHASE 2 TEST: Run boot effect and test FastLED state after it completes
    LOG_NOTICE("BOOT", "🧪 PHASE 2: Testing FastLED state after boot effect");

    ledEffects().startEffectCycles("chase_single", 1, 0x0000FF);
    LOG_VERBOSE("BOOT", "Boot LED effect started (chase_single, 1 cycle)");

    // Wait for the 1-cycle boot effect to complete
    LOG_VERBOSE("BOOT", "Waiting for boot effect to complete...");
    delay(1000);  // Give it time to finish 1 cycle (30 LEDs @ 60Hz ~= 500ms)

    // Check if effect is still active (using private member, just for debugging)
    // LOG_VERBOSE("BOOT", "Effect still active: %s", ledEffects().isEffectActive() ? "YES" : "NO");

    // Stop the effect explicitly
    LOG_VERBOSE("BOOT", "Stopping LED effect explicitly...");
    ledEffects().stopEffect();
    delay(100);

    // Test direct FastLED.show() after boot effect
    LOG_NOTICE("BOOT", "🧪 Testing direct FastLED.show() after boot effect...");
    FastLED.show();
    LOG_NOTICE("BOOT", "✅ Direct FastLED.show() succeeded after boot effect");

    // Clear LEDs
    LOG_VERBOSE("BOOT", "Clearing all LEDs...");
    FastLED.clear();
    FastLED.show();
    LOG_VERBOSE("BOOT", "✅ LEDs cleared");
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
    startMQTTClient(true);
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

  // Initialize Unbidden Ink schedule
  initializeUnbiddenInk();

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

void loop() {
  // === EXACT COPY OF main.cpp loop() ===

  // Feed the watchdog
  esp_task_wdt_reset();

  // Process ezTime events (for timezone updates)
  events();

  // Check WiFi connection and reconnect if needed
  handleWiFiReconnection();

  // Handle DNS server for captive portal in AP mode
  handleDNSServer();

  // Check for button presses (only in STA mode)
  if (!isAPMode())
  {
    checkHardwareButtons();
  }

#if ENABLE_LEDS
  ledEffects().update();  // ← CRASH EXPECTED HERE (matching main app)
#endif

  // Handle MQTT connection and messages (only in STA mode when MQTT enabled)
  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED && isMQTTEnabled())
  {
    handleMQTTConnection();
    handlePrinterDiscovery();
  }

  // Check if we have a new message to print
  bool shouldPrint = false;
  if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(10)) == pdTRUE)
  {
    shouldPrint = currentMessage.shouldPrintLocally;
    xSemaphoreGive(currentMessageMutex);
  }

  if (shouldPrint)
  {
    LOG_VERBOSE("MAIN", "Printing message from main loop");
    printMessage(); // printMessage() will acquire mutex internally

    // Clear the flag after printing
    if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      currentMessage.shouldPrintLocally = false;
      xSemaphoreGive(currentMessageMutex);
    }
  }

  // Check for unbidden ink scheduled print
  if (currentWiFiMode == WIFI_MODE_STA_CONNECTED)
  {
    checkUnbiddenInk();
  }

  // Periodic memory monitoring
  if (millis() - lastMemCheck > memCheckIntervalMs)
  {
    lastMemCheck = millis();
    int freeHeap = ESP.getFreeHeap();
    LOG_VERBOSE("MEM", "Free heap: %d bytes", freeHeap);
  }
}
