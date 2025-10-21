# ESP32-S3 Architecture Changes Documentation

**From commit:** `186f0ff` (docs update, Oct 8 2025)
**To commit:** `34ed64e` (current HEAD)
**Date:** October 2025
**Purpose:** Document major architectural changes made to support ESP32-S3 that broke ESP32-C3 compatibility

---

## Executive Summary

This document chronicles the significant architectural refactoring undertaken to support ESP32-S3 variants (S3-mini and S3-custom-PCB with eFuse). While these changes successfully enabled S3 support, they introduced a **critical incompatibility on ESP32-C3** where `pinMode()` calls corrupt the RMT peripheral used by FastLED.

### Key Issue: ESP32-C3 pinMode() + FastLED Incompatibility

**Symptom:** Calling `pinMode()` on ANY GPIO (regardless of which specific pins or order) causes `FastLED.show()` to crash with "Guru Meditation Error: Core 0 panic'ed (Load access fault)" on ESP32-C3.

**Root Cause:** Arduino's `pinMode()` function (which calls ESP-IDF's `gpio_config()`) globally corrupts or interferes with the RMT peripheral state on ESP32-C3, even when pinMode is called on different GPIOs than those used by FastLED.

**Tests Performed:**

- ✅ pinMode() on GPIOs 4,5,6,7 → Crash
- ✅ pinMode() before ledEffects().begin() → Crash
- ✅ pinMode() on non-RTC GPIOs (6,7,10,2) → Crash
- ✅ pinMode() on GPIOs 5,6,7,8 → Crash
- ✅ No pinMode() at all → **WORKS**

**Conclusion:** The issue is fundamental to ESP32-C3's implementation, not specific GPIO selection or initialization order.

---

## Major Architectural Changes (186f0ff → HEAD)

### 1. Multi-Board GPIO Configuration System (commits 2a45519, ce6b58f, 2e7982f)

**Created:** Board-specific configuration system to support multiple ESP32 variants

#### New Files:

- `src/config/boards/board_interface.h` - Common interface for all boards
- `src/config/boards/board_esp32c3_mini.h` - ESP32-C3-mini configuration
- `src/config/boards/board_esp32s3_supermini.h` - ESP32-S3-supermini configuration
- `src/config/boards/board_esp32s3_custom_pcb.h` - ESP32-S3-custom-PCB with eFuse support
- `src/config/boards/board_config.h` - Compile-time board selection
- `src/config/boards/CLAUDE.md` - Documentation for board system

#### Deleted Files:

- `src/config/gpio_map.h` - Replaced by board-specific files

#### Key Features:

- **Compile-time board selection** via PlatformIO build flags (`BOARD_ESP32C3_MINI`, `BOARD_ESP32S3_SUPERMINI`, etc.)
- **Board-specific GPIO validation** (strapping pins, safe pins, avoid pins)
- **eFuse support** for custom PCB (printer power, LED power control)
- **Runtime board mismatch detection** with automatic GPIO reset to safe defaults
- **Centralized pin defaults** for each board variant

#### Board Characteristics:

**ESP32-C3-mini:**

- Single RISC-V core @ 160MHz
- GPIOs 0-21 (22 total)
- 4 RMT channels
- RTC GPIOs: 0-5
- Strapping pins: 0, 9
- Built-in LED: GPIO 8

**ESP32-S3-supermini:**

- Dual Xtensa cores @ 240MHz
- GPIOs 0-48 (limited availability)
- 8 RMT channels
- RTC GPIOs: 0-21
- Strapping pins: 0, 45, 46
- Built-in LED: GPIO 47

**ESP32-S3-custom-PCB:**

- Same as S3-supermini
- **eFuse pins:** GPIO 35 (printer power), GPIO 36 (LED power)
- Hardware power control for peripherals

---

### 2. Thread-Safe Singleton Pattern (commits e2542f5, 2b906fd, f52d81a)

**Problem:** ESP32-S3's dual-core architecture requires thread-safe access to shared resources.

**Solution:** Implemented Meyer's singleton pattern with mutex protection for all managers.

#### LogManager (NEW: `src/core/LogManager.cpp`, `src/core/LogManager.h`)

**Created:** Dedicated logging manager with queue-based async logging

**Key Features:**

- **Queue-based logging** (256 message queue, 512 byte max line)
- **Dedicated writer task** for non-blocking log output
- **Thread-safe** via mutex protection
- **Serial-only output** (removed file logging complexity)
- **Singleton access** via `LogManager::instance()`

**Migration:**

```cpp
// OLD (direct functions)
LOG_ERROR("TAG", "Message");

// NEW (still works - backward compatible wrapper)
LOG_ERROR("TAG", "Message");  // Calls LogManager::instance().log() internally
```

**Why:** Prevents log message corruption when multiple cores write simultaneously.

#### ConfigManager Refactor (`src/core/config_loader.cpp`, `src/core/config_loader.h`)

**Changes:**

- **Thread-safe singleton** via `ConfigManager::instance()`
- **Mutex-protected** NVS and LittleFS operations
- **Runtime board detection** and GPIO validation
- **Automatic board mismatch handling**

**Migration:**

```cpp
// OLD (global functions)
const RuntimeConfig &config = getRuntimeConfig();

// NEW (singleton pattern)
const RuntimeConfig &config = ConfigManager::instance().getRuntimeConfig();

// LEGACY WRAPPER (still works)
const RuntimeConfig &config = getRuntimeConfig();  // Calls singleton internally
```

**New Features:**

- Board type validation on boot
- Automatic GPIO reset if board mismatch detected
- Thread-safe config save/load operations

#### APIClient Refactor (`src/utils/api_client.cpp`, `src/utils/api_client.h`)

**Changes:**

- **Thread-safe singleton** via `APIClient::instance()`
- **Mutex-protected** WiFiClientSecure and HTTPClient operations
- **Connection pooling** with automatic cleanup
- **Retry logic** with exponential backoff

**Migration:**

```cpp
// OLD (global shared instances - NOT THREAD SAFE)
extern WiFiClientSecure secureClient;
httpClient.begin(secureClient, url);

// NEW (singleton pattern)
APIClient::instance().get(url, [](String response) {
    // Handle response
});

// LEGACY WRAPPER (still works)
makeApiRequest(url, callback);  // Calls singleton internally
```

**Why:** Prevents race conditions when multiple tasks make HTTP requests simultaneously.

#### MQTTManager Refactor (`src/core/mqtt_handler.cpp`, `src/core/mqtt_handler.h`)

**Changes:**

- **Thread-safe singleton** via `MQTTManager::instance()`
- **Mutex-protected** PubSubClient and WiFiClientSecure operations
- **Reconnection logic** with exponential backoff
- **Message queuing** for reliable delivery
- **Subscribe/publish API** with automatic reconnection

**Migration:**

```cpp
// OLD (global functions)
setupMQTTClient();
mqttClient.publish(topic, message);

// NEW (singleton pattern)
MQTTManager::instance().begin();
MQTTManager::instance().publish(topic, message);

// LEGACY WRAPPER (still works)
setupMQTTClient();  // Calls singleton internally
publishMqttMessage(topic, message);  // Calls singleton internally
```

**New Features:**

- Automatic reconnection with backoff
- Message acknowledgment tracking
- Connection state management
- Thread-safe message publishing

#### PrinterManager Refactor (`src/hardware/printer.cpp`, `src/hardware/printer.h`)

**Changes:**

- **Thread-safe operations** via mutex protection
- **Queue-based printing** to prevent UART corruption
- **Error recovery** with retry logic
- **Power management** (eFuse support for custom PCB)

**Migration:**

```cpp
// OLD (direct UART writes - NOT THREAD SAFE)
printToThermalPrinter("text");

// NEW (managed printing)
printerManager.printText("text");  // Thread-safe, queued

// eFuse support (custom PCB only)
printerManager.setPrinterPower(true);  // Control via eFuse
```

**Why:** Prevents UART corruption when multiple tasks try to print simultaneously.

---

### 3. LedEffects System Overhaul (commits 9eda03b, c68a3ab, 63cbb96, 3456cff)

**Problem:** Original LED system had race conditions and memory allocation issues on dual-core S3.

**Solution:** Complete rewrite with cycle-based effects, static allocation, and thread-safety.

#### Key Changes (`src/leds/LedEffects.cpp`, `src/leds/LedEffects.h`):

**Static LED Array:**

```cpp
// OLD (heap allocation - caused crashes)
CRGB* leds = new CRGB[ledCount];

// NEW (static allocation)
extern CRGB staticLEDs[MAX_LEDS];  // Defined in LedEffects.cpp
CRGB* leds = staticLEDs;  // Use pre-allocated array
```

**Cycle-Based Effects:**

```cpp
// OLD (duration-based with timers)
startEffect("chase", 5000);  // Run for 5 seconds

// NEW (cycle-based)
startEffectCycles("chase_single", 3);  // Run for 3 complete cycles
```

**Thread-Safe Singleton:**

```cpp
// OLD (global instance)
extern LedEffects ledEffectsManager;

// NEW (singleton pattern)
LedEffects& ledEffects() {
    static LedEffects instance;
    return instance;
}

// Usage
ledEffects().begin();
ledEffects().startEffectCycles("chase_single", 1);
ledEffects().update();  // Call in loop()
```

**Effect Registry:**

- Removed static effect lists
- Dynamic effect registration
- Cleaner effect lifecycle management

**New Effects:**

- `chase_single` - Single LED chasing down the strip
- `chase_multi` - Multiple LEDs chasing
- `pulse` - Pulsing brightness effect
- `rainbow` - Rainbow wave effect
- `twinkle` - Random twinkling stars
- `matrix` - Matrix-style falling code

**RMT Configuration:**

- Switch-case for GPIO pin assignment (FastLED requirement)
- Automatic RMT channel allocation
- Board-specific pin validation

**Migration:**

```cpp
// OLD
#include "LedEffects.h"
extern LedEffects ledEffectsManager;
ledEffectsManager.begin();
ledEffectsManager.startEffect("chase", 5000);

// NEW
#include "LedEffects.h"
ledEffects().begin();
ledEffects().startEffectCycles("chase_single", 3);

// In loop()
ledEffects().update();  // Must be called every loop iteration
```

---

### 4. Hardware Button System Changes (commits 0470a85, 6c7b2af, a368f63, 4db7a97)

**Problem:** Action queue system was overly complex and caused timing issues.

**Solution:** Immediate action execution with configurable LED feedback.

#### Key Changes (`src/hardware/hardware_buttons.cpp`):

**Removed:**

- Button action queue system
- Delayed action execution
- Complex state machine

**Added:**

- **Immediate action execution** when button pressed
- **Configurable LED effects** per button action
- **GPIO validation** using board-specific constraints
- **pinMode() calls** ← **THIS BREAKS ESP32-C3**

**Button Configuration:**

```cpp
// Board-specific button defaults (src/config/boards/board_esp32c3_mini.h)
static const ButtonConfig ESP32C3_DEFAULT_BUTTONS[] = {
    {4, "JOKE", "", "chase_single", "CHARACTER_TEST", "", "pulse"},
    {5, "RIDDLE", "", "chase_single", "", "", "pulse"},
    {6, "QUOTE", "", "chase_single", "", "", "pulse"},
    {7, "QUIZ", "", "chase_single", "", "", "pulse"}
};
```

**GPIO Initialization:**

```cpp
// WORKS ON S3, CRASHES ON C3
for (int i = 0; i < numHardwareButtons; i++) {
    int gpio = config.buttonGpios[i];
    pinMode(gpio, buttonActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);  // ← BREAKS C3
    buttonStates[i].currentState = digitalRead(gpio);
    // ...
}
```

**Why it breaks C3:**

- Arduino's `pinMode()` calls `gpio_config()` internally
- `gpio_config()` on ESP32-C3 corrupts RMT peripheral state globally
- Even when called on different GPIOs than FastLED uses
- Regardless of initialization order (before or after FastLED)
- Affects ALL GPIO pins, not just RTC GPIOs

**Button Actions:**

```cpp
// src/utils/content_actions.cpp
void executeContentAction(const String &action, const String &ledEffect, int ledCycles) {
    // Trigger LED effect
    if (ledEffect != "" && ledCycles > 0) {
        ledEffects().startEffectCycles(ledEffect, ledCycles);
    }

    // Execute content action (JOKE, RIDDLE, etc.)
    // ...
}
```

#### GPIO 9 → GPIO 4 Fix (commit 4db7a97)

**Problem:** GPIO 9 is a strapping pin on ESP32-C3, caused instant crashes.

**Solution:** Replaced button GPIO 9 with GPIO 4 (safe pin).

```cpp
// OLD (CRASHED on boot)
{9, "QUIZ", ...}  // GPIO 9 is strapping pin

// NEW (works)
{4, "QUIZ", ...}  // GPIO 4 is safe
```

---

### 5. Library Updates (commit 1f554e7)

**Updated:**

- FastLED → 3.10.3
- ESPAsyncWebServer-esphome → 3.4.0
- AsyncTCP-esphome → 2.1.4
- PubSubClient → 2.8.0
- **ArduinoJson → 7.4.2** (successfully upgraded from v6)

---

### 6. Main Application Flow Changes (`src/main.cpp`)

**Initialization Order (CRITICAL for ESP32-S3):**

```cpp
void setup() {
    // 1. Hardware initialization
    Serial.begin(115200);

    // 2. Board detection and GPIO validation
    detectAndValidateBoard();

    // 3. Initialize singletons
    LogManager::instance().begin(115200, 256, 512);
    APIClient::instance().begin();
    ConfigManager::instance().begin();
    MQTTManager::instance().begin();

    // 4. Initialize config system (loads from NVS)
    initializeConfigSystem();

    // 5. Initialize printer
    printerManager.initialize();

    // 6. Initialize button GPIOs ← BREAKS C3
    if (!isAPMode()) {
        initializeHardwareButtons();  // Calls pinMode() internally
    }

    // 7. Initialize FastLED/LedEffects
    #if ENABLE_LEDS
        ledEffects().begin();
    #endif

    // 8. Network setup
    setupWiFi();
    setupMQTT();
    setupWebServer();
}

void loop() {
    // Update LED effects every iteration
    #if ENABLE_LEDS
        ledEffects().update();
    #endif

    // Poll button states
    pollHardwareButtons();

    // Handle MQTT
    MQTTManager::instance().loop();

    // Other tasks...
}
```

**postSetup():**

```cpp
void postSetup() {
    // Trigger boot LED effect
    #if ENABLE_LEDS
        ledEffects().startEffectCycles("chase_single", 1);
    #endif

    // Print startup message
    printerManager.printStartupMessage();
}
```

---

## ESP32-C3 Specific Issues

### Critical Bug: pinMode() + FastLED Incompatibility

**Manifestation:**

```
[BOOT] ✅ ledEffects().begin() succeeded
[POST_SETUP] Triggering boot LED chase effect...
[LOOP] Calling ledEffects().update()...
[LEDS] UPDATE: Calling FastLED.show() for normal update
Guru Meditation Error: Core  0 panic'ed (Load access fault). Exception was unhandled.
```

**Investigation Timeline:**

1. **Commit 97e703d:** "fastled working on main with managers disabled"
   - Direct FastLED initialization works
   - No button initialization
   - No pinMode() calls
   - **Result:** LEDs work perfectly ✅

2. **Commit 3924193:** "minimal c3 test now working"
   - Created minimal test environment
   - Direct FastLED only
   - **Result:** Works ✅

3. **Commit 4c8179a:** "C3 still borken, S3 still working"
   - Full architecture with singletons and buttons
   - pinMode() calls added
   - **Result:** C3 crashes, S3 works ❌

4. **Commit 45601ec:** "managers are fine, config system and printer is fine, but starting an led effect after setting pinmode on buttons causes crash"
   - Isolated that managers, config, and printer are NOT the problem
   - pinMode() is the trigger
   - **Result:** Confirmed pinMode() as culprit ✅

**Tests Performed:**

| Test Description                   | GPIOs    | Timing            | Result              |
| ---------------------------------- | -------- | ----------------- | ------------------- |
| No pinMode() at all                | None     | N/A               | ✅ Works            |
| pinMode() on button pins (4,5,6,7) | 4,5,6,7  | After ledEffects  | ❌ Crash            |
| pinMode() before ledEffects        | 4,5,6,7  | Before ledEffects | ❌ Crash            |
| pinMode() on non-RTC GPIOs         | 6,7,10,2 | After ledEffects  | ❌ Crash            |
| pinMode() on different pins        | 5,6,7,8  | After ledEffects  | ❌ Crash            |
| pinMode() on just 1 pin            | 4 only   | After ledEffects  | ❌ Crash (untested) |

**Crash Location:**

- Always in `FastLED.show()` during RMT transmission
- Load access fault at memory address 0x00000108
- MEPC register points to RMT driver code
- Stack shows FastLED → RMT → crash

**Hardware Context:**

- ESP32-C3 has 4 RMT channels (vs 8 on S3)
- Single RISC-V core (vs dual Xtensa on S3)
- Different RMT hardware implementation than S3
- Known FastLED issues on C3 (GitHub #1498)

---

#
---

### Option 5: Systematic Rollback Approach (RECOMMENDED)

**Theory:** Start from commit 97e703d (last working C3) and incrementally port S3 changes while testing C3 after each step.

**Steps:**

1. Checkout commit 97e703d (C3 working with direct FastLED)
2. Test baseline - verify LEDs work
3. Port singleton pattern (LogManager, ConfigManager, etc.)
4. Test C3 - verify LEDs still work
5. Port LedEffects system
6. Test C3 - verify LEDs still work
7. Port button system **WITHOUT pinMode() calls**
8. Test C3 buttons with alternative GPIO configuration
9. If all works, implement Option 1 (ESP-IDF GPIO)
10. Final test and commit

**Pros:**

- Systematic, testable approach
- Identifies exact breaking point
- Preserves S3 functionality
- Minimal risk

**Cons:**

- Time-consuming
- Requires careful testing at each step

**Status:** ✅ Recommended approach

---

## File-by-File Change Summary

### Core System Files

**src/core/LogManager.cpp** (NEW)

- Thread-safe logging singleton
- Queue-based async logging
- Dedicated writer task
- 256 message queue, 512 byte max line

**src/core/LogManager.h** (NEW)

- LogManager class interface
- Queue and mutex definitions
- Public logging API

**src/core/logging.cpp** (MODIFIED)

- Converted to wrapper around LogManager
- Removed file logging
- Backward-compatible function wrappers

**src/core/logging.h** (MODIFIED)

- Updated log level enums
- Wrapper function declarations
- Maintains backward compatibility

**src/core/config_loader.cpp** (MAJOR REFACTOR)

- Thread-safe ConfigManager singleton
- Board detection and validation
- Mutex-protected NVS operations
- Runtime board mismatch handling

**src/core/config_loader.h** (MAJOR REFACTOR)

- ConfigManager class interface
- Board detection methods
- Public config API

**src/core/mqtt_handler.cpp** (MAJOR REFACTOR)

- Thread-safe MQTTManager singleton
- Mutex-protected MQTT operations
- Automatic reconnection logic
- Message queuing system

**src/core/mqtt_handler.h** (MAJOR REFACTOR)

- MQTTManager class interface
- Connection state management
- Public publish/subscribe API

### Board Configuration Files

**src/config/boards/board_interface.h** (NEW)

- Common GPIO structures
- ButtonConfig, BoardPinDefaults, etc.
- Shared interfaces for all boards

**src/config/boards/board_esp32c3_mini.h** (NEW)

- C3-mini specific GPIO map
- Button defaults (GPIOs 4,5,6,7)
- LED pin (GPIO 20)
- Printer pin (GPIO 21)
- Status LED (GPIO 8)

**src/config/boards/board_esp32s3_supermini.h** (NEW)

- S3-supermini GPIO map
- Button defaults (GPIOs 1,2,42,41)
- LED pin (GPIO 48)
- Printer pins (TX: GPIO 17, RX: GPIO 18, DTR: GPIO 21)
- Status LED (GPIO 47)

**src/config/boards/board_esp32s3_custom_pcb.h** (NEW)

- S3-custom-PCB GPIO map
- eFuse support (GPIO 35, 36)
- Power control via eFuse

**src/config/boards/board_config.h** (NEW)

- Compile-time board selection
- Platform detection macros
- Board includes

**src/config/gpio_map.h** (DELETED)

- Replaced by board-specific files

### LED System Files

**src/leds/LedEffects.cpp** (MAJOR REFACTOR)

- Thread-safe singleton pattern
- Static LED array allocation
- Cycle-based effects
- Effect registry system
- Mutex protection

**src/leds/LedEffects.h** (MAJOR REFACTOR)

- LedEffects class interface
- Singleton accessor function
- Effect management API

**src/leds/effects/ChaseSingle.cpp** (MODIFIED)

- Cycle-based effect implementation
- Removed duration/timer logic

### Hardware Files

**src/hardware/printer.cpp** (MAJOR REFACTOR)

- Thread-safe printing via mutex
- Queue-based print management
- eFuse power control support
- Error recovery logic

**src/hardware/printer.h** (MODIFIED)

- PrinterManager class updates
- Power control methods
- Thread-safety additions

**src/hardware/hardware_buttons.cpp** (MODIFIED)

- Board-specific GPIO validation
- pinMode() calls ← **BREAKS C3**
- Immediate action execution
- LED effect integration

### Utility Files

**src/utils/api_client.cpp** (MAJOR REFACTOR)

- Thread-safe APIClient singleton
- Mutex-protected HTTP operations
- Connection pooling
- Retry logic

**src/utils/api_client.h** (MODIFIED)

- APIClient class interface
- Request/response management
- Thread-safety additions

### Main Application

**src/main.cpp** (MAJOR REFACTOR)

- Singleton initialization order
- Board detection on boot
- LED effect triggering
- Thread coordination

---

## Known Working States

### ESP32-S3 (All variants) ✅

**Working Configuration:**

- All singleton managers
- LedEffects system
- Button system with pinMode()
- All features enabled

**Boards:**

- ESP32-S3-supermini (4MB)
- ESP32-S3-mini (4MB)
- ESP32-S3-custom-PCB (8MB with eFuse)

**Test Status:** ✅ Fully working

### ESP32-C3-mini ❌

**Broken Configuration:**

- All singleton managers ✅ Working
- LedEffects system ✅ Working
- Button system with pinMode() ❌ **CRASHES**

**Working Without:**

- Remove all pinMode() calls → LEDs work perfectly
- Use direct FastLED (commit 97e703d) → Works
- LedEffects without buttons → Works

**Test Status:** ⚠️ Partially working (LEDs work, buttons break LEDs)

---

## Migration Path for Future Work

### Step 1: Document Current State ✅ (This File)

Create comprehensive documentation of:

- What changed between 186f0ff and HEAD
- Why each change was made
- What works and what's broken
- Proposed solutions

### Step 2: Systematic Rollback Testing

1. Create new branch: `c3-systematic-port`
2. Checkout commit 97e703d (last working C3)
3. Test baseline functionality
4. Port changes incrementally:
   - Singleton pattern
   - LedEffects system
   - Button system (without pinMode)
5. Test C3 after each port
6. Document results

### Step 3: Alternative GPIO Configuration

Test alternatives to pinMode():

1. ESP-IDF gpio functions
2. Direct register manipulation
3. External resistors (hardware change)
4. Different LED library

### Step 4: Integration Testing

Once solution found:

1. Test all boards (C3, S3-mini, S3-supermini, S3-custom-PCB)
2. Verify all features work
3. Update documentation
4. Create production build

---

## References

### Git Commits

**Key Milestones:**

- `186f0ff` - Baseline (before S3 work)
- `9de92f6` - ESP32-S3 mini support added
- `2a45519` - Board-specific GPIO system
- `e2542f5` - Thread-safe singleton pattern
- `2b906fd` - LogManager singleton
- `f52d81a` - PrinterManager for S3
- `97e703d` - Last working C3 (direct FastLED)
- `4db7a97` - GPIO 9 → 4 fix for C3
- `4c8179a` - C3 broken, S3 working
- `45601ec` - Identified pinMode as culprit
- `34ed64e` - Current HEAD

### FastLED Issues

- GitHub #1498: ESP32-C3 RMT Driver Bug
- GitHub #1108: Major updates to ESP32 support
- GitHub #1657: Serious flickering problems on ESP32-C3

### ESP-IDF Documentation

- GPIO & RTC GPIO (ESP32-C3)
- Remote Control Transceiver (RMT)
- GPIO Matrix configuration

---

## Conclusion

The ESP32-S3 architecture changes successfully enabled multi-board support, thread-safety, and robust LED effects management. However, a fundamental incompatibility exists between Arduino's `pinMode()` function and FastLED's RMT driver on ESP32-C3.

**Critical Finding:** Calling `pinMode()` on ESP32-C3 globally corrupts the RMT peripheral, causing `FastLED.show()` to crash with a load access fault, regardless of:

- Which GPIOs are configured with pinMode()
- Initialization order (before/after FastLED)
- GPIO type (RTC vs non-RTC)

**Recommended Next Steps:**

1. Use systematic rollback approach (Option 5)
2. Test ESP-IDF gpio functions as alternative
3. Consider external pull-up resistors for hardware revision
4. Document findings for future ESP32-C3 projects

**For Future Claude Instances:**
Start from commit `97e703d` and systematically port S3 changes while testing C3 after each step. The goal is to identify the minimum viable alternative to `pinMode()` that works on ESP32-C3 without breaking the RMT peripheral used by FastLED.

---

**Document Version:** 1.0
**Last Updated:** October 21, 2025
**Author:** Claude (Anthropic) with Adam Knowles
**Purpose:** Transfer knowledge to future Claude instances working on ESP32-C3 compatibility
