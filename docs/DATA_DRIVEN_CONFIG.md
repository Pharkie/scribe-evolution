# Data-Driven Configuration System

## Overview

The Scribe project uses a **data-driven configuration system** that eliminates hardcoded field validation and provides automatic partial updates for any configuration field. This system is implemented in `src/web/config_field_registry.h/.cpp`.

## Architecture

### Single Source of Truth
All configuration fields are defined once in the `CONFIG_FIELDS[]` array:

```cpp
const ConfigFieldDef CONFIG_FIELDS[] = {
    {"device.owner", ValidationType::NON_EMPTY_STRING, offsetof(RuntimeConfig, deviceOwner), 0, 0, nullptr, 0},
    {"buttons.button1.gpio", ValidationType::GPIO, offsetof(RuntimeConfig, buttonGpios[0]), 0, 0, nullptr, 0},
    {"leds.brightness", ValidationType::RANGE_INT, offsetof(RuntimeConfig, ledBrightness), 0, 255, nullptr, 0},
    // etc...
};
```

### Validation Types
- `STRING` - Any string value (including empty)
- `NON_EMPTY_STRING` - String that cannot be empty
- `GPIO` - GPIO pin number (validates safety using `isValidGPIO()` and `isSafeGPIO()`)
- `RANGE_INT` - Integer within min/max range
- `RANGE_FLOAT` - Float within min/max range
- `BOOLEAN` - Boolean true/false
- `ENUM_STRING` - String that must match one of provided options

### Automatic Processing
The config handler uses **8 lines** to process all fields:

```cpp
// Load current configuration for partial updates
RuntimeConfig newConfig = getRuntimeConfig();

// Data-driven configuration processing - handles ALL fields generically
String errorMsg;
if (!processJsonObject("", doc.as<JsonObject>(), newConfig, errorMsg)) {
    sendValidationError(request, ValidationResult(false, errorMsg));
    return;
}
```

## Adding New Configuration Fields

### Step 1: Add to RuntimeConfig struct
```cpp
// In src/core/runtime_config.h
struct RuntimeConfig {
    // ... existing fields ...
    int newIntField;
    String newStringField;
};
```

### Step 2: Add to CONFIG_FIELDS registry
```cpp
// In src/web/config_field_registry.cpp
const ConfigFieldDef CONFIG_FIELDS[] = {
    // ... existing fields ...
    {"section.newIntField", ValidationType::RANGE_INT, offsetof(RuntimeConfig, newIntField), 1, 100, nullptr, 0},
    {"section.newStringField", ValidationType::NON_EMPTY_STRING, offsetof(RuntimeConfig, newStringField), 0, 0, nullptr, 0},
};
```

### Step 3: Update CONFIG_FIELDS_COUNT (automatic)
The count is calculated automatically: `sizeof(CONFIG_FIELDS) / sizeof(CONFIG_FIELDS[0])`

## Field Path Convention

Use **dot notation** to represent nested JSON structure:
- `"device.owner"` → `{ "device": { "owner": "value" } }`
- `"buttons.button1.gpio"` → `{ "buttons": { "button1": { "gpio": 4 } } }`
- `"leds.brightness"` → `{ "leds": { "brightness": 128 } }`

## Validation Examples

### GPIO Validation
```cpp
{"device.printerTxPin", ValidationType::GPIO, offsetof(RuntimeConfig, printerTxPin), 0, 0, nullptr, 0}
```
- Automatically calls `isValidGPIO()` and `isSafeGPIO()`
- Generates error: `"device.printerTxPin invalid GPIO pin: 6 - GPIO 6 is used for flash memory"`

### Range Validation
```cpp
{"leds.brightness", ValidationType::RANGE_INT, offsetof(RuntimeConfig, ledBrightness), 0, 255, nullptr, 0}
```
- Validates value is between 0-255
- Generates error: `"leds.brightness must be between 0 and 255"`

### Enum Validation
```cpp
{"buttons.button1.shortAction", ValidationType::ENUM_STRING, offsetof(RuntimeConfig, buttonShortActions[0]), 0, 0, VALID_BUTTON_ACTIONS, VALID_BUTTON_ACTIONS_COUNT}
```
- Validates against predefined string array
- Generates error: `"buttons.button1.shortAction invalid value: INVALID_ACTION"`

## Partial Update Examples

The system automatically handles any combination of fields:

```javascript
// Update single field
POST /api/config { "device": { "owner": "NewName" } }

// Update multiple fields across sections  
POST /api/config {
    "device": { "timezone": "America/New_York" },
    "leds": { "brightness": 128 },
    "buttons": { "button1": { "gpio": 4 } }
}

// Update array element
POST /api/config { "buttons": { "button2": { "shortAction": "JOKE" } } }
```

## Benefits Over Hardcoded Approach

### Before (200+ lines per handler)
```cpp
if (doc.containsKey("device")) {
    if (!device.containsKey("owner")) {
        sendValidationError(request, "Missing device owner");
        return;
    }
    String owner = device["owner"];
    if (owner.length() == 0) {
        sendValidationError(request, "Device owner cannot be empty");
        return;
    }
    newConfig.deviceOwner = owner;
    // Repeat for every field...
}
```

### After (8 lines total)
```cpp
String errorMsg;
if (!processJsonObject("", doc.as<JsonObject>(), newConfig, errorMsg)) {
    sendValidationError(request, ValidationResult(false, errorMsg));
    return;
}
```

### Advantages
- ✅ **DRY**: Single field definition, no duplication
- ✅ **Type Safe**: Compile-time field offsets using `offsetof`
- ✅ **Maintainable**: Add field = 1 line, not 20+ lines
- ✅ **Consistent**: Automatic error message generation
- ✅ **Reliable**: No hardcoded field names to get out of sync
- ✅ **ESP32 Optimized**: Static arrays, no dynamic allocation
- ✅ **Partial Updates**: Any valid key:value combination works automatically

## Error Handling

The system provides detailed, consistent error messages:
- `"Unknown configuration field: invalid.field"`
- `"device.owner cannot be empty"`  
- `"leds.brightness must be between 0 and 255"`
- `"buttons.button1.gpio invalid GPIO pin: 6 - GPIO 6 is used for flash memory"`

## Memory Usage

- **Static arrays only** - no dynamic allocation
- **Compile-time field mapping** using `offsetof()` macro
- **Minimal stack usage** - validation happens in-place
- **ESP32 friendly** - designed for microcontroller constraints

## Future Enhancements

1. **LED Effects**: Make LED effect configuration data-driven
2. **Validation Parameters**: Support more complex validation rules  
3. **Conditional Fields**: Fields that depend on other field values
4. **Array Handling**: Better support for dynamic arrays
5. **Schema Export**: Generate JSON schema from field definitions

## Testing

Test the system by sending partial configuration updates:

```bash
# Test single field update
curl -X POST http://device-ip/api/config \
  -H "Content-Type: application/json" \
  -d '{"device":{"owner":"TestUser"}}'

# Test validation error
curl -X POST http://device-ip/api/config \
  -H "Content-Type: application/json" \
  -d '{"device":{"printerTxPin":999}}'
```

This system is the foundation for all settings pages - each page can send any subset of configuration fields and they will be validated and applied automatically.