# Core System - CLAUDE.md

<system_context>
ESP32-C3/S3 system fundamentals: configuration, network, MQTT, logging.
Memory-constrained environment with dual-layer config architecture.
Thread-safe logging via LogManager singleton.
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

<logging_architecture>
LogManager Singleton:

- Thread-safe serial output via FreeRTOS queue
- Single writer task prevents concurrent Serial corruption
- Non-blocking LOG\_\* macros (drops on queue overflow)
- ISR-safe logfISR() for interrupt contexts
- Captures ESP_LOGx macros from ESP-IDF

NEVER use Serial.print() - ALWAYS use LOG_NOTICE, LOG_ERROR, LOG_WARNING, or LOG_VERBOSE
See docs/logging-system.md for complete documentation
</logging_architecture>
