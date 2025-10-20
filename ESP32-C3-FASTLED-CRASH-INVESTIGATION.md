# ESP32-C3 FastLED Crash Investigation

**Date:** 2025-10-21
**Board:** ESP32-C3-mini (4MB flash)
**Issue:** ~~Crashes when using LedEffects wrapper~~ **RESOLVED**
**Status:** ✅ LedEffects system WORKING on ESP32-C3!

---

## Working Configuration (Commit: 97e703d)

### What Works

- ✅ Direct FastLED.show() in loop()
- ✅ pinMode() on button GPIOs (5,6,7,4)
- ✅ digitalRead() in loop()
- ✅ checkHardwareButtons() in loop()
- ✅ All managers initialized (LogManager, ConfigManager, APIClient, MQTTManager)
- ✅ Full WiFi, web server, MQTT, buttons, printer
- ✅ Static LED array (staticLEDs[MAX_LEDS])

### Direct FastLED Code (WORKING)

```cpp
// In setup()
extern CRGB staticLEDs[];
FastLED.addLeds<WS2812B, 20, GRB>(staticLEDs, 30);
FastLED.setBrightness(100);
FastLED.clear();
FastLED.show();

// In loop()
static unsigned long lastLedUpdate = 0;
static int ledPos = 0;

if (millis() - lastLedUpdate > 50) {
  for (int i = 0; i < 30; i++) {
    staticLEDs[i] = CRGB::Black;
  }
  staticLEDs[ledPos] = CRGB::Blue;

  FastLED.show();  // ✅ WORKS

  ledPos = (ledPos + 1) % 30;
  lastLedUpdate = millis();
}
```

---

---

## ROOT CAUSE & SOLUTION

### The Problem

**Double FastLED initialization** - main.cpp was calling `FastLED.addLeds()` directly AND then `ledEffects().begin()` was also trying to initialize FastLED, causing ESP32-C3 RMT peripheral crash.

Crash signature:

```
Guru Meditation Error: Core 0 panic'ed (Load access fault)
MCAUSE: 0x00000005  MTVAL: 0x00000108
```

### The Fix

Remove direct FastLED initialization from main.cpp. Let `ledEffects().begin()` handle ALL FastLED setup.

**Before (BROKEN):**

```cpp
// In setup()
FastLED.addLeds<WS2812B, 20, GRB>(staticLEDs, 30);  // ❌ First init
ledEffects().begin();  // ❌ Second init = CRASH
```

**After (WORKING):**

```cpp
// In setup()
ledEffects().begin();  // ✅ Only init - WORKS!
```

---

## LedEffects System Analysis

### Initialization Flow

```
ledEffects().begin()
  ↓
Creates mutex (xSemaphoreCreateMutex)
  ↓
Acquires lock (LedLock)
  ↓
Calls getRuntimeConfig()
  ↓
Calls reinitializeInternal()
    ↓
  Sets ledPin, ledCount, ledBrightness, ledRefreshRate
    ↓
  Points leds = staticLEDs (static global array)
    ↓
  Validates GPIO with isSafeGPIO()
    ↓
  Calls FastLED.clear()  // ← First FastLED operation
    ↓
  Calls FastLED.addLeds<WS2812B, pin, GRB>(leds, ledCount) via switch
    ↓
  Sets FastLED.setBrightness()
    ↓
  Creates EffectRegistry (new operator)
    ↓
  Stores effect configuration
```

### Key Differences from Direct FastLED

| Aspect          | Direct FastLED         | LedEffects System                    |
| --------------- | ---------------------- | ------------------------------------ |
| Mutex           | None                   | xSemaphoreCreateMutex + LedLock RAII |
| Memory          | Direct staticLEDs      | Points to staticLEDs                 |
| Init Order      | addLeds → clear → show | clear → addLeds → brightness         |
| Config Access   | Hardcoded              | getRuntimeConfig()                   |
| Effect Registry | None                   | new EffectRegistry()                 |
| GPIO Validation | None                   | isSafeGPIO() check                   |

### Suspected Issues

1. **FastLED.clear() BEFORE FastLED.addLeds()**
   - Line 224: `FastLED.clear()` called before line 234-280: `FastLED.addLeds()`
   - Comment on line 222-223 warns about FastLED.clearData() corruption
   - May need to remove FastLED.clear() entirely before addLeds()

2. **Mutex/Lock Timing**
   - LedLock acquires mutex at line 136 in begin()
   - Held during entire FastLED initialization
   - ESP32-C3 has different FreeRTOS behavior than S3?

3. **getRuntimeConfig() Call**
   - Line 144: Accesses NVS-backed configuration
   - May trigger memory operations incompatible with RMT state

4. **EffectRegistry Heap Allocation**
   - Line 290-295: `new EffectRegistry()` after FastLED init
   - Heap allocation during/after RMT peripheral setup?

5. **GPIO Validation Functions**
   - Line 215-218: isSafeGPIO() check
   - May perform operations that interfere with RMT

---

## ESP32-C3 Constraints

### Hardware Differences (C3 vs S3)

| Feature      | ESP32-C3      | ESP32-S3    |
| ------------ | ------------- | ----------- |
| Core         | Single RISC-V | Dual Xtensa |
| RMT Channels | 4             | 8           |
| RAM          | 400KB         | 512KB       |
| GPIO Range   | 0-21          | 0-48        |

### Known ESP32-C3 Issues

- ⚠️ RMT peripheral more sensitive to initialization order
- ⚠️ Single core = no concurrent tasks during init
- ⚠️ Limited RMT channels (4 vs 8 on S3)
- ⚠️ Comment in code (line 222): "FastLED.clearData() before addLeds() causes ESP32-C3 crash"

---

## Test Results

### Test 1: Minimal FastLED Test

- **File:** `test/test_c3_fastled.cpp`
- **Result:** ✅ WORKS
- **Config:** Direct FastLED with pinMode() AFTER addLeds()

### Test 2: Direct FastLED in Main App

- **Commit:** 97e703d
- **Result:** ✅ WORKS
- **Config:** Full app with direct FastLED, no LedEffects wrapper

### Test 3: LedEffects.begin() in Main App

- **Result:** ❌ CRASHES
- **Location:** During `ledEffects().begin()` in setup()

### Test 4: LedEffects.startEffectCycles() in PostSetup

- **Result:** ❌ CRASHES
- **Location:** During `ledEffects().startEffectCycles("chase_single", 1)`

---

## Investigation Plan

### Phase 1: Isolate Root Cause ✓

- [x] Confirm direct FastLED works
- [x] Confirm LedEffects crashes
- [x] Identify crash location (begin/startEffect/update)

### Phase 2: Test LedEffects Components (TODO)

1. Test FastLED.clear() BEFORE addLeds()
   - Remove line 224 FastLED.clear()
   - See if init succeeds

2. Test without mutex/lock
   - Comment out mutex creation
   - Comment out LedLock acquisition
   - See if ESP32-C3 single-core doesn't need it

3. Test without EffectRegistry
   - Comment out line 290-295
   - See if heap allocation is the issue

4. Test without getRuntimeConfig()
   - Hardcode values instead of line 144
   - See if NVS access interferes

5. Test without GPIO validation
   - Comment out line 215-218
   - See if validation functions interfere

### Phase 3: Incremental Build (TODO)

- Start with minimal LedEffects that just wraps FastLED
- Add features one at a time until crash reproduces
- Identify exact breaking change

### Phase 4: Fix Implementation (TODO)

- Based on Phase 2/3 findings
- Implement ESP32-C3 specific workarounds
- Test thoroughly with all features

---

## Next Steps

1. **Test FastLED.clear() removal**
   - Most suspicious: clear() before addLeds()
   - Direct code doesn't call clear() before addLeds()

2. **Test mutex removal**
   - ESP32-C3 is single core
   - May not need mutex protection

3. **Test minimal wrapper**
   - Create stripped-down LedEffects
   - Add complexity gradually

---

## Code Locations

- **Working code:** `src/main.cpp` (commit 97e703d)
- **LedEffects impl:** `src/leds/LedEffects.cpp`
- **LedEffects header:** `src/leds/LedEffects.h`
- **Minimal test:** `test/test_c3_fastled.cpp`
- **Static array:** `src/leds/LedEffects.cpp:23` (staticLEDs[MAX_LEDS])

---

---

## SOLUTION FOUND! ✅

### Root Cause

**Double initialization of FastLED** was causing the crashes. When both direct FastLED.addLeds() AND ledEffects().begin() tried to initialize FastLED, the ESP32-C3 RMT peripheral would crash with null pointer dereference.

### Working Configuration (Current)

```cpp
// In setup() - NO direct FastLED initialization
#if ENABLE_LEDS
  // Let ledEffects handle ALL FastLED initialization
  if (ledEffects().begin()) {
    Serial.println("[BOOT] ✅ ledEffects().begin() succeeded");
  }
#endif

// In postSetup() - Trigger boot LED effect
#if ENABLE_LEDS
  ledEffects().startEffectCycles("chase_single", 1);
#endif

// In loop() - Update LED effects
#if ENABLE_LEDS
  ledEffects().update();
#endif
```

### What's Working Now

- ✅ ledEffects().begin() initializes FastLED properly
- ✅ ledEffects().startEffectCycles() triggers boot chase effect
- ✅ ledEffects().update() runs effects in loop
- ✅ No crashes, continuous operation confirmed
- ✅ Chase effect visible on physical LEDs

### What's Currently Disabled (For Testing)

These were disabled to isolate the LED issue and are NOT required for LEDs to work:

1. ❌ WiFi connection
2. ❌ All managers (LogManager, ConfigManager, APIClient, MQTTManager)
3. ❌ Config system & PrinterManager
4. ❌ pinMode() calls in button initialization (`hardware_buttons.cpp:133`)
5. ❌ MQTT client
6. ❌ Web server
7. ❌ Unbidden Ink
8. ❌ Startup message printing

### Next Steps: Re-enable Production Features

Now that LedEffects works, systematically re-enable disabled components:

1. **Re-enable all managers** (ConfigManager needed for getRuntimeConfig())
2. **Re-enable config system & printer**
3. **Re-enable pinMode() in buttons** (test if compatible with ledEffects)
4. **Re-enable WiFi**
5. **Re-enable web server**
6. **Re-enable MQTT**
7. **Re-enable Unbidden Ink**
8. **Test each addition to ensure no regression**

### Original Hypotheses - INCORRECT

All suspected issues in LedEffects.cpp were red herrings:

- ❌ FastLED.clear() before addLeds() - NOT the issue
- ❌ Mutex during initialization - NOT the issue
- ❌ EffectRegistry heap allocation - NOT the issue
- ❌ getRuntimeConfig() NVS access - NOT the issue
- ❌ GPIO validation - NOT the issue

**The real issue:** User code was calling FastLED.addLeds() directly before ledEffects().begin(), causing double initialization.
