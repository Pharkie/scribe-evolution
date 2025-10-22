# MQTT Topic Consolidation & Restructuring Plan

**Status:** ✅ COMPLETED - Implemented October 21, 2025
**Date:** October 19, 2025 (Planned) | October 21, 2025 (Implemented)
**Breaking Change:** ⚠️ YES - All existing MQTT subscriptions will break

## Objective

1. **Consolidate hardcoded MQTT topics** into DRY constants (single source of truth)
2. **Rebrand** from `scribe/` to `scribe-evolution/`
3. **Restructure** to RESTful pattern: resource-first hierarchy
4. **Update all documentation** to reflect new topic structure

## Current vs New Topic Structure

```
OLD PATTERN:
  scribe/{printerName}/print              → scribe/Pharkie/print
  scribe/printer-status/{printerId}       → scribe/printer-status/243d0813

NEW PATTERN:
  scribe-evolution/print/{printerName}      → scribe-evolution/print/Pharkie
  scribe-evolution/status/{printerId}       → scribe-evolution/status/243d0813
```

## Benefits

✅ **DRY:** Single source of truth per language (C++ and JavaScript)
✅ **Maintainable:** Change topic structure in one place
✅ **Type-safe:** Helper functions prevent typos
✅ **Self-documenting:** Function names explain purpose
✅ **RESTful:** Resource-first hierarchy (`resource/identifier`)
✅ **Branded:** Uses `scribe-evolution` namespace
✅ **Consistent:** Same pattern for all topics
✅ **Documented:** All examples updated to match new structure

## Implementation Plan

### PART 1: Create C++ MQTT Topic Constants & Helpers

**File:** `src/config/system_constants.h`
**Location:** After line 110 (after existing MQTT constants)

**Add new namespace:**

```cpp
// ============================================================================
// MQTT TOPIC STRUCTURE - DRY Constants
// ============================================================================
namespace MqttTopics {
    // Base namespace
    static const char* NAMESPACE = "scribe-evolution";

    // Resource types
    static const char* PRINT_RESOURCE = "print";
    static const char* STATUS_RESOURCE = "status";

    // Helper functions
    inline String buildPrintTopic(const String& printerName) {
        return String(NAMESPACE) + "/" + PRINT_RESOURCE + "/" + printerName;
    }

    inline String buildStatusTopic(const String& printerId) {
        return String(NAMESPACE) + "/" + STATUS_RESOURCE + "/" + printerId;
    }

    inline String buildStatusSubscription() {
        return String(NAMESPACE) + "/" + STATUS_RESOURCE + "/+";
    }

    inline String getStatusPrefix() {
        return String(NAMESPACE) + "/" + STATUS_RESOURCE + "/";
    }

    inline bool isStatusTopic(const String& topic) {
        return topic.startsWith(getStatusPrefix());
    }
}
```

### PART 2: Create JavaScript MQTT Constants

**File:** `src/js-source/config/mqtt.js` (NEW FILE)

**Content:**

```javascript
/**
 * MQTT Topic Structure Constants
 * Single source of truth for all MQTT topic patterns
 */

// Base namespace
export const MQTT_NAMESPACE = "scribe-evolution";

// Resource types
export const MQTT_PRINT_RESOURCE = "print";
export const MQTT_STATUS_RESOURCE = "status";

/**
 * Build print topic: scribe-evolution/print/{printerName}
 * @param {string} printerName - Human-readable printer name
 * @returns {string} Full MQTT topic
 */
export function buildPrintTopic(printerName) {
  return `${MQTT_NAMESPACE}/${MQTT_PRINT_RESOURCE}/${printerName}`;
}

/**
 * Build status topic: scribe-evolution/status/{printerId}
 * @param {string} printerId - Unique printer ID (from MAC)
 * @returns {string} Full MQTT topic
 */
export function buildStatusTopic(printerId) {
  return `${MQTT_NAMESPACE}/${MQTT_STATUS_RESOURCE}/${printerId}`;
}

/**
 * Build status subscription wildcard: scribe-evolution/status/+
 * @returns {string} MQTT subscription pattern
 */
export function buildStatusSubscription() {
  return `${MQTT_NAMESPACE}/${MQTT_STATUS_RESOURCE}/+`;
}
```

### PART 3: Refactor C++ Files to Use Helpers

#### 3.1: config_utils.h (2 changes)

**Line 45:**

```cpp
// OLD:
snprintf(derivedMqttTopic, sizeof(derivedMqttTopic), "scribe/%s/print", key);

// NEW:
String topic = MqttTopics::buildPrintTopic(key);
strncpy(derivedMqttTopic, topic.c_str(), sizeof(derivedMqttTopic));
```

**Line 53:**

```cpp
// OLD:
snprintf(otherTopics[index], sizeof(otherTopics[index]), "scribe/%s/print", key);

// NEW:
String topic = MqttTopics::buildPrintTopic(key);
strncpy(otherTopics[index], topic.c_str(), sizeof(otherTopics[index]));
```

#### 3.2: printer_discovery.cpp (1 change)

**Line 52:**

```cpp
// OLD:
String statusTopic = "scribe/printer-status/" + printerId;

// NEW:
String statusTopic = MqttTopics::buildStatusTopic(printerId);
```

#### 3.3: mqtt_handler.cpp (3 changes)

**Line 83:**

```cpp
// OLD:
if (topicStr.startsWith("scribe/printer-status/"))

// NEW:
if (MqttTopics::isStatusTopic(topicStr))
```

**Line 273:**

```cpp
// OLD:
String statusTopic = "scribe/printer-status/" + printerId;

// NEW:
String statusTopic = MqttTopics::buildStatusTopic(printerId);
```

**Line 332:**

```cpp
// OLD:
if (!mqttClient.subscribe("scribe/printer-status/+"))

// NEW:
if (!mqttClient.subscribe(MqttTopics::buildStatusSubscription().c_str()))
```

### PART 4: Refactor JavaScript Files to Use Helpers

#### 4.1: stores/index.js (2 changes + import)

**Line 1 (add import):**

```javascript
import { buildPrintTopic } from "../config/mqtt.js";
```

**Line 227:**

```javascript
// OLD:
const topic = `scribe/${printer.name}/print`;

// NEW:
const topic = buildPrintTopic(printer.name);
```

**Line 409:**

```javascript
// OLD:
? `scribe/${this.overlayPrinterName}/print`

// NEW:
? buildPrintTopic(this.overlayPrinterName)
```

### PART 5: Update Documentation Files

#### 5.1: docs/mqtt-integration.md (4 changes)

- **Line 98:** `scribe/{your-device-name}/print` → `scribe-evolution/print/{your-device-name}`
- **Line 103:** `scribe/alice/print` → `scribe-evolution/print/alice`
- **Line 104:** `scribe/office-main/print` → `scribe-evolution/print/office-main`
- **Line 152:** `"topic": "scribe/alice/print"` → `"topic": "scribe-evolution/print/alice"`

#### 5.2: docs/troubleshooting.md (2 changes)

- **Line 283:** `"scribe/+/print"` → `"scribe-evolution/print/+"`
- **Line 287:** `"scribe/yourprinter/print"` → `"scribe-evolution/print/yourprinter"`

#### 5.3: docs/apple-shortcuts.md (1 change)

- **Line 268:** `"scribe/+/print"` → `"scribe-evolution/print/+"`

#### 5.4: docs/pipedream-integration.md (3 changes)

- **Line 105:** `scribe/YourName/print` → `scribe-evolution/print/YourName`
- **Line 179:** `"scribe/Pharkie/print"` → `"scribe-evolution/print/Pharkie"`
- **Line 232:** `scribe/Pharkie/print` → `scribe-evolution/print/Pharkie`

#### 5.5: docs/home-assistant/ha-core.md (2 changes)

- **Line 170:** `"scribe/YourPrinterName/print"` → `"scribe-evolution/print/YourPrinterName"`
- **Line 184:** `scribe/Krists/print` → `scribe-evolution/print/Krists`

#### 5.6: docs/home-assistant/ha-supervised.md (2 changes)

- **Line 157:** `"scribe/YourPrinterName/print"` → `"scribe-evolution/print/YourPrinterName"`
- **Line 171:** `scribe/Krists/print` → `scribe-evolution/print/Krists`

#### 5.7: docs/home-assistant/ha-container.md (2 changes)

- **Line 436:** `"scribe/YourPrinterName/print"` → `"scribe-evolution/print/YourPrinterName"`
- **Line 450:** `scribe/Krists/print` → `scribe-evolution/print/Krists`

### PART 6: Build & Test

1. **Build frontend:** `npm run build`
2. **Build firmware:** `pio run -e s3-4mb-dev`
3. **Verify logs show new topics:**
   - Print: `scribe-evolution/print/Pharkie`
   - Status: `scribe-evolution/status/243d0813`
   - Subscription: `scribe-evolution/status/+`
4. **Test MQTT publish** to new topic structure
5. **Verify documentation** examples are accurate

## Breaking Changes

⚠️ **All existing MQTT subscriptions will break**

Users must update their automations from:

```
OLD:
  Print:  scribe/{name}/print
  Status: scribe/printer-status/{id}

NEW:
  Print:  scribe-evolution/print/{name}
  Status: scribe-evolution/status/{id}
```

## Files Modified Summary

### Created (2 files)

- `src/config/system_constants.h` - Add MqttTopics namespace
- `src/js-source/config/mqtt.js` - New JavaScript constants file

### Code Files (4 files, 7 locations)

- `src/core/config_utils.h` - Use buildPrintTopic() helper (2 locations)
- `src/core/printer_discovery.cpp` - Use buildStatusTopic() helper (1 location)
- `src/core/mqtt_handler.cpp` - Use all helpers (3 locations)
- `src/js-source/stores/index.js` - Import and use buildPrintTopic() (2 locations, + 1 import)

### Documentation Files (7 files, 16 locations)

- `docs/mqtt-integration.md` - Update examples (4 locations)
- `docs/troubleshooting.md` - Update MQTT debug commands (2 locations)
- `docs/apple-shortcuts.md` - Update subscription example (1 location)
- `docs/pipedream-integration.md` - Update integration examples (3 locations)
- `docs/home-assistant/ha-core.md` - Update HA examples (2 locations)
- `docs/home-assistant/ha-supervised.md` - Update HA examples (2 locations)
- `docs/home-assistant/ha-container.md` - Update HA examples (2 locations)

**Total:** 13 files, 24 locations updated

## Migration Notes for Users

When this is implemented, users will need to:

1. **Update Home Assistant automations** to use new topic structure
2. **Update Node-RED flows** to use new topic structure
3. **Update Pipedream workflows** to use new topic structure
4. **Update Apple Shortcuts** to use new topic structure
5. **Update any custom MQTT clients** or scripts
6. **Re-test all integrations** after updating

Consider creating a migration guide document when implementing this change.

## Future Considerations

After this refactoring, any new MQTT topics should:

1. Use the `MqttTopics` namespace helpers in C++
2. Use the `mqtt.js` helper functions in JavaScript
3. Follow the `scribe-evolution/{resource}/{identifier}` pattern
4. Be documented in all relevant places

This ensures consistency and maintainability going forward.
