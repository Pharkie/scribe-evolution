# eFuse GPIO Reset Behavior Analysis

**Date**: 2025-11-04
**Topic**: Understanding eFuse GPIO behavior during ESP32-S3 reset
**Board**: ESP32-S3 Custom PCB (8MB flash, no PSRAM)

---

## Summary

The eFuse GPIO pins (GPIO 9 for printer, GPIO 10 for LEDs) on the ESP32-S3 custom PCB are **NOT configured with pull-up or pull-down resistors** in software, and their default hardware state during reset is **high-impedance (Hi-Z)** or potentially **floating/undefined**. This means:

1. **During ESP32 reset** (RESET button pressed): The eFuse GPIOs revert to their default unconfigured state
2. **Power behavior**: Whether the eFuse stays enabled depends entirely on hardware design (external pull-up/pull-down resistors on the PCB)
3. **Software initialization**: eFuses are set HIGH only after `printerManager.initialize()` or `ledEffects().begin()` completes

**Critical Finding**: There is **NO code to maintain eFuse state across resets**. The software relies on hardware design to determine what happens during reset.

---

## eFuse GPIO Initialization Sequence

### Printer eFuse (GPIO 9)

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/hardware/printer.cpp`

**Initialization**: Lines 82-86

```cpp
// Enable printer eFuse if present (custom PCB only)
#if BOARD_HAS_EFUSES
    pinMode(BOARD_EFUSE_PRINTER_PIN, OUTPUT);
    digitalWrite(BOARD_EFUSE_PRINTER_PIN, HIGH);
    LOG_VERBOSE("PRINTER", "Printer eFuse enabled (GPIO %d)", BOARD_EFUSE_PRINTER_PIN);
#endif
```

**Boot Sequence**:

1. `main.cpp` line 216: `printerManager.initialize()` called
2. Printer mutex created (lines 58-67)
3. Mutex acquired for initialization (lines 69-75)
4. **eFuse enabled** (lines 82-86) - pinMode OUTPUT + digitalWrite HIGH
5. 100ms delay for cleanup (line 90)
6. UART initialization begins (line 94)
7. DTR pin configured if present (lines 97-102)
8. 500ms delay for ESP32-S3 UART hardware (line 105)
9. Printer marked ready (line 111)

**Key Observations**:

- eFuse is enabled **BEFORE** UART initialization (proper power sequencing)
- No explicit pull-up or pull-down resistor configuration (`INPUT_PULLUP` not used)
- 600ms total delay after eFuse enable before printer is ready (100ms + 500ms)

---

### LED eFuse (GPIO 10)

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/LedEffects.cpp`

**Initialization**: Lines 120-125

```cpp
// Enable LED eFuse if present (custom PCB only) - one-time init
#if BOARD_HAS_EFUSES
    pinMode(BOARD_EFUSE_LED_PIN, OUTPUT);
    digitalWrite(BOARD_EFUSE_LED_PIN, HIGH);
    LOG_VERBOSE("LEDS", "LED strip eFuse enabled (GPIO %d)", BOARD_EFUSE_LED_PIN);
#endif
```

**Boot Sequence**:

1. `main.cpp` line 262: `ledEffects().begin()` called (in setup, AFTER printer initialization)
2. LED mutex created (lines 100-106)
3. Mutex acquired for initialization (lines 110-115)
4. **eFuse enabled** (lines 120-125) - pinMode OUTPUT + digitalWrite HIGH
5. `reinitializeInternal()` called to configure FastLED (line 128)
6. FastLED initialization on configured GPIO pin (lines 189-261)
7. LED system marked ready (line 275)

**Key Observations**:

- eFuse is enabled **BEFORE** FastLED initialization (proper power sequencing)
- No explicit pull-up or pull-down resistor configuration
- LED initialization happens AFTER printer initialization in boot sequence

---

## Main Setup Sequence

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/main.cpp`

**Relevant Timeline**:

```
Line 216: printerManager.initialize()     // Printer eFuse enabled HERE
          ↓
Line 242: setupWebServerRoutes()
          ↓
Line 262: ledEffects().begin()             // LED eFuse enabled HERE (if ENABLE_LEDS)
          ↓
Line 331: printerManager.printStartupMessage()  // First printer use
```

**Critical Point**: eFuses are **NOT initialized in main setup()** - they are initialized **inside peripheral initialization functions** (`printer.cpp` and `LedEffects.cpp`)

---

## GPIO Default State During Reset

### ESP32-S3 Hardware Behavior

Based on ESP32 documentation and web research:

1. **At Reset**: All GPIO pins are set to **high-impedance (Hi-Z)** state with outputs disabled
2. **Pull resistors**: Most GPIOs have pull-up/pull-down **disabled by default** after reset
3. **Undefined state**: Without external resistors, GPIO level is **undefined** until software configures it
4. **Brief glitches**: Some GPIOs can briefly toggle during boot sequence (30ms spikes documented)

### GPIO 9 and GPIO 10 Specifics

**From board configuration** (`/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/config/boards/esp32s3_custom_pcb.h`):

```cpp
#define BOARD_EFUSE_PRINTER_PIN 9   // Line 34
#define BOARD_EFUSE_LED_PIN 10      // Line 35

// GPIO map documentation:
{9, GPIO_TYPE_SAFE, "Printer eFuse enable"},   // Line 75
{10, GPIO_TYPE_SAFE, "LED strip eFuse enable"}, // Line 76
```

**Status**: Both GPIOs are "safe" general-purpose I/O pins

- Not strapping pins (those are GPIO 0, 3, 45, 46)
- Not USB pins (GPIO 19, 20)
- Not flash pins
- **No special hardware behavior during reset**

### What Happens During RESET Button Press

**Without external pull resistors on the PCB**:

1. ESP32 enters reset
2. GPIO 9 and GPIO 10 revert to **high-impedance (Hi-Z)** state
3. Output drivers are **disabled**
4. Pin state becomes **floating/undefined**
5. Whether eFuse stays enabled depends on:
   - Hardware design of the eFuse circuit
   - External pull-up resistors (if any)
   - Capacitance holding charge temporarily
   - Leakage currents

**After reset completes**:

1. Bootloader runs
2. App starts (`setup()` in main.cpp)
3. Eventually reaches `printerManager.initialize()` (line 216)
4. Printer eFuse GPIO 9 set HIGH (600ms+ from reset)
5. Eventually reaches `ledEffects().begin()` (line 262)
6. LED eFuse GPIO 10 set HIGH (even later in boot)

**Time window**: There is a significant period (estimated 2-5 seconds based on boot messages) where eFuse GPIOs are in undefined state.

---

## Code Search Results

### No Pull-Up/Pull-Down Configuration Found

**Search performed**: Looked for `PULLUP`, `PULLDOWN`, `INPUT_PULLUP`, `INPUT_PULLDOWN` in all `.cpp` files

**Only result**:

```cpp
// /Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/hardware/hardware_buttons.cpp:113
pinMode(gpio, buttonActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);
```

This is for **button inputs only** - not for eFuse GPIOs.

### No eFuse State Preservation Code

**Search performed**: Looked for eFuse-related initialization, persistence, or state management

**Findings**:

- eFuse pins are set HIGH only during peripheral initialization
- No RTC memory storage of eFuse state
- No NVS storage of eFuse state
- No deep sleep handling of eFuse GPIOs
- No code to detect or recover from eFuse power loss

---

## Documentation Review

### Board Configuration CLAUDE.md

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/config/boards/CLAUDE.md`

**Lines 299-338**: eFuse Protection Circuits documentation

Key excerpts:

```markdown
**Printer eFuse** (GPIO 9 on custom S3 PCB)

- Controls printer power rail
- HIGH = power enabled
- LOW = power disabled
- Protects against printer shorts/overcurrent
- Enabled in `printer.cpp` `initialize()` method

**Initialization Pattern**:

- eFuses enabled in peripheral initialization methods, not in main.cpp setup()
- No explicit stabilization delay needed - peripheral init provides sufficient time
- One-time setup functions (`initialize()`, `begin()`) ensure no repeated enables
- Compile-time conditionals ensure zero overhead on boards without eFuses
```

**Key Design Point**: Documentation explicitly states "one-time setup" - **no mention of reset behavior or state preservation**.

### Hardware CLAUDE.md

**Location**: `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/hardware/CLAUDE.md`

**Lines 68-83**: eFuse initialization pattern example

Shows the same pattern used in actual code - no pull resistor configuration mentioned.

---

## Critical Questions Answered

### 1. When is the eFuse GPIO initialized during boot?

**Answer**:

- **Printer eFuse (GPIO 9)**: During `printerManager.initialize()` at line 216 of main.cpp
- **LED eFuse (GPIO 10)**: During `ledEffects().begin()` at line 262 of main.cpp (if ENABLE_LEDS)
- **Timing**: Approximately 2-5 seconds after power-on or reset

### 2. What is its default/initial state when the ESP32 powers up?

**Answer**:

- **High-impedance (Hi-Z)** with outputs disabled
- Pull-up and pull-down resistors are **NOT configured** in software
- Actual voltage level is **undefined/floating** without external hardware resistors
- ESP32 documentation warns: "Do not rely on default configuration values"

### 3. Does pressing RESET button cause the GPIO to go LOW (turning off power)?

**Answer**:

- **Indeterminate** - depends entirely on hardware PCB design
- Software sets GPIO to **high-impedance (Hi-Z)** during reset
- If PCB has **external pull-down resistor**: eFuse will go LOW (power OFF)
- If PCB has **external pull-up resistor**: eFuse will stay HIGH (power ON)
- If PCB has **no external resistor**: eFuse state is **undefined/floating**
- Capacitance on the eFuse circuit may hold state temporarily

### 4. Is there any code that explicitly maintains eFuse state across resets?

**Answer**:

- **NO** - No RTC memory storage, no NVS storage, no state preservation code
- eFuses are treated as "one-time initialization" during boot
- No detection or recovery mechanism if eFuse loses power unexpectedly

### 5. Are there any pull-up/pull-down resistors configured for the eFuse GPIO?

**Answer**:

- **NO** software-configured pull resistors
- `pinMode(BOARD_EFUSE_PRINTER_PIN, OUTPUT)` does NOT configure pull resistors
- `digitalWrite()` only sets output state when pin is configured as OUTPUT
- During reset, pin reverts to input mode with pulls disabled (default ESP32 state)

---

## Potential Issues and Risks

### Issue 1: Power Loss During Reset

**Scenario**: User presses RESET button during active printing or LED effect

**What happens**:

1. GPIO 9/10 revert to Hi-Z (undefined state)
2. If no external pull-up: Printer/LEDs may lose power
3. Printer loses state, may produce corrupted output
4. LED strip may glitch or turn off
5. After ~2-5 seconds, power restored when eFuse re-enabled

**Risk Level**: Medium to High (depends on PCB design)

### Issue 2: Undefined State Window

**Scenario**: Boot sequence delay creates window where eFuse state is undefined

**Duration**: ~2-5 seconds from reset to eFuse initialization

**What happens**:

- Printer may be unpowered during this window
- LED strip may be unpowered during this window
- If eFuse circuit has capacitance, may stay powered temporarily
- Behavior is non-deterministic without hardware specifications

**Risk Level**: Low to Medium (depends on PCB design and use case)

### Issue 3: No Error Detection

**Scenario**: eFuse circuit fails or loses power unexpectedly

**What happens**:

- Software has no way to detect eFuse failure
- No UART communication would indicate printer issue, but wouldn't identify eFuse as cause
- LED strip not working could be mistaken for LED configuration issue

**Risk Level**: Low (debugging complexity, not safety issue)

---

## Recommended Hardware Design

Based on this analysis, the **custom PCB should have**:

### Option A: Pull-Up Resistors (Recommended)

- **External 10kΩ pull-up resistors** on GPIO 9 and GPIO 10
- Keeps eFuse enabled during reset
- Printer/LEDs stay powered during software reset
- Most user-friendly behavior

### Option B: Pull-Down Resistors (Safe Default)

- **External 10kΩ pull-down resistors** on GPIO 9 and GPIO 10
- Keeps eFuse disabled during reset
- Printer/LEDs power off during software reset
- Safer for preventing unexpected power states

### Option C: Capacitor Hold (Current Design?)

- No external pull resistors
- Relies on eFuse circuit capacitance to maintain state during brief resets
- Works for quick resets but fails for extended power-off
- Not recommended for production hardware

---

## Recommendations for Software Improvement

### 1. Add Pull-Up Configuration (If Hardware Allows)

If the PCB design permits internal pull-ups to be effective, add this to initialization:

```cpp
#if BOARD_HAS_EFUSES
    // Configure eFuse GPIO with pull-up BEFORE setting as output
    pinMode(BOARD_EFUSE_PRINTER_PIN, INPUT_PULLUP);
    delay(10);  // Allow pull-up to stabilize
    pinMode(BOARD_EFUSE_PRINTER_PIN, OUTPUT);
    digitalWrite(BOARD_EFUSE_PRINTER_PIN, HIGH);
    LOG_VERBOSE("PRINTER", "Printer eFuse enabled (GPIO %d)", BOARD_EFUSE_PRINTER_PIN);
#endif
```

**Limitation**: Internal pull-ups are typically 45kΩ and may not provide enough current for eFuse circuits. External resistors are preferred.

### 2. Add eFuse State Detection

Add code to verify eFuse is actually controlling power (if feedback circuit exists):

```cpp
#if BOARD_HAS_EFUSES
    pinMode(BOARD_EFUSE_PRINTER_PIN, OUTPUT);
    digitalWrite(BOARD_EFUSE_PRINTER_PIN, HIGH);
    delay(50);  // Allow eFuse to stabilize

    // If there's a feedback pin to read eFuse state:
    // bool efuseEnabled = digitalRead(BOARD_EFUSE_FEEDBACK_PIN);
    // if (!efuseEnabled) {
    //     LOG_ERROR("PRINTER", "eFuse failed to enable!");
    // }
#endif
```

### 3. Document Hardware Dependencies

Add to board configuration documentation:

- Whether PCB has external pull resistors
- Expected eFuse behavior during reset
- Minimum capacitance requirements for eFuse circuit

---

## File References

All file paths referenced in this analysis:

- `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/hardware/printer.cpp` (lines 82-86)
- `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/leds/LedEffects.cpp` (lines 120-125)
- `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/main.cpp` (lines 216, 262)
- `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/config/boards/esp32s3_custom_pcb.h` (lines 34-35, 75-76)
- `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/config/boards/CLAUDE.md` (lines 299-338)
- `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/src/hardware/CLAUDE.md` (lines 68-83)

---

## Conclusion

The eFuse GPIO pins on the ESP32-S3 custom PCB are **software-controlled only during normal operation**. During reset:

1. **GPIO state reverts to high-impedance (Hi-Z)** - outputs disabled
2. **No software pull-up/pull-down resistors** are configured
3. **No state preservation** mechanism exists in software
4. **Actual behavior depends entirely on PCB hardware design** (external resistors)

**Next Steps**:

- Verify custom PCB schematic for external pull resistors on GPIO 9 and GPIO 10
- Test actual reset behavior with multimeter on eFuse enable pins
- Consider adding pull-up configuration in software if PCB supports it
- Document hardware requirements in board configuration files

---

**Analysis complete. Findings documented for main agent review.**
