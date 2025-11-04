# Codebase Analysis: Line Reversal in Thermal Printer

**Date:** 2025-11-04
**Topic:** Line reversal logic for 180° printer rotation
**Analyst:** Claude (Read-only exploration agent)

---

## Summary

The thermal printer in this system is physically mounted upside-down (180° rotation), which causes all output to appear inverted unless compensated for. The codebase implements line reversal at a **single critical point** in the printer driver to compensate for this hardware orientation. The reversal is **always active** and affects all printed content uniformly.

### Key Finding

**Line reversal happens in ONE location only:**

- `src/hardware/printer.cpp` lines 221-226 in `printWrappedInternal()`
- This is the ONLY place where line order is modified
- All content flows through this function, ensuring consistent behavior

---

## Relevant Files Identified

### Core Printer Files

- **`src/hardware/printer.cpp`** (408 lines) - Main printer driver implementation
  - Contains the ONLY line reversal logic in the entire codebase
  - Implements `printWrappedInternal()` which reverses line order
  - Handles word wrapping and text formatting

- **`src/hardware/printer.h`** (66 lines) - Printer manager interface
  - Defines `PrinterManager` class and global `printerManager` instance
  - Thread-safe with mutex protection

### Message Flow Files

- **`src/main.cpp`** (410 lines) - Application entry point
  - Loop checks `currentMessage.shouldPrintLocally` flag
  - Calls `printMessage()` when flag is set

- **`src/core/shared_types.h`** (59 lines) - Message structure definition

  ```cpp
  struct Message {
      String message;          // Content to print
      String timestamp;        // Timestamp for header
      bool shouldPrintLocally; // Queue flag
      bool hasMessage;         // Validity flag
  };
  extern Message currentMessage;
  ```

- **`src/core/mqtt_handler.cpp`** (935 lines) - MQTT message processing
  - Receives messages from MQTT broker
  - Calls `printerManager.printWithHeader()` directly (bypasses queue)

- **`src/content/content_handlers.cpp`** (425 lines) - Web API handlers
  - `/api/print-local` endpoint queues messages to `currentMessage`
  - Sets `shouldPrintLocally = true` to trigger printing in main loop

- **`src/utils/content_actions.cpp** - Content generation helpers
  - Formats content with headers (JOKE, RIDDLE, etc.)
  - Queues to `currentMessage` for local printing

### Supporting Files

- **`src/content/content_generators.cpp`** (438 lines) - Content generators
  - Generates jokes, riddles, quotes, etc.
  - Uses `reverseString()` for hidden answers (not related to line reversal)

- **`src/utils/api_client.cpp`** (323 lines) - HTTP client
  - Contains `reverseString()` utility function (line 316-322)
  - Used ONLY for reversing answer text in riddles/quizzes (not for line reversal)

---

## Implementation Details

### Printer Hardware Configuration

**Location:** `src/hardware/printer.cpp` lines 129-133

```cpp
// Enable 180° rotation (which also reverses the line order)
uart->write(0x1B);
uart->write('{');
uart->write(0x01); // ESC { 1
delay(50);
```

**Hardware Command:** ESC { 1 (0x1B 0x7B 0x01)

- Tells the printer to rotate output 180°
- This is a **printer firmware feature**, not software emulation
- Applied once during printer initialization

**Why This Exists:**

- The thermal printer is physically mounted upside-down in the enclosure
- Without rotation, all text would print upside-down and backwards
- The ESC { 1 command makes the printer compensate for this

---

### Line Reversal Logic

**Location:** `src/hardware/printer.cpp` lines 158-226

#### Function: `printWrappedInternal(const String& text)`

**Purpose:** Wraps long text to fit printer width (32 chars/line) and reverses line order to compensate for 180° printer rotation.

**Process:**

1. **Text Parsing** (lines 163-219): Split input into lines respecting newlines and word boundaries
2. **Line Reversal** (lines 221-226): Print lines in reverse order to compensate for rotation
3. **UART Output** (line 224): Send each reversed line to printer

**Critical Code:**

```cpp
// Print lines in reverse order to compensate for 180° printer rotation
for (int i = lines.size() - 1; i >= 0; i--)
{
    uart->println(lines[i]);
}
```

**Why Reversal is Needed:**

- The ESC { 1 command rotates the _printer output_ 180°
- This causes the printer to print from bottom-to-top of the paper
- **Without line reversal:** First line would appear at the bottom, last line at top
- **With line reversal:** Lines print bottom-to-top, but we send them top-to-bottom, resulting in correct order

**Detailed Word Wrapping Logic:**

- Splits text at newlines first (preserves intentional line breaks)
- Wraps long lines at word boundaries when possible
- Falls back to character-limit breaks if no spaces found
- Preserves empty lines for spacing

---

### Text Flow Through the System

#### Path 1: Local Web API → Printer

```
Web API (/api/print-local)
  ↓
content_handlers.cpp::handlePrintLocal()
  ↓
Set currentMessage.message = <content>
Set currentMessage.shouldPrintLocally = true
  ↓
main.cpp::loop() detects flag
  ↓
printer.cpp::printMessage()
  ↓
printerManager.printWithHeader(timestamp, message)
  ↓
printWrappedInternal(cleanBodyText)  [BODY PRINTED FIRST]
  ↓
printWrappedInternal(cleanHeaderText) [HEADER PRINTED LAST - appears at top after rotation]
  ↓
UART output with lines reversed
```

#### Path 2: MQTT → Printer (Direct)

```
MQTT message received
  ↓
mqtt_handler.cpp::onMessageReceived()
  ↓
handleMQTTMessageInternal()
  ↓
printerManager.printWithHeader(timestamp, printMessage)
  [BYPASSES currentMessage queue - prints immediately]
  ↓
printWrappedInternal(cleanBodyText)  [BODY PRINTED FIRST]
  ↓
printWrappedInternal(cleanHeaderText) [HEADER PRINTED LAST]
  ↓
UART output with lines reversed
```

#### Path 3: Hardware Buttons → Printer

```
Button press detected
  ↓
hardware_buttons.cpp::handleButtonAction()
  ↓
executeContentAction() generates content
  ↓
Set currentMessage.message = <content>
Set currentMessage.shouldPrintLocally = true
  ↓
[Same as Path 1 from here]
```

---

### Print Order Logic

**Location:** `src/hardware/printer.cpp` lines 232-276

#### Function: `printWithHeader(const String& headerText, const String& bodyText)`

**Print Sequence:**

1. **Body text first** (line 261): `printWrappedInternal(cleanBodyText)`
2. **Header text last** (lines 267-269):
   ```cpp
   setInverseInternal(true);
   printWrappedInternal(cleanHeaderText);
   setInverseInternal(false);
   ```

**Why This Order?**

- Printer outputs bottom-to-top due to 180° rotation
- Body printed first → appears at bottom of receipt
- Header printed last → appears at top of receipt (inverse video for emphasis)
- Both go through `printWrappedInternal()`, so both have lines reversed

**Visual Example:**

```
What gets sent to printer (reversed order):
Line 5 of body
Line 4 of body
Line 3 of body
Line 2 of body
Line 1 of body
Header line (inverse video)

What appears on paper (after 180° rotation):
Header line (inverse video)
Line 1 of body
Line 2 of body
Line 3 of body
Line 4 of body
Line 5 of body
```

---

## Code Flow Analysis

### Entry Points to Printing

1. **`printMessage()` - Free function** (printer.cpp:376-407)
   - Called from main loop when `currentMessage.shouldPrintLocally == true`
   - Reads `currentMessage.timestamp` and `currentMessage.message`
   - Calls `printerManager.printWithHeader(timestamp, message)`

2. **`printWithHeader()` - Direct call** (printer.cpp:232-277)
   - Public method of `PrinterManager`
   - Called directly by MQTT handler and hardware button handlers
   - Accepts timestamp (header) and message (body)
   - Handles cleaning, reversal, and formatting

3. **`printStartupMessage()` - Boot message** (printer.cpp:279-369)
   - Called once at startup
   - Prints device info and WiFi details
   - Uses same `printWrappedInternal()` for consistency

### Internal Print Chain

```
printWithHeader(header, body)
  ↓
printWrappedInternal(body)     ← LINE REVERSAL HAPPENS HERE
  ↓ (split into lines)
  ↓ (wrap at word boundaries)
  ↓ (iterate i = size-1 to 0)  ← REVERSAL LOOP
  ↓
uart->println(lines[i])
  ↓
printWrappedInternal(header)    ← LINE REVERSAL HAPPENS HERE
  ↓ (same process)
  ↓
advancePaperInternal(2)
```

---

## Key Observations

### 1. Single Point of Control

- **ALL printing goes through `printWrappedInternal()`**
- This is the ONLY place line reversal occurs
- Ensures consistent behavior across all content types

### 2. No Conditional Reversal

- Line reversal is **always active**
- No flags or settings to disable it
- The 180° rotation command (ESC { 1) is sent during initialization and stays active

### 3. Header/Body Print Order

- Body is printed FIRST, header LAST
- This seems backwards but makes sense with 180° rotation:
  - Last thing printed (header) appears at top of receipt
  - First thing printed (body) appears at bottom
- Both body and header have their internal lines reversed

### 4. Thread Safety

- `printWithHeader()` acquires mutex via `ManagerLock`
- Prevents concurrent printer access
- Safe for multi-core ESP32-S3

### 5. Character Mapping

- All text goes through `cleanString()` before printing
- Handles special characters that the thermal printer doesn't support
- Applied AFTER reversal (doesn't affect line order)

---

## Answer to Original Question

### Where does line reversal happen?

**One location only:**

- `src/hardware/printer.cpp` lines 221-226 in `printWrappedInternal()`

### Under what conditions does it apply?

**Always applies to ALL printed content:**

- No conditional logic
- No flags or settings
- No way to disable it
- Applies to:
  - Web API messages
  - MQTT messages
  - Hardware button triggers
  - Startup messages
  - All content types (jokes, riddles, quotes, news, etc.)

### Why might output be inconsistent?

**Based on this analysis, it SHOULD NOT be inconsistent:**

- Only one code path for printing
- Reversal always active
- No conditional logic

**If you're seeing inconsistent behavior, possible causes:**

1. **Different content types being tested** - But all go through same function
2. **MQTT vs Local printing** - Both use same `printWrappedInternal()`
3. **External factors:**
   - Paper feed direction physically changing
   - Printer hardware issue
   - Multiple printers with different configurations
4. **Testing artifacts:**
   - Reading receipts upside-down vs right-side-up
   - Comparing old receipts (before rotation fix) with new ones

---

## Architecture Strengths

1. **Single Responsibility:** One function handles all line ordering
2. **Fail-Fast:** No fallback or defensive logic
3. **Thread-Safe:** Mutex protection prevents corruption
4. **Consistent:** All content flows through same path
5. **Maintainable:** Easy to modify behavior in one place

---

## Additional Notes

### Related but NOT Line Reversal

- **`reverseString()` in api_client.cpp (line 316):** Reverses individual characters in a string
  - Used for hiding quiz/riddle answers (e.g., "Paris" → "siraP")
  - NOT related to line order reversal
  - Does NOT affect printed output order

### Printer Commands Used

- **ESC @** (0x1B 0x40): Reset printer
- **ESC 7** (0x1B 0x37): Set heating parameters
- **ESC {** (0x1B 0x7B): Enable 180° rotation
- **GS B** (0x1D 0x42): Enable/disable inverse printing (white text on black)

### Word Wrapping Algorithm

1. Find newline or end of string
2. Extract segment
3. If segment > 32 chars:
   - Find last space before char 32
   - Break at space (or char 32 if no space)
   - Trim leading spaces from next line
4. Repeat until all text processed
5. Reverse array of resulting lines

---

## File Relationships Map

```
┌─────────────────────────────────────────────────────────┐
│                    main.cpp::loop()                     │
│  Checks: currentMessage.shouldPrintLocally == true      │
└────────────────────┬────────────────────────────────────┘
                     │
                     ↓
        ┌────────────────────────────┐
        │  printer.cpp::printMessage()│
        │  Reads currentMessage       │
        └────────────┬───────────────┘
                     │
                     ↓
        ┌────────────────────────────────────┐
        │ PrinterManager::printWithHeader()  │
        │ Thread-safe (mutex)                │
        └────────────┬───────────────────────┘
                     │
            ┌────────┴────────┐
            ↓                 ↓
    ┌───────────────┐  ┌──────────────┐
    │ Body Text     │  │ Header Text  │
    │ (normal)      │  │ (inverse)    │
    └───────┬───────┘  └──────┬───────┘
            │                 │
            └────────┬────────┘
                     ↓
        ┌─────────────────────────────┐
        │ printWrappedInternal()      │
        │ ★ LINE REVERSAL HERE ★      │
        │ for (i = size-1; i >= 0)    │
        └────────────┬────────────────┘
                     ↓
        ┌─────────────────────────────┐
        │ uart->println(lines[i])     │
        │ Send to thermal printer     │
        └─────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│            Alternative Entry Points:                    │
├─────────────────────────────────────────────────────────┤
│ • mqtt_handler.cpp → printWithHeader() [direct]         │
│ • hardware_buttons.cpp → currentMessage → printMessage()│
│ • content_handlers.cpp → currentMessage → printMessage()│
└─────────────────────────────────────────────────────────┘
```

---

## Conclusion

The line reversal system is **simple, consistent, and centralized**. All printed content goes through a single function (`printWrappedInternal()`) that always reverses line order to compensate for the 180° printer rotation. There are no conditional paths or settings that would cause inconsistent behavior.

**If you're experiencing "sometimes first-first, sometimes last-first" behavior:**

1. Check if you're comparing MQTT messages with local messages (both should be identical)
2. Verify printer hardware orientation hasn't physically changed
3. Check if old receipts (pre-rotation-fix) are being compared with new ones
4. Look for external factors (paper feed direction, multiple printers)

The code itself has **one print path, one reversal implementation, always active**.

---

**Analysis complete. Findings documented in `/Users/adamknowles/dev/Scribe Evolution/VS Code Workspace/Git repo/scribe/notes/2025-11-04_analysis_line_reversal_printing.md` for main agent review.**
