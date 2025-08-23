/**
 * @file main-button-test.cpp
 * @brief ESP32-C3 Hardware Button GPIO Crash Investigation Test Firmware
 * @description Minimal test to isolate button press crash issue - hardware vs software
 *
 * LOCATION NOTE: This file is stored in the ROOT directory (not src/) to avoid
 * compilation conflicts with main.cpp. Both files define setup() and loop()
 * functions, causing "multiple definition" linker errors if both are in src/.
 *
 * USAGE: To run this test, temporarily replace src/main.cpp with this file:
 *   mv src/main.cpp src/main.cpp.backup
 *   cp main-button-test.cpp src/main.cpp
 *   pio run --target upload -e main
 *
 * This was a TEMPORARY replacement for main.cpp to debug the button crash issue.
 * Successfully identified that crashes were SOFTWARE-related, not hardware.
 *
 * Test approach:
 * 1. Minimal setup - no WiFi, no web server, no complex libraries
 * 2. Direct GPIO setup using Arduino framework
 * 3. Simple polling loop to detect button presses
 * 4. Serial output for monitoring (watchdog-safe)
 * 5. Progressive testing - start with one button, add more if stable
 *
 * Expected outcomes:
 * - If crashes still occur: Hardware issue (wiring, power, GPIO conflicts)
 * - If no crashes: Software issue in original codebase (libraries, interrupts, tasks)
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
 * International License.
 */

#include <Arduino.h>

// Import button configuration from config.h
#include "core/config.h"

// Test configuration
const unsigned long TEST_DURATION_MS = 300000;      // 5 minutes total test
const unsigned long STATUS_REPORT_INTERVAL = 10000; // Status every 10 seconds
const unsigned long BLINK_INTERVAL = 500;           // LED blink every 500ms
const int POLL_INTERVAL_MS = 10;                    // Button polling interval

// GPIO pin assignments from config.h
const int BUTTON_GPIOS[] = {5, 6, 7, 9}; // From defaultButtons[] in config.h
const char *BUTTON_NAMES[] = {"JOKE", "RIDDLE", "QUOTE", "QUIZ"};
const int NUM_BUTTONS = 4;
const int STATUS_LED_PIN = statusLEDPin; // GPIO 8 from config.h

// Test state tracking
bool testButtonStates[NUM_BUTTONS];
bool testLastButtonStates[NUM_BUTTONS];
unsigned long testButtonPressCounts[NUM_BUTTONS] = {0, 0, 0, 0};
unsigned long testLastButtonChangeTime[NUM_BUTTONS] = {0, 0, 0, 0};
unsigned long testStartTime;
unsigned long testLastStatusReport;
unsigned long testLastLEDBlink;
bool testLedState = false;
unsigned long totalLoopCycles = 0;
bool crashDetected = false;

/**
 * @brief Safe serial print with watchdog management
 */
void safePrintln(const String &message)
{
  Serial.println(message);
  Serial.flush();
  yield(); // Allow watchdog reset
}

void safePrint(const String &message)
{
  Serial.print(message);
  yield(); // Allow watchdog reset
}

/**
 * @brief Initialize GPIO pins for button testing
 */
void initializeGPIO()
{
  safePrintln("=== Initializing GPIO for Button Test ===");

  // Initialize status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  safePrintln("Status LED initialized on GPIO " + String(STATUS_LED_PIN));

  // Initialize button GPIOs
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    int gpio = BUTTON_GPIOS[i];

    safePrintln("Initializing Button " + String(i) + " (" + String(BUTTON_NAMES[i]) + ") on GPIO " + String(gpio));

    // Configure as input with pullup (active LOW)
    pinMode(gpio, INPUT_PULLUP);

    // Read initial state
    testButtonStates[i] = digitalRead(gpio);
    testLastButtonStates[i] = testButtonStates[i];
    testLastButtonChangeTime[i] = millis();
    testButtonPressCounts[i] = 0;

    safePrintln("  Initial state: " + String(testButtonStates[i] ? "HIGH (released)" : "LOW (pressed)")); // Small delay between GPIO initializations
    delay(100);
  }

  safePrintln("GPIO initialization complete");
}

/**
 * @brief Process button state changes with debouncing
 */
void processButtons()
{
  unsigned long currentTime = millis();

  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    int gpio = BUTTON_GPIOS[i];
    bool currentState = digitalRead(gpio);

    // Check for state change
    if (currentState != testLastButtonStates[i])
    {
      // Debounce check
      if ((currentTime - testLastButtonChangeTime[i]) > buttonDebounceMs)
      {
        // Valid state change
        bool wasPressed = (testLastButtonStates[i] == LOW); // Active LOW
        bool isPressed = (currentState == LOW);

        if (!wasPressed && isPressed)
        {
          // Button press detected
          testButtonPressCounts[i]++;
          safePrintln("*** BUTTON PRESS *** " + String(BUTTON_NAMES[i]) + " (GPIO " + String(gpio) + ") - Count: " + String(testButtonPressCounts[i]));

          // Flash status LED on button press
          digitalWrite(STATUS_LED_PIN, HIGH);
          delay(50);
          digitalWrite(STATUS_LED_PIN, LOW);
        }
        else if (wasPressed && !isPressed)
        {
          // Button release detected
          safePrintln("Button " + String(BUTTON_NAMES[i]) + " released");
        }

        testLastButtonStates[i] = currentState;
        testLastButtonChangeTime[i] = currentTime;
      }
    }

    // Update current state for next iteration
    testButtonStates[i] = currentState;
  }
}

/**
 * @brief Print status report
 */
void printStatusReport()
{
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - testStartTime;

  safePrintln("\n=== STATUS REPORT ===");
  safePrintln("Test runtime: " + String(elapsed / 1000) + " seconds");
  safePrintln("Loop cycles: " + String(totalLoopCycles));
  safePrintln("Cycles per second: " + String((totalLoopCycles * 1000) / elapsed));

  unsigned long totalPresses = 0;
  for (int i = 0; i < NUM_BUTTONS; i++)
  {
    totalPresses += testButtonPressCounts[i];
    safePrintln("Button " + String(i) + " (" + String(BUTTON_NAMES[i]) + "): " + String(testButtonPressCounts[i]) + " presses");
  }

  safePrintln("Total button presses: " + String(totalPresses));
  safePrintln("Free heap: " + String(ESP.getFreeHeap()) + " bytes");
  safePrintln("Crash detected: " + String(crashDetected ? "YES" : "NO"));

  if (totalPresses > 0)
  {
    safePrintln("✅ SUCCESS: Button presses detected without crashes!");
    safePrintln("This suggests the crash is likely SOFTWARE-related, not hardware.");
  }
  else
  {
    safePrintln("⚠️  No button presses detected yet - press buttons to test");
  }

  safePrintln("=====================\n");
}

/**
 * @brief Arduino setup function
 */
void setup()
{
  Serial.begin(115200);
  delay(2000); // Wait for serial monitor

  safePrintln("");
  for (int i = 0; i < 60; i++)
    safePrint("=");
  safePrintln("");
  safePrintln("ESP32-C3 Hardware Button GPIO Crash Investigation");
  safePrintln("TEMPORARY TEST - Original main.cpp backed up");
  for (int i = 0; i < 60; i++)
    safePrint("=");
  safePrintln("");

  safePrintln("\nTesting Configuration:");
  safePrintln("Button GPIOs: 5, 6, 7, 9 (from config.h)");
  safePrintln("Status LED: GPIO " + String(STATUS_LED_PIN));
  safePrintln("Test Duration: " + String(TEST_DURATION_MS / 1000) + " seconds");
  safePrintln("Button Config: INPUT_PULLUP, Active LOW");
  safePrintln("Debounce Time: " + String(buttonDebounceMs) + "ms");

  safePrintln("\nObjective:");
  safePrintln("- Determine if button crashes are hardware or software related");
  safePrintln("- Test GPIO stability with minimal code");
  safePrintln("- Monitor for ESP32-C3 resets or crashes");
  safePrintln("");

  // Initialize test timing
  testStartTime = millis();
  testLastStatusReport = testStartTime;
  testLastLEDBlink = testStartTime;

  // Initialize GPIO
  initializeGPIO();

  safePrintln("\n🚀 TEST STARTED - Press buttons to test for crashes");
  safePrintln("Serial monitor will show button presses and status reports");
  safePrintln("If ESP32-C3 resets/crashes, the serial output will restart\n");
}

/**
 * @brief Arduino main loop
 */
void loop()
{
  unsigned long currentTime = millis();
  totalLoopCycles++;

  // Check if test duration exceeded
  if ((currentTime - testStartTime) > TEST_DURATION_MS)
  {
    safePrintln("\n");
    for (int i = 0; i < 50; i++)
      safePrint("=");
    safePrintln("");
    safePrintln("TEST COMPLETED - " + String(TEST_DURATION_MS / 1000) + " seconds elapsed");
    printStatusReport();

    unsigned long totalPresses = 0;
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
      totalPresses += testButtonPressCounts[i];
    }

    if (totalPresses > 0)
    {
      safePrintln("🎉 CONCLUSION: Button hardware appears STABLE");
      safePrintln("   The crash issue is likely in the main firmware software:");
      safePrintln("   - Complex library interactions");
      safePrintln("   - Task/interrupt conflicts");
      safePrintln("   - Memory management issues");
      safePrintln("   - Strapping pin GPIO 9 conflicts");
    }
    else
    {
      safePrintln("⚠️  INCONCLUSIVE: No button presses were detected");
      safePrintln("   Check button wiring and connections");
    }

    safePrintln("\nRestore original firmware:");
    safePrintln("mv src/main.cpp.backup src/main.cpp");
    for (int i = 0; i < 50; i++)
      safePrint("=");
    safePrintln("");

    // Blink LED to indicate test completion
    for (int i = 0; i < 10; i++)
    {
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(200);
      digitalWrite(STATUS_LED_PIN, LOW);
      delay(200);
    }

    // Stop the test
    while (true)
    {
      delay(1000);
    }
  }

  // Process button inputs
  processButtons();

  // Status LED heartbeat
  if ((currentTime - testLastLEDBlink) > BLINK_INTERVAL)
  {
    testLedState = !testLedState;
    digitalWrite(STATUS_LED_PIN, testLedState ? HIGH : LOW);
    testLastLEDBlink = currentTime;
  }

  // Periodic status reports
  if ((currentTime - testLastStatusReport) > STATUS_REPORT_INTERVAL)
  {
    printStatusReport();
    testLastStatusReport = currentTime;
  }

  // Small delay to prevent overwhelming the system
  delay(POLL_INTERVAL_MS);

  // Watchdog management
  yield();
}
