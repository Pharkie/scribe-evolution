# Configuration System

## Overview

Scribe Evolution separates compile‑time defaults from runtime settings persisted in NVS (ESP32 Preferences).

## Core Principle

Defaults in `config.h` are ONLY seeds; runtime code must read from `RuntimeConfig` (loaded from NVS).

## Configuration Layers

### 1) `config.h` (compile‑time defaults)

- **Purpose**: Initial default values for first-time setup
- **Usage**: Only to populate missing values in `config.json`
- **Rule**: Application code should NEVER access these directly

### 2) NVS (runtime configuration)

- **Purpose**: Active configuration values
- **Usage**: Loaded at boot via `config_loader`; updated by web UI/API
- **Rule**: Single source of truth for user‑configurable settings

## File Structure

```
src/core/
├── config.h.example   # Defaults template (copy to config.h, gitignored)
├── config.h           # Local copy (not tracked)
├── config_loader.h    # Runtime configuration API
└── config_loader.cpp  # Loads/saves NVS
```

## Configuration Categories

### Runtime Defaults (User-Configurable)

These values in `config.h` are prefixed with `default*` and seed NVS on first boot:

```cpp
// Example: WiFi settings
static const char *defaultWifiSSID = "MooseCave";
static const char *defaultWifiPassword = "password123";

// Example: MQTT settings
static const char *defaultMqttServer = "mqtt.example.com";
static const int defaultMqttPort = 8883;
```

### Backend Constants (Fixed at Compile-time)

These values are NOT user‑configurable and are used directly:

```cpp
// Example: Hardware pins
static const int TX_PIN = 21;
static const int statusLEDPin = 8;

// Example: System limits
static const int maxCharacters = 1000;
static const unsigned long watchdogTimeoutSeconds = 8;
```

## Proper Usage Patterns

### ✅ CORRECT – Use `RuntimeConfig` (via `config_loader`)

```cpp
const RuntimeConfig& cfg = getRuntimeConfig();
String ssid = cfg.wifiSSID;
String password = cfg.wifiPassword;
String mqttServer = cfg.mqttServer;
```

### ❌ WRONG – Direct access to defaults

```cpp
// NEVER do this in application code
String ssid = defaultWifiSSID;  // BAD!
String password = defaultWifiPassword;  // BAD!
```

Internals: defaults are applied inside `config_loader` only when NVS keys are missing.

## Configuration Flow

1. On boot, `loadNVSConfig()` populates `RuntimeConfig` (creating missing keys with defaults).
2. Web UI/API updates write back to NVS.
3. Application code always reads from `RuntimeConfig`.

## Special Cases

### GPIO Pin Configuration

- **GPIO pins**: Fixed at compile‑time (hardware‑dependent constants)
- **Button actions/MQTT topics/LED effects**: User‑configurable via NVS

## Best Practices

1. **New Configuration Values**:
   - Add `default*` value in `config.h`
   - Add getter/setter methods in `ConfigManager`
   - Update web interface to allow user modification
   - Always access via `ConfigManager` in application code

2. **Constants vs Defaults**:
   - If users should be able to change it → Make it a `default*` value
   - If it's hardware-dependent or system critical → Make it a constant

3. **Validation**:
   - Always validate configuration values when loading from `config.json`
   - Fall back to defaults for invalid values
   - Log configuration issues appropriately

## Testing Configuration Changes

When modifying config:

1. Erase NVS or remove relevant keys to test default seeding
2. Verify UI changes persist across reboots
3. Confirm no direct access to `default*` variables

## Security Considerations

- NVS contains sensitive data (WiFi passwords, API tokens)
- Defaults in `config.h` are compiled into firmware; keep the file gitignored
- All config endpoints require authentication (STA mode) and CSRF for POST

---

**Remember**: Default values are like a template - they're only used to create
the initial configuration file. Once `config.json` exists, it becomes the
authoritative source for all runtime settings.
