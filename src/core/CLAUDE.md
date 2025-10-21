# Core System - CLAUDE.md

<system_context>
ESP32-C3/S3 system fundamentals: configuration, network, MQTT, logging.
Memory-constrained environment with dual-layer config architecture.
Thread-safe singletons: LogManager (logging), APIClient (HTTP), ConfigManager (NVS/LittleFS), MQTTManager (MQTT).
</system_context>

<critical_notes>

- All defaults MUST be defined once in config.h
- Never hardcode values - always reference config constants
- Use appropriate JSON buffer sizes for ESP32-C3 memory limits
- Validate all GPIO pins with safety functions
- NEVER call Serial.print() directly - always use LOG\_\* macros
  </critical_notes>

<paved_path>
Dual Configuration System:

1. Compile-time defaults in config.h (hardware, constants, defaults)
2. Runtime settings in NVS (user preferences, persistent state)

Include pattern: #include "core/config.h"
NVS namespace: "scribe-app"
</paved_path>

<patterns>
// GPIO validation - ALWAYS use these functions
if (!isValidGPIO(pin) || !isSafeGPIO(pin)) {
    return false;
}

// JSON document sizing (ArduinoJson 7+)
JsonDocument doc; // Dynamic size
// or
StaticJsonDocument<512> doc; // Stack-allocated for ESP32 constraints

// NVS key access
preferences.begin("scribe-app", false);
String value = preferences.getString(NVS_KEY_DEVICE_OWNER, DEFAULT_DEVICE_OWNER);
preferences.end();

// Configuration loading
void loadRuntimeConfig() {
// Load from NVS with config.h defaults as fallbacks
runtimeConfig.deviceOwner = getNVSString(NVS_KEY_DEVICE_OWNER, DEFAULT_DEVICE_OWNER);
}

// Thread-safe logging - ALWAYS use LOG\_\* macros
LOG_NOTICE("COMPONENT", "Message format: %s", variable);
LOG_ERROR("COMPONENT", "Error: %d", errorCode);
LOG_VERBOSE("COMPONENT", "Debug info: %s", details);
// NEVER: Serial.print() - causes concurrent write corruption!
</patterns>

<common_tasks>
Adding new configuration:

1. Add default to config.h: #define DEFAULT_NEW_SETTING "value"
2. Add NVS key to nvs_keys.h: #define NVS_KEY_NEW_SETTING "new_setting"
3. Add to RuntimeConfig struct
4. Add loading/saving in config management
5. Add validation if needed
   </common_tasks>

<fatal_implications>

- Hardcode values = Impossible to maintain
- Skip GPIO validation = Hardware damage (magic smoke)
- Wrong JSON buffer size = Stack overflow/heap exhaustion
- Multiple config sources = Inconsistent state
- Direct Serial.print() = Concurrent write corruption, garbled output
  </fatal_implications>

<thread_safe_architecture>
Thread-Safe Singleton Managers (prevent concurrent access corruption):

## Singleton Implementation Pattern

**CRITICAL**: All singletons MUST follow this pattern to avoid ESP32-C3/S3 crashes:

```cpp
class MyManager {
public:
    static MyManager& instance() {
        static MyManager instance;  // Meyer's singleton (thread-safe C++11+)
        return instance;
    }

    void begin() {
        if (initialized) return;

        // Create mutex HERE, not in constructor!
        mutex = xSemaphoreCreateMutex();
        if (mutex == nullptr) {
            LOG_ERROR("TAG", "Failed to create mutex!");
            return;
        }

        initialized = true;
        LOG_NOTICE("TAG", "MyManager initialized (thread-safe singleton)");
    }

    void doSomething() {
        if (!initialized) {
            LOG_ERROR("TAG", "Not initialized - call begin() first!");
            return;
        }

        if (xSemaphoreTake(mutex, portMAX_DELAY) != pdTRUE) {
            LOG_ERROR("TAG", "Failed to acquire mutex!");
            return;
        }

        // ... protected code ...

        xSemaphoreGive(mutex);
    }

private:
    MyManager() : mutex(nullptr), initialized(false) {}  // Minimal constructor
    ~MyManager() { if (mutex) vSemaphoreDelete(mutex); }
    MyManager(const MyManager&) = delete;
    MyManager& operator=(const MyManager&) = delete;

    SemaphoreHandle_t mutex;
    bool initialized;
};
```

**Why this pattern?**

- Static initialization order is unpredictable
- FreeRTOS scheduler may not be running during static init
- ESP32-C3 is sensitive to early mutex creation (crashes with nullptr mutex)
- Mutex created in begin() ensures FreeRTOS is fully initialized

## Active Singleton Managers

1. **LogManager** - Serial logging
   - FreeRTOS queue + dedicated writer task on Core 0
   - Non-blocking LOG\_\* macros (fire-and-forget)
   - ISR-safe logfISR() for interrupt contexts
   - NEVER use Serial.print() directly!
   - Pattern: ✅ Mutex in begin()

2. **APIClient** - HTTP operations
   - Thread-safe HTTP GET/POST with Bearer auth
   - Single WiFiClientSecure + HTTPClient instance
   - Usage: APIClient::instance().fetchFromAPI(url, userAgent, timeout)
   - Pattern: ✅ Mutex in begin()

3. **ConfigManager** - NVS/LittleFS operations
   - Thread-safe config save/load/reset
   - Direct read access (no mutex) for getRuntimeConfig()
   - Usage: ConfigManager::instance().saveNVSConfig(config)
   - Pattern: ✅ Mutex in begin()

4. **MQTTManager** - MQTT operations
   - Thread-safe publish/subscribe/connect/disconnect
   - State machine encapsulated in singleton
   - Usage: MQTTManager::instance().publishMessage(topic, header, body)
   - Pattern: ✅ Mutex in begin()

5. **LedEffects** - LED effects system (when ENABLE_LEDS=1)
   - Thread-safe effect control and FastLED access
   - RAII lock guards for automatic mutex release
   - Usage: ledEffects().startEffectCycles(name, cycles, color)
   - Pattern: ✅ Mutex in begin()

6. **PrinterManager** - Thermal printer control
   - Thread-safe UART access for thermal printer
   - Global singleton initialized with initialize()
   - Usage: printerManager.printWithHeader(timestamp, message)
   - Pattern: ✅ Mutex in initialize()

## Initialization Order (main.cpp setup())

```cpp
// Initialize all singletons AFTER LittleFS and WiFi
LogManager::instance().begin(115200, 256, 512);
APIClient::instance().begin();
ConfigManager::instance().begin();
MQTTManager::instance().begin();
#if ENABLE_LEDS
ledEffects().begin();  // Uses getInstance() internally
#endif
printerManager.initialize();
```

**Backward-compatible wrapper functions exist for all managers.**
See docs/logging-system.md for complete LogManager documentation.
</thread_safe_architecture>
