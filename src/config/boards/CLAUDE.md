# Board Configuration System

<system_context>
Multi-board GPIO configuration system for Scribe Evolution.
Supports ESP32-C3-mini, ESP32-S3-mini, and custom PCB variants.
Compile-time board selection with runtime validation.
</system_context>

<critical_notes>

- Board type is auto-detected from PlatformIO build flags
- GPIO mappings are board-specific and fail-fast
- Board mismatch triggers automatic GPIO reset to new defaults
- eFuse protection circuits only on custom PCB builds
- Never hardcode GPIO pins - use board defaults
  </critical_notes>

## Supported Boards

### ESP32-C3-mini (Standard Development Board)

- **File**: `board_esp32c3_mini.h`
- **Build Flag**: `-DBOARD_ESP32C3_MINI`
- **GPIO Range**: 0-21 (22 pins)
- **Default Printer TX**: GPIO 21 (UART1)
- **Default LED Data**: GPIO 20
- **Default Buttons**: GPIO 5, 6, 7, 4
- **Status LED**: GPIO 8 (built-in)
- **eFuse Support**: None
- **Features**: Single core RISC-V, USB Serial/JTAG

### ESP32-S3-mini (Standard Development Board)

- **File**: `board_esp32s3_mini.h`
- **Build Flag**: `-DBOARD_ESP32S3_MINI`
- **GPIO Range**: 0-48 (49 pins)
- **Default Printer**: TX=GPIO 44, RX=-1 (disabled), DTR=-1 (disabled) (UART1)
- **Default LED Data**: GPIO 14
- **Default Buttons**: GPIO 5, 6, 7, 8
- **Status LED**: GPIO 8 (simple digital output)
- **eFuse Support**: None
- **Features**: Dual core Xtensa, more RMT channels, USB OTG

### ESP32-S3-custom-PCB (Custom PCB with eFuse Protection)

- **File**: `board_esp32s3_custom_pcb.h`
- **Build Flag**: `-DBOARD_ESP32S3_CUSTOM_PCB`
- **Base**: Extends ESP32-S3-mini
- **Default Printer**: TX=GPIO 43, RX=GPIO 44, DTR=GPIO 15 (bidirectional UART with flow control)
- **Default LED Data**: GPIO 14 (WS2812 strip)
- **Default Buttons**: GPIO 5, 6, 7, 8
- **Status LED**: GPIO 16 (WS2812 RGB LED with color-coded WiFi status)
- **eFuse Enables**: Printer=GPIO 9, LED strip=GPIO 10 (overcurrent protection)
- **eFuse Support**: Yes (BOARD_HAS_EFUSES=true)
- **Features**: All S3-mini features + overcurrent protection + bidirectional printer comms + RGB status LED

## Architecture

### Board Selection Flow

1. PlatformIO sets board flag in `build_flags` (e.g., `-DBOARD_ESP32C3_MINI`)
2. `board_config.h` detects flag and includes correct board header
3. Compile-time macros provide board-specific defaults
4. Runtime functions validate against hardware

### Key Files

**Board Selector** (`board_pins.h`)

- Auto-detects board type from PlatformIO build flags
- Includes correct board-specific header
- Provides simple accessor functions (optional convenience)
- Compile-time error if no board flag set

**Individual Board Headers** (`esp32c3_mini.h`, `esp32s3_mini.h`, `esp32s3_custom_pcb.h`)

- Define compile-time macros for all pin assignments (`BOARD_*` macros)
- Define board characteristics (`BOARD_NAME`, `BOARD_MAX_GPIO`, `BOARD_HAS_EFUSE`)
- Define `GPIOInfo` struct and map for GPIO safety documentation
- Implement inline validation functions: `isValidGPIO()`, `isSafeGPIO()`, `getGPIODescription()`
- Simple, self-contained, no complex interfaces

## Usage Patterns

### In Code - Use Board Macros

```cpp
// Access board-specific pin macros directly
int printerTX = BOARD_PRINTER_TX_PIN;
int ledPin = BOARD_LED_STRIP_PIN;
int statusLED = BOARD_STATUS_LED_PIN;
int button0 = BOARD_BUTTON_PINS[0];  // Array of button pins
int button1 = BOARD_BUTTON_PINS[1];

// Check board characteristics
const char* boardName = BOARD_NAME;          // "ESP32-C3-mini", etc.
int maxGPIO = BOARD_MAX_GPIO;                // 21 or 48

// Check eFuse support (compile-time)
#if BOARD_HAS_EFUSES
    // Custom PCB only - enable power control
    // NOTE: eFuses are enabled in peripheral init (printer.cpp, LedEffects.cpp)
    // NOT in main.cpp - they enable right before peripheral initialization
#endif

// Validate GPIO at runtime
if (!isValidGPIO(pin)) {
    LOG_ERROR("Invalid GPIO %d for %s", pin, BOARD_NAME);
}

if (!isSafeGPIO(pin)) {
    LOG_WARNING("GPIO %d: %s", pin, getGPIODescription(pin));
}
```

### In Configuration - Reference Board Macros

```cpp
// config.h or system_constants.h
// Board macros are available after including board_pins.h
static const int defaultPrinterTxPin = BOARD_PRINTER_TX_PIN;
static const int statusLEDPin = BOARD_STATUS_LED_PIN;
static const int defaultButtons[4] = {
    BOARD_BUTTON_PINS[0],
    BOARD_BUTTON_PINS[1],
    BOARD_BUTTON_PINS[2],
    BOARD_BUTTON_PINS[3]
};
```

### PlatformIO Environments

```ini
[env:c3-4mb-prod]
extends = env:esp32c3_base
build_flags =
    ${env:esp32c3_base.build_flags}
    -DBOARD_ESP32C3_MINI  # Required!
    -DENABLE_LEDS=1

[env:s3-pcb-prod]
extends = env:esp32s3_base
build_flags =
    ${env:esp32s3_base.build_flags}
    -DBOARD_ESP32S3_CUSTOM_PCB  # Custom PCB variant
    -DENABLE_LEDS=1
```

## Board Mismatch Handling

When firmware detects board type doesn't match saved NVS:

1. **Detection**: Compare saved `NVS_BOARD_TYPE` vs `BOARD_IDENTIFIER`
2. **Reset GPIOs**: All GPIO configs reset to new board defaults
3. **Update NVS**: Save new board type
4. **Log Warning**: Clear visual warning displayed

**Example**:

```
╔═══════════════════════════════════════════════════════════╗
║  ⚠️  BOARD MISMATCH DETECTED - RESETTING GPIO CONFIGS  ⚠️  ║
╠═══════════════════════════════════════════════════════════╣
║  Saved Board:   C3_MINI                                   ║
║  Current Board: S3_MINI                                   ║
║  Resetting all GPIO pins to new board defaults...        ║
╚═══════════════════════════════════════════════════════════╝
```

## Adding a New Board

### Step 1: Create Board Header

Create `boards/yourboard.h` following the simple pattern:

```cpp
#ifndef YOURBOARD_H
#define YOURBOARD_H

// ============================================================================
// BOARD IDENTIFICATION
// ============================================================================
#define BOARD_NAME "Your Board Name"
#define BOARD_MAX_GPIO 21
#define BOARD_HAS_EFUSES false  // true only if board has eFuse power control

// ============================================================================
// PIN ASSIGNMENTS (customize for your board)
// ============================================================================
#define BOARD_LED_STRIP_PIN 20
#define BOARD_PRINTER_TX_PIN 21
#define BOARD_STATUS_LED_PIN 8
static const int BOARD_BUTTON_PINS[4] = {4, 5, 6, 7};

// Optional: eFuse pins (only if BOARD_HAS_EFUSE is true)
// #define BOARD_EFUSE_PRINTER_PIN 9
// #define BOARD_EFUSE_LED_PIN 10

// ============================================================================
// GPIO VALIDATION DATA
// ============================================================================
enum GPIOType {
    GPIO_TYPE_SAFE = 0,
    GPIO_TYPE_AVOID = 1
};

struct GPIOInfo {
    int pin;
    GPIOType type;
    const char* description;
};

// GPIO map (for documentation and validation)
static const GPIOInfo BOARD_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Strapping pin"},
    {4, GPIO_TYPE_SAFE, "Button 0"},
    {5, GPIO_TYPE_SAFE, "Button 1"},
    // ... document all pins
};

static const int BOARD_GPIO_MAP_SIZE = sizeof(BOARD_GPIO_MAP) / sizeof(BOARD_GPIO_MAP[0]);
static const int BOARD_AVOID_PINS[] = {0, 9, 18, 19};  // Pins to avoid

// ============================================================================
// VALIDATION FUNCTIONS (inline for zero runtime cost)
// ============================================================================
inline bool isValidGPIO(int pin) {
    return (pin >= 0 && pin <= BOARD_MAX_GPIO);
}

inline bool isSafeGPIO(int pin) {
    for (int i = 0; i < sizeof(BOARD_AVOID_PINS) / sizeof(int); i++) {
        if (pin == BOARD_AVOID_PINS[i]) return false;
    }
    return isValidGPIO(pin);
}

inline const char* getGPIODescription(int pin) {
    for (int i = 0; i < BOARD_GPIO_MAP_SIZE; i++) {
        if (BOARD_GPIO_MAP[i].pin == pin) {
            return BOARD_GPIO_MAP[i].description;
        }
    }
    return "Unknown GPIO";
}

#endif // YOURBOARD_H
```

### Step 2: Update Board Selector

Add detection to `board_pins.h`:

```cpp
#elif defined(BOARD_YOURBOARD)
    #include "boards/yourboard.h"
#else
    #error "No board type defined!"
#endif
```

### Step 3: Add PlatformIO Environment

Create environment in `platformio.ini`:

```ini
[env:yourboard-prod]
platform = ...
board = ...
build_flags =
    ${common.build_flags_base}
    -DBOARD_YOURBOARD  # Required!
    ${common.build_flags_release}
    -DENABLE_LEDS=1
```

### Step 4: Test

```bash
pio run -e yourboard-prod
```

## GPIO Safety Classification

**GPIO_TYPE_SAFE**: Safe for general use

- Digital I/O
- No special boot functions
- Not connected to critical peripherals

**GPIO_TYPE_AVOID**: Should be avoided

- Strapping pins (affect boot mode)
- USB pins (D+/D-, critical for programming)
- Flash pins (SPI flash communication)
- Built-in peripherals (LEDs, buttons on devkit)

## eFuse Protection Circuits

Custom PCB feature for overcurrent protection:

**Printer eFuse** (GPIO 9 on custom S3 PCB)

- Controls printer power rail
- HIGH = power enabled
- LOW = power disabled
- Protects against printer shorts/overcurrent
- Enabled in `printer.cpp` `initialize()` method

**LED Strip eFuse** (GPIO 10 on custom S3 PCB)

- Controls LED strip power rail
- HIGH = power enabled
- LOW = power disabled
- Protects against LED strip shorts/overcurrent
- Enabled in `LedEffects.cpp` `begin()` method

**Initialization Pattern**:

```cpp
#if BOARD_HAS_EFUSES
    // Enable eFuse right before peripheral initialization
    pinMode(BOARD_EFUSE_PRINTER_PIN, OUTPUT);
    digitalWrite(BOARD_EFUSE_PRINTER_PIN, HIGH);
    LOG_VERBOSE("PRINTER", "Printer eFuse enabled (GPIO %d)", BOARD_EFUSE_PRINTER_PIN);
#endif

// Existing peripheral initialization delays provide sufficient stabilization
uart->begin(9600, SERIAL_8N1, config.printerRxPin, config.printerTxPin);
```

**Key Design Points**:

- eFuses enabled in peripheral initialization methods, not in main.cpp setup()
- No explicit stabilization delay needed - peripheral init provides sufficient time
- One-time setup functions (`initialize()`, `begin()`) ensure no repeated enables
- Compile-time conditionals ensure zero overhead on boards without eFuses

## Extensibility

The board system is designed for easy expansion:

**Variants of Existing Boards**:

- Create new header that `#include`s base board
- Override only what changes (see `board_esp32s3_custom_pcb.h`)

**Completely New Boards**:

- Implement full board interface
- No changes to core code needed
- Self-contained in boards directory

**Future Boards**:

- ESP32-C6 (newer, WiFi 6)
- ESP32-H2 (Thread/Zigbee)
- Custom PCB with different pin layouts
- All follow same pattern
