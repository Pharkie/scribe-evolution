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
- **Default Printer**: TX=GPIO 44, RX=GPIO 43, DTR=GPIO 15 (UART1)
- **Default LED Data**: GPIO 14
- **Default Buttons**: GPIO 5, 6, 7, 8
- **Status LED**: GPIO 8
- **eFuse Support**: None
- **Features**: Dual core Xtensa, more RMT channels, USB OTG

### ESP32-S3-custom-PCB (Custom PCB with eFuse Protection)

- **File**: `board_esp32s3_custom_pcb.h`
- **Build Flag**: `-DBOARD_ESP32S3_CUSTOM_PCB`
- **Base**: Extends ESP32-S3-mini
- **eFuse Enables**: Printer=GPIO 9, LED strip=GPIO 10
- **Features**: All S3-mini features + overcurrent protection

## Architecture

### Board Selection Flow

1. PlatformIO sets board flag in `build_flags` (e.g., `-DBOARD_ESP32C3_MINI`)
2. `board_config.h` detects flag and includes correct board header
3. Compile-time macros provide board-specific defaults
4. Runtime functions validate against hardware

### Key Files

**Board Interface** (`board_interface.h`)

- Defines common structures all boards must implement
- `GPIOInfo`, `BoardConstraints`, `BoardPinDefaults`
- Validation function signatures: `isValidGPIO()`, `isSafeGPIO()`, `getGPIODescription()`

**Board Selector** (`board_config.h`)

- Auto-detects board type from build flags
- Includes correct board-specific header
- Provides `validateBoardMatch()` for runtime checking
- Compile-time error if board unsupported

**Individual Board Files**

- Implement board interface
- Define GPIO maps (safe/avoid classification)
- Provide hardware constraints (strapping pins, max GPIO, etc.)
- Set board-specific defaults (printer, LEDs, buttons)

## Usage Patterns

### In Code - Use Board Macros

```cpp
// Get board-specific defaults
const BoardPinDefaults &defaults = getBoardDefaults();

// Access printer configuration
int printerTX = defaults.printer.tx;
int printerRX = defaults.printer.rx;  // -1 if not used
int printerDTR = defaults.printer.dtr; // -1 if not present

// Check eFuse support (compile-time)
#if BOARD_HAS_PRINTER_EFUSE
    pinMode(defaults.efuse.printer, OUTPUT);
    digitalWrite(defaults.efuse.printer, HIGH);
#endif

// Validate GPIO at runtime
if (!isValidGPIO(pin)) {
    LOG_ERROR("Invalid GPIO %d for %s", pin, BOARD_NAME);
}

if (!isSafeGPIO(pin)) {
    LOG_WARNING("GPIO %d: %s", pin, getGPIODescription(pin));
}
```

### In Configuration - Reference Board Defaults

```cpp
// system_constants.h
static const int defaultPrinterTxPin = BOARD_DEFAULT_PRINTER_TX;
static const int statusLEDPin = BOARD_STATUS_LED_PIN;
static const ButtonConfig *defaultButtons = BOARD_DEFAULT_BUTTONS;
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

Create `board_yourboard.h` implementing the interface:

```cpp
#include "board_interface.h"

// GPIO map with safety classifications
static const GPIOInfo YOURBOARD_GPIO_MAP[] = {
    {0, GPIO_TYPE_AVOID, "Avoid: Strapping pin"},
    {1, GPIO_TYPE_SAFE, "Safe"},
    // ... all GPIOs
};

// Hardware constraints
static const BoardConstraints YOURBOARD_CONSTRAINTS = {
    .maxGPIO = 21,
    .strappingPins = ...,
    .avoidPins = ...,
    .gpioMap = YOURBOARD_GPIO_MAP,
    .gpioMapSize = sizeof(YOURBOARD_GPIO_MAP) / sizeof(GPIOInfo)
};

// Default pin assignments
static const BoardPinDefaults YOURBOARD_DEFAULTS = {
    .boardName = "Your Board Name",
    .boardIdentifier = "YOURBOARD",
    .printer = {.tx = 21, .rx = -1, .dtr = -1},
    .ledDataPin = 20,
    .statusLedPin = 8,
    .buttons = ...,
    .buttonCount = 4,
    .efuse = {.printer = -1, .ledStrip = -1}
};

// Implement validation functions
inline bool isValidGPIO(int pin) { /* ... */ }
inline bool isSafeGPIO(int pin) { /* ... */ }
inline const char *getGPIODescription(int pin) { /* ... */ }
inline const BoardConstraints &getBoardConstraints() { return YOURBOARD_CONSTRAINTS; }
inline const BoardPinDefaults &getBoardDefaults() { return YOURBOARD_DEFAULTS; }

// Define macros for compile-time access
#define BOARD_NAME "Your Board Name"
#define BOARD_IDENTIFIER "YOURBOARD"
#define BOARD_MAX_GPIO 21
#define BOARD_HAS_PRINTER_EFUSE false
#define BOARD_HAS_LED_EFUSE false
#define BOARD_DEFAULT_PRINTER_TX YOURBOARD_DEFAULTS.printer.tx
// ... etc
```

### Step 2: Update Board Selector

Add detection to `board_config.h`:

```cpp
#elif defined(BOARD_YOURBOARD) || defined(ARDUINO_YOURBOARD)
    #include "board_yourboard.h"
    #define BOARD_DETECTED "Your Board Name"
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

**LED Strip eFuse** (GPIO 10 on custom S3 PCB)

- Controls LED strip power rail
- HIGH = power enabled
- LOW = power disabled
- Protects against LED strip shorts/overcurrent

**Initialization Order**:

1. Enable eFuse (HIGH)
2. Wait 10ms for stabilization
3. Initialize peripheral (UART/FastLED)

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
