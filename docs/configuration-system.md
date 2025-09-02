# Configuration System Architecture

## Overview

The Scribe Evolution project uses a dual-layer configuration system designed to separate
compile-time constants from runtime user settings. This document outlines the
rules and architecture to ensure proper usage.

## Core Principle

**Default values in `config.h` should ONLY be used to populate `config.json`,
never used directly by application code.**

## Configuration Layers

### 1. config.h - Compile-time Defaults

- **Purpose**: Initial default values for first-time setup
- **Usage**: Only to populate missing values in `config.json`
- **Rule**: Application code should NEVER access these directly

### 2. config.json - Runtime Configuration

- **Purpose**: Active configuration values used by the application
- **Usage**: Loaded at boot time, overrides all defaults
- **Rule**: This is the single source of truth for all user-configurable
  settings

## File Structure

```
src/core/
├── config.h          # Compile-time defaults and constants
└── config_manager.h  # Runtime configuration management
data/
└── config.json       # User configuration (overrides defaults)
```

## Configuration Categories

### Runtime Defaults (User-Configurable)

These values in `config.h` are prefixed with `default*` and can be overridden by
`config.json`:

```cpp
// Example: WiFi settings
static const char *defaultWifiSSID = "MooseCave";
static const char *defaultWifiPassword = "password123";

// Example: MQTT settings
static const char *defaultMqttServer = "mqtt.example.com";
static const int defaultMqttPort = 8883;
```

### Backend Constants (Fixed at Compile-time)

These values are NOT user-configurable and are used directly:

```cpp
// Example: Hardware pins
static const int TX_PIN = 21;
static const int statusLEDPin = 8;

// Example: System limits
static const int maxCharacters = 1000;
static const unsigned long watchdogTimeoutSeconds = 8;
```

## Proper Usage Patterns

### ✅ CORRECT - Use ConfigManager

```cpp
// Application code should use ConfigManager
String ssid = configManager.getWifiSSID();
String password = configManager.getWifiPassword();
String mqttServer = configManager.getMqttServer();
```

### ❌ WRONG - Direct access to defaults

```cpp
// NEVER do this in application code
String ssid = defaultWifiSSID;  // BAD!
String password = defaultWifiPassword;  // BAD!
```

### ✅ CORRECT - ConfigManager initialization

```cpp
// Only ConfigManager should use defaults for initialization
void ConfigManager::loadDefaults() {
    setWifiSSID(defaultWifiSSID);  // OK - only for populating missing config.json values
    setWifiPassword(defaultWifiPassword);
}
```

## Configuration Flow

```
Boot Sequence:
1. ConfigManager::loadFromFile() reads config.json
2. If config.json missing/incomplete → Use defaults to create/populate it
3. All runtime code uses configManager.getXXX(), never defaultXXX
```

## Special Cases

### GPIO Pin Configuration

As noted in `config.h`:

```cpp
// Note GPIO are fixed in config.h, but endpoints can be changed in config.json
```

- **GPIO pins**: Fixed at compile-time (hardware-dependent)
- **Button actions/endpoints**: User-configurable via `config.json`

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

When modifying the configuration system:

1. **Delete `config.json`** to test default population
2. **Verify all defaults** are properly loaded into `config.json`
3. **Test configuration persistence** through web interface
4. **Confirm no direct access** to `default*` variables in application code

## Security Considerations

- `config.json` may contain sensitive data (WiFi passwords, API tokens)
- Default values in `config.h` are compiled into firmware
- Use appropriate access controls for configuration endpoints
- Consider encryption for sensitive configuration data

---

**Remember**: Default values are like a template - they're only used to create
the initial configuration file. Once `config.json` exists, it becomes the
authoritative source for all runtime settings.
