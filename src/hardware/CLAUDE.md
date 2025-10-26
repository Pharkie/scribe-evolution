# Hardware - CLAUDE.md

<system_context>
Hardware interfaces: thermal printer and physical buttons.
Direct GPIO control with safety validation.
Printer supports bidirectional communication (RX) and hardware flow control (DTR) on custom PCB.
</system_context>

<critical_notes>

- ALWAYS validate GPIO pins with isValidGPIO() and isSafeGPIO()
- Use debouncing for all button inputs
- Check printer buffer availability before sending
- ESP32-C3 has limited safe GPIO pins
- Custom PCB has eFuse power control - enabled in `printer.cpp` before UART init
- RX/DTR pins configurable: -1 = disabled (mini boards), GPIO = enabled (custom PCB)
  </critical_notes>

<paved_path>
Button Pattern:

1. Configure GPIO as input with pullup
2. Implement debouncing with millis() timing
3. Handle press events with rate limiting

Printer Pattern:

1. Enable eFuse (custom PCB only) before UART init
2. Initialize UART with TX (always), RX (optional), DTR (optional)
3. Configure DTR pin HIGH if enabled (ready to receive)
4. Check buffer space before writing
5. Use character mapping for special characters
   </paved_path>

<patterns>
// GPIO validation (CRITICAL)
if (!isValidGPIO(pin) || !isSafeGPIO(pin)) {
    logger.log("Invalid GPIO pin: " + String(pin));
    return false;
}

// Button debouncing
static unsigned long lastPress = 0;
const unsigned long DEBOUNCE_MS = 250;

if (digitalRead(BUTTON_PIN) == LOW &&
millis() - lastPress > DEBOUNCE_MS) {
handleButtonPress();
lastPress = millis();
}

// Printer buffer check
if (printer.availableForWrite() < message.length()) {
delay(100); // Wait for buffer space
return false;
}

// Safe printing with bidirectional communication
bool printMessage(const String& content) {
if (!printer.connected()) return false;

    String mapped = mapSpecialCharacters(content);
    printer.print(mapped);
    return true;

}

// eFuse power control (custom PCB only)
#if BOARD_HAS_EFUSES
pinMode(BOARD_EFUSE_PRINTER_PIN, OUTPUT);
digitalWrite(BOARD_EFUSE_PRINTER_PIN, HIGH);
LOG_VERBOSE("PRINTER", "Printer eFuse enabled (GPIO %d)", BOARD_EFUSE_PRINTER_PIN);
#endif

// UART initialization with optional RX/DTR
uart->begin(9600, SERIAL_8N1, config.printerRxPin, config.printerTxPin);

// DTR hardware flow control (optional)
if (config.printerDtrPin != -1) {
pinMode(config.printerDtrPin, OUTPUT);
digitalWrite(config.printerDtrPin, HIGH); // DTR active = ready to receive
LOG_VERBOSE("PRINTER", "DTR enabled on GPIO %d", config.printerDtrPin);
}

// Reading printer status (bidirectional communication)
// The CSN-A4L supports realtime status queries
// NOTE: Requires RX pin configured (printerRxPin != -1)
bool checkPrinterStatus() {
if (config.printerRxPin == -1) {
return false; // RX not configured
}

    // Request realtime status (DLE EOT n)
    printer.write(0x10); // DLE
    printer.write(0x04); // EOT
    printer.write(0x01); // Status type 1 (printer status)

    // Wait for response
    unsigned long timeout = millis() + 100;
    while (!printer.available() && millis() < timeout) {
        delay(1);
    }

    if (printer.available()) {
        uint8_t status = printer.read();
        // Bit 3: Paper end sensor status
        // Bit 5: Overheating
        return (status & 0x28) == 0;  // No paper end, no overheating
    }

    return false;  // Timeout or no response

}
</patterns>

<common_tasks>
Adding new button:

1. Validate GPIO pin is safe for ESP32-C3
2. Configure as INPUT_PULLUP
3. Add debouncing logic
4. Implement action handler
5. Test press/release behavior
   </common_tasks>

<fatal_implications>

- Skip GPIO validation = Hardware damage (magic smoke)
- No debouncing = Ghost presses and spam
- No buffer checks = Lost data/corruption
- Wrong UART config = Communication failure
  </fatal_implications>
