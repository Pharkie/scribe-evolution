# ESP32-C3 LedEffects Status

**Date:** 2025-10-21
**Board:** ESP32-C3-mini (4MB flash)
**Status:** ✅ LedEffects working with minimal configuration

---

## Current Working State

### What's Working ✅

- ✅ ledEffects().begin() - Initializes FastLED successfully
- ✅ ledEffects().startEffectCycles("chase_single", 1) - Boot chase effect triggers
- ✅ ledEffects().update() - Effect updates running in loop
- ✅ Hardware buttons - Checking button states (without pinMode)
- ✅ checkHardwareButtons() - Button debouncing and event handling
- ✅ Physical LEDs confirmed working (chase effect visible)
- ✅ No crashes, continuous stable operation

### What's Currently Disabled ❌

These components were disabled during debugging to isolate LED issues:

1. **WiFi** - `src/main.cpp:129-133`
   - WiFi connection disabled
   - Device not connecting to network

2. **All Managers** - `src/main.cpp:135-143`
   - LogManager disabled
   - ConfigManager disabled
   - APIClient disabled
   - MQTTManager disabled

3. **Config System & Printer** - `src/main.cpp:188-198`
   - initializeConfigSystem() disabled
   - printerManager.initialize() disabled

4. **pinMode() in Buttons** - `src/hardware/hardware_buttons.cpp:133`
   - pinMode() calls commented out
   - Buttons working without explicit pin configuration

5. **MQTT Client** - `src/main.cpp:233-250`
   - MQTT connection disabled
   - No message publishing/receiving

6. **Web Server** - `src/main.cpp:252-256`
   - AsyncWebServer disabled
   - No web interface access

7. **Unbidden Ink** - `src/main.cpp:258-260`
   - Unbidden Ink system disabled
   - No autonomous content generation

8. **Startup Message** - `src/main.cpp:300-302` (postSetup)
   - Printer startup message disabled

---

## Code Configuration

### Current main.cpp Setup

```cpp
// In setup()
#if ENABLE_LEDS
  // ledEffects handles ALL FastLED initialization
  Serial.println("[BOOT] Testing ledEffects().begin() initialization (no direct FastLED)...");
  if (ledEffects().begin()) {
    Serial.println("[BOOT] ✅ ledEffects().begin() succeeded");
  } else {
    Serial.println("[BOOT] ❌ ledEffects().begin() FAILED");
  }
#endif

// Hardware buttons enabled (without pinMode)
if (!isAPMode()) {
  initializeHardwareButtons();
}
```

### Current postSetup()

```cpp
void postSetup()
{
#if ENABLE_LEDS
  // Trigger boot LED chase effect (1 cycle)
  Serial.println("[POST_SETUP] Triggering boot LED chase effect...");
  ledEffects().startEffectCycles("chase_single", 1);
  Serial.println("[POST_SETUP] ✅ Boot LED effect started");
#endif
}
```

### Current loop()

```cpp
void loop()
{
  // Feed watchdog
  esp_task_wdt_reset();

  // Process ezTime events
  events();

  // WiFi/DNS/MQTT all disabled

  // Check hardware buttons (only if not in AP mode)
  if (!isAPMode()) {
    checkHardwareButtons();
  }

#if ENABLE_LEDS
  // Update LED effects
  Serial.println("[LOOP] Calling ledEffects().update()...");
  ledEffects().update();
  Serial.println("[LOOP] ✓ ledEffects().update() succeeded!");
#endif
}
```

---

## Next Steps: Systematic Re-enablement

Test re-enabling components one at a time to identify any incompatibilities:

### Phase 1: Core Systems

1. **Re-enable all managers** (LogManager, ConfigManager, APIClient, MQTTManager)
   - ConfigManager needed for ledEffects to read config
   - Test: Does ledEffects still work?

2. **Re-enable config system & printer**
   - initializeConfigSystem()
   - printerManager.initialize()
   - Test: Does ledEffects still work?

### Phase 2: GPIO Configuration

3. **Re-enable pinMode() in buttons** (`hardware_buttons.cpp:133`)
   - Uncomment pinMode() calls
   - Test: Does this interfere with FastLED RMT?

### Phase 3: Network Services

4. **Re-enable WiFi**
   - connectToWiFi()
   - Test: Does WiFi affect FastLED?

5. **Re-enable web server**
   - setupWebServerRoutes()
   - server.begin()
   - Test: Does AsyncWebServer affect FastLED?

6. **Re-enable MQTT**
   - startMQTTClient()
   - Test: Does MQTT affect FastLED?

### Phase 4: Additional Features

7. **Re-enable Unbidden Ink**
   - initializeUnbiddenInk()
   - Test: Does scheduled content generation affect FastLED?

8. **Re-enable startup message**
   - printerManager.printStartupMessage()
   - Test: Does printer output affect FastLED?

---

## Test Protocol

For each re-enablement:

1. Uncomment the disabled code
2. Build and upload firmware
3. Monitor serial output for crashes
4. Check for Guru Meditation errors
5. Verify LEDs still working (boot chase + continuous effects)
6. If crash occurs, document which component caused it
7. If successful, move to next component

---

## Known Working Commits

- **97e703d** - Direct FastLED working (no ledEffects)
  - Direct FastLED.addLeds() in setup
  - Direct chase effect in loop
  - No ledEffects system used

- **Current** - ledEffects working (minimal config)
  - ledEffects().begin() in setup
  - ledEffects().startEffectCycles() in postSetup
  - ledEffects().update() in loop
  - Most components disabled

---

## ESP32-C3 Hardware Notes

- **Single RISC-V core** (vs dual-core S3)
- **4 RMT channels** (vs 8 on S3)
- **GPIO 0-21** (vs 0-48 on S3)
- **400KB RAM** (vs 512KB on S3)
- FastLED uses RMT peripheral for WS2812B control
- RMT channel allocation may be more sensitive on C3

---

## Questions to Answer

1. **Does pinMode() interfere with RMT?**
   - Currently disabled, buttons work without it
   - Need to test re-enabling

2. **Do managers affect FastLED?**
   - ConfigManager needed for getRuntimeConfig()
   - LogManager for logging
   - Test if they interfere

3. **Does WiFi/AsyncWebServer affect RMT?**
   - Both disabled currently
   - Async operations might interfere

4. **Is this a timing/race condition?**
   - Single core = no true parallelism
   - But still has interrupts/tasks

---

## Files Modified

- `src/main.cpp` - LED initialization, disabled components
- `src/hardware/hardware_buttons.cpp:133` - pinMode() commented out
- `ESP32-C3-FASTLED-CRASH-INVESTIGATION.md` - This file
