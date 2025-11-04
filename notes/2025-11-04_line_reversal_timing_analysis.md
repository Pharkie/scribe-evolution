# Codebase Analysis: Line Reversal Timing Issue on Power-On

**Date:** 2025-11-04
**Issue:** Line reversal (180° rotation) is "consistently missing it on power on somehow"
**Analyzed By:** Claude Code Agent (Read-Only Analysis)

---

## Summary

The line reversal issue appears to be a **timing race condition** between:

1. **Printer initialization** (which sends ESC { 1 rotation command at lines 129-133)
2. **First print operation** (startup message triggered in `postSetup()` at line 331)

The key finding is that there are **minimal delays** (only 650ms total) between printer power-on and the rotation command taking effect, but the **first print happens immediately in the first loop() iteration** via `postSetup()`. There's no explicit delay to ensure the rotation command has been fully processed by the thermal printer hardware before printing begins.

---

## Printer Initialization Sequence

### File: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/hardware/printer.cpp`

#### Timeline of Events (lines 47-137)

```cpp
void PrinterManager::initialize()
{
    // 1. Mutex creation and acquisition (lines 58-75)
    mutex = xSemaphoreCreateMutex();
    ManagerLock lock(mutex, "PRINTER", 10000);

    // 2. eFuse power enable (lines 81-86) - Custom PCB only
    #if BOARD_HAS_EFUSES
    pinMode(BOARD_EFUSE_PRINTER_PIN, OUTPUT);
    digitalWrite(BOARD_EFUSE_PRINTER_PIN, HIGH);
    #endif

    // 3. UART initialization (lines 88-94)
    uart->end();            // Clean state
    delay(100);             // ⏱️ DELAY 1: 100ms
    uart->begin(9600, SERIAL_8N1, config.printerRxPin, config.printerTxPin);

    // 4. DTR configuration (lines 96-102)
    if (config.printerDtrPin != -1) {
        pinMode(config.printerDtrPin, OUTPUT);
        digitalWrite(config.printerDtrPin, HIGH);
    }

    // 5. ESP32-S3 UART stabilization (lines 104-108)
    delay(500);             // ⏱️ DELAY 2: 500ms
    esp_task_wdt_reset();

    // 6. Mark printer ready (line 111)
    ready.store(true, std::memory_order_release);

    // 7. ESC @ Reset command (lines 116-119)
    uart->write(0x1B);
    uart->write('@');
    delay(100);             // ⏱️ DELAY 3: 100ms

    // 8. Heating parameters (lines 121-127)
    uart->write(0x1B);
    uart->write('7');
    uart->write(heatingDots);     // 10 (from system_constants.h:158)
    uart->write(heatingTime);     // 150ms (from system_constants.h:159)
    uart->write(heatingInterval); // 250ms (from system_constants.h:160)
    delay(50);              // ⏱️ DELAY 4: 50ms

    // 9. 180° ROTATION COMMAND (lines 129-133)
    uart->write(0x1B);
    uart->write('{');
    uart->write(0x01);      // ESC { 1 - Enable 180° rotation + line reversal
    delay(50);              // ⏱️ DELAY 5: 50ms ⚠️ POTENTIAL ISSUE

    LOG_VERBOSE("PRINTER", "Printer initialized successfully - ready = %s",
                ready.load() ? "TRUE" : "FALSE");
}
```

**Total Delays During Init:** 100 + 500 + 100 + 50 + 50 = **800ms**
**Delay After Rotation Command:** **50ms only** (line 133)

---

## When First Print Happens

### File: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/main.cpp`

#### Boot Sequence (lines 216-333)

```cpp
void setup() {
    // ... WiFi, time sync, managers ...

    // Line 216: Printer initialization
    printerManager.initialize();  // Sends ESC { 1 at end (50ms delay after)
    LOG_NOTICE("BOOT", "Printer initialized");

    // Line 220-238: mDNS setup, MQTT connection
    setupmDNS();
    if (!isAPMode() && isMQTTEnabled()) {
        startMQTTClient(true);  // ⚠️ MQTT starts here - can receive messages!
    }

    // Line 240-248: Web server start
    setupWebServerRoutes(maxCharacters);
    server.begin();

    // Line 250-312: Boot completion logs, LED init, boot banner
    // NO explicit delay between printer init and loop()
}

// Lines 315-333: Post-setup function (runs FIRST in loop())
void postSetup() {
    #if ENABLE_LEDS
    ledEffects().startEffectCycles("chase_single", 1);
    #endif

    // Line 331: ⚠️ FIRST PRINT HAPPENS HERE
    printerManager.printStartupMessage();
    LOG_VERBOSE("POST_SETUP", "Startup message printed");
}

// Lines 335-409: Main loop
void loop() {
    static bool firstRun = true;
    if (firstRun) {
        postSetup();  // Triggers printStartupMessage() immediately
        firstRun = false;
    }
    // ... rest of loop ...
}
```

#### Timing Between Rotation Command and First Print

1. **Line 216:** `printerManager.initialize()` completes
   - ESC { 1 sent at **t=0**
   - 50ms delay after command (line 133 in printer.cpp)
2. **Lines 220-312:** Various setup tasks (mDNS, MQTT, web server, logs)
   - **Estimated duration:** ~200-500ms (highly variable, depends on network)
3. **Line 340:** First `loop()` iteration starts
4. **Line 331:** `printStartupMessage()` executes **IMMEDIATELY**

**Critical Gap:** The delay between ESC { 1 and first print is **non-deterministic** and depends on:

- mDNS registration time
- MQTT connection attempt (may timeout)
- Web server route setup
- Boot logging overhead

**On a fast boot (no MQTT delays), this could be < 300ms total.**

---

## Potential Race Condition

### Thermal Printer Command Processing

CSN-A4L thermal printers (and similar models) process commands **asynchronously**:

- ESC { 1 (rotation command) sets an internal flag in the printer's firmware
- The printer may need **100-200ms** to process mode changes
- If data arrives before the command takes effect, it prints with the old settings

### Evidence of Race Condition

From the code:

1. **Line 133 (printer.cpp):** Only **50ms delay** after ESC { 1
2. **Line 331 (main.cpp):** `printStartupMessage()` happens **ASAP** in first loop
3. **No explicit stabilization delay** between rotation command and first print

**Likely Scenario:**

- On power-on boot, printer hardware is still initializing
- ESC { 1 is sent after 650ms (UART init delays)
- Boot continues quickly (~200ms more for setup tasks)
- First print arrives **before printer firmware fully applies rotation mode**
- Result: First startup message prints upside-down/wrong line order

---

## MQTT Early Print Risk

### File: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/main.cpp`

**Line 225:** MQTT client starts **before first local print**

```cpp
startMQTTClient(true); // true = immediate connection on boot
```

### File: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/core/mqtt_handler.cpp`

**Line 193:** MQTT messages trigger **immediate printing** via `printerManager.printWithHeader()`

```cpp
void MQTTManager::handleMQTTMessageInternal(String topic, String message)
{
    // ... parse JSON message ...

    // Line 193: Print immediately using printerManager
    printerManager.printWithHeader(timestamp, printMessage);
}
```

**Risk Timeline:**

1. **t=0:** Printer initialized, ESC { 1 sent (50ms delay)
2. **t=50ms:** MQTT client starts connecting
3. **t=100ms:** MQTT connects (fast network)
4. **t=150ms:** **MQTT message arrives** → prints immediately
5. **t=200ms:** `postSetup()` prints startup message

**If an MQTT message arrives between t=100-300ms, it could print BEFORE the rotation command fully takes effect.**

---

## Current Logging for Debugging

### Printer Initialization (printer.cpp)

**Line 113-114:** Verbose log for UART init

```cpp
LOG_VERBOSE("PRINTER", "UART initialized (TX=%d, RX=%d, DTR=%d)",
            config.printerTxPin, config.printerRxPin, config.printerDtrPin);
```

**Line 135-136:** Verbose log for completion

```cpp
LOG_VERBOSE("PRINTER", "Printer initialized successfully - ready = %s",
            ready.load() ? "TRUE" : "FALSE");
```

**Missing Logs:**

- ❌ No log for ESC @ reset command (line 116-119)
- ❌ No log for heating parameters (line 121-127)
- ❌ **No log for ESC { 1 rotation command** (line 129-133) ⚠️ CRITICAL
- ❌ No timestamp logs to measure actual timing

### Print Operations (printer.cpp)

**Line 251:** Notice-level log for every print

```cpp
LOG_NOTICE("PRINTER", "Printing: %s", preview.c_str());
```

**Line 378-379:** Verbose log when `printMessage()` called

```cpp
LOG_VERBOSE("PRINTER", "printMessage() called - printerReady = %s",
            printerManager.isReady() ? "TRUE" : "FALSE");
```

**Line 301, 342:** Verbose logs for startup message

```cpp
LOG_VERBOSE("PRINTER", "Printing AP setup message");
LOG_VERBOSE("PRINTER", "Printing startup message");
```

---

## Additional Print Triggers

### 1. Hardware Buttons (content_handlers.cpp:358)

```cpp
currentMessage.shouldPrintLocally = true;
```

**Risk:** Low (user-initiated, happens after boot)

### 2. MQTT Messages (mqtt_handler.cpp:193)

```cpp
printerManager.printWithHeader(timestamp, printMessage);
```

**Risk:** **HIGH** - Can happen **immediately after MQTT connects** (line 225 in main.cpp)

### 3. Web API Calls (content_actions.cpp:150)

```cpp
currentMessage.shouldPrintLocally = true;
```

**Risk:** Low (requires web server to be fully ready)

---

## Recommendations for Main Agent

### 1. Add Stabilization Delay After ESC { 1

**Location:** `printer.cpp:133`

Current:

```cpp
uart->write(0x01); // ESC { 1
delay(50);          // Too short!
```

Suggested:

```cpp
uart->write(0x01); // ESC { 1
delay(200);         // Increased: 50ms → 200ms for hardware settling
LOG_VERBOSE("PRINTER", "180° rotation command sent - waiting for hardware");
```

**Reasoning:**

- 50ms may be insufficient for printer firmware to process mode change
- 200ms aligns with typical thermal printer command processing time
- Minimal impact on boot time (additional 150ms)

### 2. Add Explicit Delay Before First Print

**Location:** `main.cpp:331` (in `postSetup()`)

Current:

```cpp
void postSetup() {
    printerManager.printStartupMessage();  // Immediate!
}
```

Suggested:

```cpp
void postSetup() {
    // Ensure rotation command has fully taken effect
    delay(100);  // Additional safety margin after setup() completes
    LOG_VERBOSE("POST_SETUP", "Starting first print (rotation stabilized)");
    printerManager.printStartupMessage();
}
```

### 3. Add Verbose Logging for Rotation Command

**Location:** `printer.cpp:129-136`

Add timestamp logging:

```cpp
LOG_VERBOSE("PRINTER", "Sending ESC @ reset command");
uart->write(0x1B); uart->write('@');
delay(100);

LOG_VERBOSE("PRINTER", "Sending heating parameters (dots=%d, time=%dms, interval=%dms)",
            heatingDots, heatingTime, heatingInterval);
uart->write(0x1B); uart->write('7');
// ... heating params ...
delay(50);

LOG_VERBOSE("PRINTER", "Sending ESC { 1 (180° rotation + line reversal)");
uart->write(0x1B); uart->write('{'); uart->write(0x01);
delay(200);  // Increased delay
LOG_VERBOSE("PRINTER", "Rotation command sent - hardware stabilizing");
```

### 4. Defer MQTT Connection Until After First Print

**Location:** `main.cpp:225`

Current sequence:

1. Printer init
2. MQTT start (can receive messages immediately)
3. First loop → print startup message

Safer sequence:

1. Printer init
2. First loop → print startup message
3. MQTT start (after first print completes)

Move MQTT start to **after `postSetup()`** completes:

```cpp
void loop() {
    static bool firstRun = true;
    if (firstRun) {
        postSetup();  // Print startup message first

        // NOW start MQTT (after first print is done)
        if (!isAPMode() && isMQTTEnabled()) {
            startMQTTClient(true);
            LOG_NOTICE("POST_SETUP", "MQTT client started after first print");
        }

        firstRun = false;
    }
    // ... rest of loop ...
}
```

**Trade-off:** MQTT connection delayed by ~1-2 seconds, but eliminates race condition.

---

## File Relationships Map

```
main.cpp (setup)
├─→ Line 216: printerManager.initialize()
│   └─→ printer.cpp:47-137
│       ├─→ Lines 88-94: UART init (delay 100ms)
│       ├─→ Line 105: ESP32-S3 delay (500ms)
│       ├─→ Lines 116-119: ESC @ reset (delay 100ms)
│       ├─→ Lines 121-127: Heating params (delay 50ms)
│       └─→ Lines 129-133: ESC { 1 rotation (delay 50ms) ⚠️
│
├─→ Line 225: startMQTTClient(true)
│   └─→ mqtt_handler.cpp:onConnectionEstablished()
│       └─→ mqtt_handler.cpp:193: printerManager.printWithHeader() ⚠️
│
└─→ Line 312: setup() completes

main.cpp (loop)
└─→ Line 340: First loop iteration
    └─→ Line 331: postSetup() → printStartupMessage() ⚠️
        └─→ printer.cpp:279-369
            └─→ printer.cpp:232: printWithHeader()
                └─→ printer.cpp:221-226: printWrappedInternal()
                    └─→ Prints lines in REVERSE order (assumes rotation active)
```

**Key Paths:**

- **Critical Path 1:** Init → ESC { 1 (50ms delay) → First Print (< 500ms later)
- **Critical Path 2:** MQTT Connect → Message Arrives → Print (could be < 200ms)

---

## Key Observations

1. **Insufficient Delay After Rotation Command**
   - Only 50ms after ESC { 1 (line 133 in printer.cpp)
   - Thermal printers typically need 100-200ms for mode changes
   - First print can happen within 200-500ms on fast boot

2. **Non-Deterministic Timing**
   - Time between rotation command and first print varies by:
     - MQTT connection speed (can timeout)
     - mDNS registration time
     - Network latency
   - Makes issue intermittent ("consistently" on cold boot, but not always reproducible)

3. **MQTT Race Condition**
   - MQTT client starts **before first print** (line 225 vs line 331)
   - If message arrives quickly, prints **before startup message**
   - No guarantee rotation command has taken effect yet

4. **No Timing Verification**
   - No logs for rotation command being sent
   - No timestamp tracking between init and first print
   - Hard to diagnose timing issues without instrumentation

5. **Hardware-Specific Behavior**
   - Custom PCB has eFuse power control (line 82-86)
   - Mini boards don't (power already on)
   - **Hypothesis:** Custom PCB cold start may need more settling time

---

## Additional Notes for Main Agent

- **Testing Strategy:** Add verbose logging with timestamps, measure actual delay between ESC { 1 and first print on cold boot
- **Verification:** After increasing delays, test with:
  - Cold power-on (printer fully off)
  - Warm reset (ESP32 reset, printer stays on)
  - Fast MQTT message arrival (< 2s after boot)
- **Regression Risk:** Increasing delays adds ~200-300ms to boot time (acceptable trade-off)

---

**Analysis Complete.**
**Findings documented in:** `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/notes/2025-11-04_line_reversal_timing_analysis.md`

**For main agent review.**
