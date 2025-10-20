# LED Effects - CLAUDE.md

<system_context>
LED effects system with conditional compilation.
FastLED-based cycle management and effect registry.
Thread-safe singleton pattern with RAII mutex locking.
Multi-board support: ESP32-C3, ESP32-S3 (4MB/8MB variants).
</system_context>

<critical_notes>

- Conditional compilation with #ifdef ENABLE_LEDS
- All LED GPIO pins must pass safety validation
- Effects use cycle-based timing for consistency
- Effect registry pattern allows extensibility
- **Thread-safe singleton**: Mutex created in begin(), NOT constructor (prevents ESP32-C3 crashes)
- **FastLED RMT configuration REQUIRED**: See platformio.ini for board-specific settings
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

All base environments in platformio.ini have correct RMT flags:

- ✅ esp32c3_base (lines 72-73)
- ✅ esp32s3_base (lines 95-96)
- ✅ esp32s3_4mb_base (lines 118-119)
- ✅ esp32s3-8mb-custom-pcb inherits from esp32s3_base

**DO NOT remove or modify these flags without testing on hardware!**
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
