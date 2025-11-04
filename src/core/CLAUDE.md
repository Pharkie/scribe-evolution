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

<configuration_architecture>

## Configuration System: NVS-Backed vs Runtime-Only Settings

The Scribe Evolution configuration system uses a **dual-layer architecture** that combines user-configurable settings with compile-time constants in a single `RuntimeConfig` struct.

### Two Types of Configuration

#### 1. NVS-Backed Settings (User-Configurable)

**Characteristics:**

- Saved to ESP32 Non-Volatile Storage (NVS) across reboots
- Exposed in web configuration interface (see `config_field_registry.cpp`)
- Initial values from `device_config.h`, runtime values from NVS
- **Total: ~51 NVS keys** (see `nvs_keys.h` for complete list)

**Examples:**

- `deviceOwner` - Web: `device.owner`
- `wifiSSID` / `wifiPassword` - Web: `wifi.ssid`, `wifi.password`
- `mqttServer` / `mqttPort` - Web: `mqtt.server`, `mqtt.port`
- `printerTxPin` / `printerRxPin` / `printerDtrPin` - Web: `device.printer*Pin`
- `buttonGpios[4]` - Web: `buttons.button{1-4}.gpio`
- `chatgptApiToken` - Web: `unbiddenInk.chatgptApiToken`
- Button actions, MQTT topics, LED effects (24 keys)
- LED configuration (4 keys when `ENABLE_LEDS=1`)
- Memos (4 keys, not in web registry - use `/api/print-memo`)

**Naming Convention:**

- Constants in `device_config.h` use `default` prefix: `defaultWifiSSID`, `defaultMqttEnabled`
- NVS keys in `nvs_keys.h` use descriptive names: `NVS_WIFI_SSID`, `NVS_MQTT_ENABLED`

#### 2. Runtime-Only Constants (Compile-Time Configuration)

**Characteristics:**

- Always loaded from `system_constants.h` at runtime
- **NEVER saved to or loaded from NVS**
- **NOT exposed in web configuration interface**
- Fixed for security, performance, and simplicity

**Examples:**

- API endpoints: `jokeAPI`, `quoteAPI`, `triviaAPI`, `chatgptApiEndpoint`
- System timeouts: `wifiConnectTimeoutMs`, `serialTimeoutMs`
- Buffer sizes: `mqttBufferSize`, `jsonDocumentSize`
- Validation limits: `maxCharacters`, `minJokeLength`
- Rate limiting: `buttonMinInterval`, `maxRequestsPerMinute`
- Logging config: `logLevel`, `espLogLevel`, `enable*Logging`

**Naming Convention:**

- NO `default` prefix (e.g., `jokeAPI`, `maxCharacters`, `wifiConnectTimeoutMs`)
- Defined directly in `system_constants.h` as constants

### Why This Architecture?

**Benefits:**

1. **Single Source of Truth** - All config accessed via `getRuntimeConfig()`
2. **Type Safety** - No need to check if a field exists at runtime
3. **Performance** - No NVS lookups for constants, direct memory access
4. **Security** - API endpoints cannot be changed by users (prevents abuse)
5. **Simplicity** - Same access pattern for all settings in application code

**Design Philosophy:**

- User-facing settings (credentials, GPIO pins, behavior) → NVS
- System internals (endpoints, timeouts, limits) → Compile-time constants
- RuntimeConfig struct intentionally contains BOTH types

### NVS Key Inventory (51 total)

| Category      | Count | NVS Keys                                                                               |
| ------------- | ----- | -------------------------------------------------------------------------------------- |
| Device        | 2     | `device_owner`, `device_timezone`                                                      |
| Hardware GPIO | 7     | `printer_tx_pin`, `printer_rx_pin`, `printer_dtr_pin`, `btn{1-4}_gpio`                 |
| WiFi          | 2     | `wifi_ssid`, `wifi_password`                                                           |
| MQTT          | 5     | `mqtt_enabled`, `mqtt_server`, `mqtt_port`, `mqtt_username`, `mqtt_password`           |
| API           | 1     | `chatgpt_token`                                                                        |
| Unbidden Ink  | 5     | `unbid_enabled`, `unbid_start_hr`, `unbid_end_hr`, `unbid_freq_min`, `unbidden_prompt` |
| Buttons       | 24    | `btn{1-4}_{short/long}_{act/mq/led}` (4 buttons × 6 fields)                            |
| LEDs          | 4     | `led_pin`, `led_count`, `led_brightness`, `led_refresh` (when `ENABLE_LEDS=1`)         |
| Memos         | 4     | `memo_{1-4}`                                                                           |
| Internal      | 1     | `board_type` (hardware mismatch detection - NOT user-configurable)                     |

**Special Case: `board_type`**

- Saved to NVS but NOT in web registry
- Used for automatic GPIO reset when hardware changes (e.g., flashing different board)
- This is intentional - it's internal state, not a user setting

### Adding New Configuration Settings

#### Decision Tree: NVS-Backed or Runtime-Only?

Ask these questions:

1. **Should users be able to change this via web interface?**
   - YES → NVS-backed setting
   - NO → Continue to question 2

2. **Does this value need to persist across reboots?**
   - YES → NVS-backed setting
   - NO → Continue to question 3

3. **Could different users/deployments need different values?**
   - YES → NVS-backed setting
   - NO → Runtime-only constant

4. **Is this a security-sensitive value that shouldn't be user-modifiable?**
   - YES → Runtime-only constant (even if it seems user-facing)
   - NO → Consider NVS-backed

**Examples:**

- WiFi credentials → NVS (user-specific, must persist)
- Joke API endpoint → Runtime-only (fixed service URL, security)
- WiFi timeout → Runtime-only (technical parameter, well-tested default)
- Button GPIO pins → NVS (hardware-specific, user may need to change)
- Max characters → Runtime-only (DoS protection, should not be user-modifiable)

#### How to Add NVS-Backed Setting

1. **Add default value to `device_config.h.example`:**

   ```cpp
   static const int defaultNewSetting = 42;
   ```

2. **Add NVS key to `nvs_keys.h`:**

   ```cpp
   constexpr const char *NVS_NEW_SETTING = "new_setting";
   ```

3. **Add field to `RuntimeConfig` struct in `config_loader.h`:**

   ```cpp
   // In NVS-BACKED SETTINGS section
   int newSetting;  // Web: section.newSetting
   ```

4. **Add to web registry in `config_field_registry.cpp`:**

   ```cpp
   {"section.newSetting", ValidationType::RANGE_INT,
    offsetof(RuntimeConfig, newSetting), 1, 100, nullptr, 0},
   ```

5. **Add load logic in `config_loader.cpp::loadNVSConfigInternal()`:**

   ```cpp
   g_runtimeConfig.newSetting = getNVSInt(prefs, NVS_NEW_SETTING,
                                          defaultNewSetting, 1, 100);
   ```

6. **Add save logic in `config_loader.cpp::saveNVSConfigInternal()`:**

   ```cpp
   prefs.putInt(NVS_NEW_SETTING, config.newSetting);
   ```

7. **Add to default loader in `config_loader.cpp::loadDefaultConfigInternal()`:**
   ```cpp
   g_runtimeConfig.newSetting = defaultNewSetting;
   ```

#### How to Add Runtime-Only Constant

1. **Add constant to `system_constants.h`:**

   ```cpp
   static const int newConstant = 42;
   ```

2. **Add field to `RuntimeConfig` struct in `config_loader.h`:**

   ```cpp
   // In RUNTIME-ONLY CONSTANTS section
   int newConstant;  // Compile-time constant from system_constants.h
   ```

3. **Add to load functions in `config_loader.cpp`:**

   ```cpp
   // In loadNVSConfigInternal()
   g_runtimeConfig.newConstant = newConstant;

   // In loadDefaultConfigInternal()
   g_runtimeConfig.newConstant = newConstant;
   ```

4. **DO NOT add to:**
   - `nvs_keys.h` (not saved to NVS)
   - `config_field_registry.cpp` (not in web interface)
   - `saveNVSConfigInternal()` (not persisted)

### Common Mistakes to Avoid

❌ **Adding runtime-only values to NVS**

- Wastes NVS storage
- Creates confusion about configurability
- Example: `wifi_timeout` was incorrectly in NVS (fixed in 2025)

❌ **Using `default` prefix for non-NVS constants**

- Violates naming convention
- Makes grep-based searches unreliable

❌ **Exposing system internals in web interface**

- Security risk (e.g., allowing users to change API endpoints)
- Stability risk (e.g., allowing buffer size changes)

❌ **Hardcoding values instead of using RuntimeConfig**

- Defeats purpose of configuration system
- Makes testing and maintenance harder

✅ **Correct Pattern:**

```cpp
// Access any config value the same way
const RuntimeConfig& config = getRuntimeConfig();

// NVS-backed (can be changed by user via web)
printer.begin(config.printerTxPin, config.printerRxPin);

// Runtime-only (fixed at compile time)
String joke = fetchFromAPI(config.jokeAPI, apiUserAgent, 5000);
```

### Verification Commands

**Check NVS keys at runtime:**

```
GET http://scribe-device.local/api/nvs-dump
```

**Grep for NVS-backed settings:**

```bash
grep "^constexpr const char \*NVS_" src/core/nvs_keys.h
```

**Grep for default values:**

```bash
grep "^static const.*default" src/config/device_config.h.example
```

**Count web-configurable fields:**

```bash
grep '{"' src/web/config_field_registry.cpp | wc -l
```

</configuration_architecture>

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
   - Uses ESP32MQTTClient library (https://github.com/cyijun/ESP32MQTTClient)
   - Test connection: MQTTManager::instance().testConnection(testCreds, errorMsg)
     - Safely tests credentials by stopping production client, testing, then restoring
     - Thread-safe, blocks for max 5 seconds while testing
     - Feeds watchdog timer during test to prevent timeout
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

## RAII Lock Guard Pattern (REQUIRED for All Managers)

**All thread-safe managers MUST use ManagerLock RAII for mutex protection:**

```cpp
#include <core/ManagerLock.h>

void MyManager::publicMethod() {
    ManagerLock lock(mutex, "MYMANAGER", timeoutMs);  // Optional timeout (default: portMAX_DELAY)
    if (!lock.isLocked()) {
        LOG_ERROR("MYMANAGER", "Failed to acquire mutex!");
        return;
    }

    // ... protected work ...

    // Mutex automatically released when lock goes out of scope
}
```

**Benefits**:

- **Prevents mutex leaks** from missed unlocks on error paths
- **Exception-safe** (rare on ESP32, but possible)
- **Cleaner code** - no manual unlock tracking needed
- **Consistent pattern** across all managers
- **Better debugging** - manager name in timeout logs

**Never use manual xSemaphoreTake/xSemaphoreGive in manager public methods.**

All managers now use ManagerLock:

- ✅ **LedEffects** - Uses ManagerLock (formerly LedLock)
- ✅ **PrinterManager** - Uses ManagerLock (formerly PrinterLock)
- ✅ **ConfigManager** - Uses ManagerLock (formerly manual locks)
- ✅ **MQTTManager** - Uses ManagerLock (formerly manual locks)
- ✅ **APIClient** - Uses ManagerLock (formerly manual locks)

## Deadlock Prevention Guidelines

**CRITICAL**: Follow these rules to prevent mutex deadlocks in multi-threaded code:

### 1. Never Call Other Managers While Holding a Lock

```cpp
// ❌ DANGEROUS - Can cause deadlock
void ConfigManager::saveWithLog() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    // ... config work ...
    LogManager::instance().log("Saved config");  // BAD: LogManager may try to access ConfigManager
    xSemaphoreGive(mutex);
}

// ✅ SAFE - Release lock before calling other managers
void ConfigManager::saveWithLog() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    // ... config work ...
    xSemaphoreGive(mutex);

    LogManager::instance().log("Saved config");  // GOOD: Lock released first
}
```

### 2. Keep Critical Sections Minimal

```cpp
// ❌ BAD - Holds lock during slow network I/O
xSemaphoreTake(mutex, portMAX_DELAY);
String data = fetchFromAPI(url);  // Blocks for seconds!
processData(data);
xSemaphoreGive(mutex);

// ✅ GOOD - Lock only for shared state access
String url_copy;
xSemaphoreTake(mutex, portMAX_DELAY);
url_copy = shared_url;
xSemaphoreGive(mutex);

String data = fetchFromAPI(url_copy);  // Network I/O outside lock
processData(data);
```

### 3. Establish Lock Ordering (If Multiple Locks Required)

If you **must** lock multiple managers, always do it in the **same order** everywhere:

```cpp
// Define global lock order: Config → MQTT → API → Log
// ALWAYS follow this order, NEVER reverse it

// ✅ SAFE - Follows lock order
xSemaphoreTake(ConfigManager::instance().mutex, portMAX_DELAY);
xSemaphoreTake(MQTTManager::instance().mutex, portMAX_DELAY);
// ... work ...
xSemaphoreGive(MQTTManager::instance().mutex);
xSemaphoreGive(ConfigManager::instance().mutex);

// ❌ DANGER - Reverse order can deadlock with above code
xSemaphoreTake(MQTTManager::instance().mutex, portMAX_DELAY);
xSemaphoreTake(ConfigManager::instance().mutex, portMAX_DELAY);  // DEADLOCK!
```

### 4. FreeRTOS Mutex Limitations

**Note**: This codebase uses FreeRTOS `SemaphoreHandle_t`, not C++ `std::mutex`:

- ✅ Manual `xSemaphoreTake()` / `xSemaphoreGive()` is the correct pattern
- ❌ No native RAII wrapper like `std::lock_guard` available
- ⚠️ **Must manually ensure `xSemaphoreGive()` in all code paths** (error returns, early exits)
- ⚠️ Exception safety is limited - avoid exceptions in critical sections

### 5. Avoid Blocking Operations Inside Locks

```cpp
// ❌ BAD - Network I/O, file I/O, delays while holding lock
xSemaphoreTake(mutex, portMAX_DELAY);
delay(1000);  // BAD: Blocks other tasks
httpClient->GET();  // BAD: Network I/O
LittleFS.open(...);  // BAD: File I/O
xSemaphoreGive(mutex);

// ✅ GOOD - Only protect shared state, not I/O
String url_copy, token_copy;
xSemaphoreTake(mutex, portMAX_DELAY);
url_copy = shared_url;
token_copy = shared_token;
xSemaphoreGive(mutex);

// I/O operations outside lock
int result = httpClient->GET();
```

### Common Deadlock Scenarios to Avoid

1. **Cross-manager dependencies** - Manager A calls Manager B while holding lock
2. **Inconsistent lock ordering** - Task 1 locks A→B, Task 2 locks B→A
3. **Callback chains** - MQTT callback triggers HTTP request while MQTT lock held
4. **Interrupt handlers** - ISR tries to acquire mutex (use `logfISR()` instead)

### Debugging Deadlocks

If the system freezes:

1. Check serial output for "Failed to acquire mutex" errors
2. Enable VERBOSE logging to trace lock acquisition order
3. Look for blocking operations (delay, network, file I/O) inside locks
4. Verify lock acquisition/release symmetry in all code paths
5. Check FreeRTOS task states with `vTaskList()` (if deadlock suspected)

</thread_safe_architecture>

<partition_tables>

## Partition Tables and Flash Layout

### Critical Alignment Requirement

**App partitions MUST start at 0x10000 (64KB) aligned addresses.**

This is a bootloader requirement for ESP32/ESP32-S3. Incorrect alignment causes build failure:

```
Partition app0 invalid: Offset 0x13000 is not aligned to 0x10000
*** [partitions.bin] Error 2
```

### Valid Alignment Examples

✅ **Valid offsets** (multiples of 0x10000):

- `0x10000` (64KB) - Can be used if NVS+otadata fit before it
- `0x20000` (128KB) - **Used in our partition tables** (NVS expanded to 32KB)
- `0x30000` (192KB)
- `0x2A0000` (2.625MB) - **Used for app1 in 8MB OTA layout**
- `0x400000` (4MB)

❌ **Invalid offsets** (NOT multiples of 0x10000):

- `0x12000` ← Will fail
- `0x13000` ← Will fail
- `0x15000` ← Will fail
- `0x402000` ← Will fail for app partitions (OK for data partitions)

**Important:** With 32KB NVS (0x9000-0x11000) + 8KB OTA data (0x11000-0x13000), the first aligned app partition start is **0x20000**, not 0x10000.

### Partition Table Files

**Three partition table variants** (see CSV files in project root for details):

- `partitions_8mb_ota.csv` - ESP32-S3-custom-PCB (OTA support)
- `partitions_4mb_no_ota.csv` - ESP32-C3/S3-mini boards
- `partitions_8mb_no_ota.csv` - ESP32-S3 8MB without OTA

**Key constraint:** All use **32KB NVS** starting at 0x9000, which pushes app partitions to start at **0x20000** (next aligned boundary after NVS + phy_init/otadata).

### NVS Storage Notes

**Current size: 32KB (0x8000)** - Increased from 20KB to prevent exhaustion.

NVS stores ~52 configuration keys (see `nvs_keys.h`):

- Device settings (owner, timezone, board_type)
- GPIO pin assignments (printer, buttons, LEDs)
- WiFi credentials (SSID, password)
- MQTT configuration (server, port, credentials)
- Button actions and LED effects (24 keys)
- Memos (4 slots, 500 chars each)
- Unbidden Ink settings

**Why 32KB?**

- NVS uses write-once pages with wear leveling
- Repeated config saves create invalid entries that consume space
- 32KB provides 2.67× headroom (12KB typical usage → 32KB capacity)
- Optimized `saveNVSConfigInternal()` only writes changed keys to reduce churn

### Partition Layout Changes

When modifying partition tables:

1. **Always maintain 0x10000 alignment** for app partitions
2. Update size comments to reflect actual partition sizes
3. Test build: `pio run -e <environment>`
4. **Require full flash erase** after partition changes: `pio run --target erase -e <env>`

</partition_tables>
