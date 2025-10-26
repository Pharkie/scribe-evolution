# LED Effects - CLAUDE.md

<system_context>
LED effects system with conditional compilation.
FastLED-based cycle management and effect registry.
Thread-safe singleton pattern with RAII mutex locking.
Multi-board support: ESP32-C3, ESP32-S3 (4MB/8MB variants).
**Platform-agnostic code**: Same codebase works on both single-core C3 and dual-core S3.
**Status LED**: Custom PCB uses WS2812 RGB LED with color-coded WiFi status; mini boards use simple digital output.
</system_context>

<critical_notes>

- Conditional compilation with #ifdef ENABLE_LEDS
- All LED GPIO pins must pass safety validation
- Effects use cycle-based timing for consistency
- Effect registry pattern allows extensibility
- **Thread-safe singleton**: Mutex created in begin(), NOT constructor (critical for initialization order)
- **Mutex required for S3 (dual-core)**: No-op overhead on C3 (single-core) but prevents race conditions on S3
- **FastLED RMT configuration REQUIRED**: See platformio.ini for board-specific settings
- **Watchdog feeding**: LED operations can be slow, esp_task_wdt_reset() before FastLED.show()
- **NO platform-specific workarounds**: Clean code with proper watchdog management instead of hacks
  </critical_notes>

<paved_path>
Effect Development:

1. Create effect class inheriting from base effect
2. Implement update() method with cycle timing
3. Register effect in effect registry
4. Test with hardware validation

Available Effects:
ChaseSingle, ChaseMulti, Rainbow, Twinkle, Pulse, Matrix
</paved_path>

<patterns>
// Conditional compilation guard
#ifdef ENABLE_LEDS

// Effect class pattern
class RainbowEffect : public BaseEffect {
public:
void update(uint32_t cycle) override {
// Effect logic here
for (int i = 0; i < numLeds; i++) {
leds[i] = CHSV((cycle + i \* 10) % 255, 255, 255);
}
FastLED.show();
}
};

// Effect registration
void registerEffects() {
effectRegistry.add("rainbow", new RainbowEffect());
effectRegistry.add("pulse", new PulseEffect());
}

// GPIO validation for LEDs
if (!isValidGPIO(LED_PIN) || !isSafeGPIO(LED_PIN)) {
logger.log("Invalid LED GPIO: " + String(LED_PIN));
return false;
}

#endif // ENABLE_LEDS
</patterns>

<common_tasks>
Adding new effect:

1. Create effect class with update() method
2. Use cycle parameter for timing consistency
3. Register in effect registry
4. Test memory usage on ESP32-C3
5. Add to web interface selection
   </common_tasks>

<fatal_implications>

- Missing #ifdef guards = Compile errors when LEDs disabled
- Skip GPIO validation = Hardware damage
- Poor cycle timing = Jerky animations
- Memory leaks = System crashes
- Missing FastLED RMT flags = ESP32-C3 crashes on LED effect start
- Create mutex in constructor = Nullptr crashes on early access
  </fatal_implications>

<fastled_rmt_configuration>

## FastLED RMT Driver Configuration (CRITICAL for ESP32)

FastLED uses the ESP32's RMT (Remote Control) peripheral for WS2812B LED strips.
**REQUIRED** build flags in platformio.ini for ALL ESP32 variants:

```ini
-DFASTLED_RMT_MAX_CHANNELS=1      # Use 1 RMT channel (single LED strip)
-DFASTLED_RMT_BUILTIN_DRIVER=1    # Use stable ESP-IDF RMT driver
```

### Why These Flags Are Critical

**Without these flags:**

- ESP32-C3: Crashes with "Load access fault" when calling FastLED.show()
- ESP32-S3: May work but uses unstable custom RMT driver
- Random crashes during LED operations
- RMT channel conflicts with other peripherals

**RMT Channel Capabilities:**

- ESP32-C3: 4 RMT channels (RMT0-RMT3)
- ESP32-S3: 4 TX + 4 RX channels (8 total, but 4 usable for TX)

**MAX_CHANNELS=1 chosen because:**

- Scribe hardware uses single LED strip per board
- Prevents conflicts with other RMT users (IR remotes, etc.)
- Conservative, stable configuration
- Can be increased to 2-4 if multiple LED strips needed

**BUILTIN_DRIVER=1 chosen because:**

- Uses ESP-IDF's native RMT driver (better tested)
- More stable than FastLED's custom implementation
- Prevents memory access violations
- Required for ESP32-C3 stability

### Board Configuration Status

LED support by platform:

- ✅ **ESP32-C3**: ENABLED with RMT driver (ENABLE_LEDS=1) - Watchdog timeout fix applied
- ✅ **ESP32-S3** (all variants): ENABLED with RMT driver (ENABLE_LEDS=1)
  - esp32s3_base (lines 95-96): RMT configured
  - esp32s3_4mb_base (lines 118-119): RMT configured
  - esp32s3-8mb-custom-pcb: Inherits from esp32s3_base

### ESP32-C3 Historical Issue (RESOLVED)

**Previous Issue**: ESP32-C3 experienced watchdog timeouts due to accumulated debugging workarounds and missing watchdog resets.

**Root Cause**:

- Multiple FastLED.show() calls with delays
- Disabled mutex causing potential issues
- No watchdog reset before slow LED operations
- Accumulated debugging code from past investigation attempts

**Solution**:

- Removed all platform-specific workarounds and hacks
- Re-enabled proper mutex locking (required for S3, harmless on C3)
- Added esp_task_wdt_reset() before FastLED.show() in update()
- Clean, platform-agnostic codebase

**Key Principle**: One clean codebase for all platforms - proper watchdog feeding instead of conditional compilation hacks.
</fastled_rmt_configuration>

<thread_safe_architecture>

## LedEffects Thread-Safe Singleton

Follows the same pattern as all core singletons (LogManager, APIClient, etc.):

```cpp
// Singleton access
LedEffects& LedEffects::getInstance() {
    static LedEffects instance;  // Meyer's singleton
    return instance;
}

// Helper function for convenience
inline LedEffects& ledEffects() {
    return LedEffects::getInstance();
}

// Initialization (MUST be called in setup())
ledEffects().begin();

// Thread-safe operations
ledEffects().startEffectCycles("chase_single", 1, 0x0000FF);
ledEffects().update();  // Called in main loop
ledEffects().stopEffect();
```

**CRITICAL PATTERN**:

- Constructor: Initialize all members, set mutex=nullptr, initialized=false
- begin(): Create mutex HERE (ensures FreeRTOS is ready), call reinitializeInternal()
- All public methods: Check initialized flag, acquire mutex, call internal method
- Internal methods: Assume mutex already held by caller

**RAII Lock Guard (LedLock)**:

- Automatically acquires mutex on construction
- Automatically releases mutex on destruction
- Prevents mutex leaks from early returns or exceptions
- Used by all public methods

See src/core/CLAUDE.md for complete singleton pattern documentation.
</thread_safe_architecture>

<status_led_module>

## StatusLed Module (Custom PCB Only)

**Location**: `src/leds/StatusLed.h`, `src/leds/StatusLed.cpp`

The StatusLed module provides WS2812 RGB LED control for the custom PCB's status indicator. This is separate from the main LED strip and only compiles when `BOARD_ESP32S3_CUSTOM_PCB && ENABLE_LEDS`.

### Architecture

**Static Class Pattern**:

- All methods are static - no instance needed
- Separate `CRGB statusLed[1]` array from main LED strip
- Compile-time GPIO pin selection via switch statement (FastLED requirement)

**Usage in network.cpp**:

```cpp
#if defined(BOARD_ESP32S3_CUSTOM_PCB) && ENABLE_LEDS
    // Custom PCB: WS2812 RGB status LED
    #include "leds/StatusLed.h"

    void initializeStatusLED() {
        StatusLed::begin();
    }

    void updateStatusLED() {
        switch (currentWiFiMode) {
        case WIFI_MODE_CONNECTING:
            StatusLed::setBlink(CRGB::Orange, 250);  // Fast orange blink
            StatusLed::update();
            break;
        case WIFI_MODE_STA_CONNECTED:
            StatusLed::setSolid(CRGB::Green);        // Solid green
            break;
        case WIFI_MODE_AP_FALLBACK:
            StatusLed::setBlink(CRGB::Blue, 1000);   // Slow blue blink
            StatusLed::update();
            break;
        case WIFI_MODE_DISCONNECTED:
            StatusLed::off();                         // Off
            break;
        }
    }
#else
    // Mini boards: Simple digital output
    void initializeStatusLED() {
        pinMode(statusLEDPin, OUTPUT);
        digitalWrite(statusLEDPin, LOW);
    }

    void updateStatusLED() {
        // Simple on/off/blink logic with digitalWrite
    }
#endif
```

### API Reference

**Initialization**:

```cpp
StatusLed::begin();  // Initialize FastLED for status LED
```

**Solid Color**:

```cpp
StatusLed::setSolid(CRGB::Green);  // Set solid color, shows immediately
```

**Blinking**:

```cpp
StatusLed::setBlink(CRGB::Orange, 250);  // Set blink color and interval (ms)
StatusLed::update();                      // Must call in loop to handle timing
```

**Turn Off**:

```cpp
StatusLed::off();  // Turn LED off immediately
```

### Color Coding (WiFi Status)

- **Orange blink (250ms)**: Connecting to WiFi
- **Solid green**: Connected to WiFi
- **Blue blink (1000ms)**: AP mode (fallback)
- **Off**: Disconnected

### Key Design Points

1. **Separate from LED strip**: Uses independent `CRGB statusLed[1]` array
2. **100% brightness**: Maximum visibility for status indication
3. **Fast blink**: 250ms interval clearer than slow pulse
4. **Conditional compilation**: Zero overhead on mini boards
5. **Static methods**: No singleton needed, simpler than LedEffects

### FastLED Compile-Time GPIO Selection

FastLED requires compile-time GPIO pin selection via template parameters. StatusLed uses a switch statement to handle this:

```cpp
void StatusLed::begin() {
    switch (BOARD_STATUS_LED_PIN) {
        case 16: FastLED.addLeds<WS2812, 16, GRB>(statusLed, 1); break;
        // Add more cases if needed for other GPIO pins
        default: break;
    }
    FastLED.setBrightness(255);  // 100% brightness
}
```

This pattern allows runtime board configuration while satisfying FastLED's compile-time requirements.

</status_led_module>
