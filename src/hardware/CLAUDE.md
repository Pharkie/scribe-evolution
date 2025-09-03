# Hardware - CLAUDE.md

<system_context>
Hardware interfaces: thermal printer and physical buttons.
Direct GPIO control with safety validation.
</system_context>

<critical_notes>

- ALWAYS validate GPIO pins with isValidGPIO() and isSafeGPIO()
- Use debouncing for all button inputs
- Check printer buffer availability before sending
- ESP32-C3 has limited safe GPIO pins
  </critical_notes>

<paved_path>
Button Pattern:

1. Configure GPIO as input with pullup
2. Implement debouncing with millis() timing
3. Handle press events with rate limiting

Printer Pattern:

1. Initialize UART with correct baud rate
2. Check buffer space before writing
3. Use character mapping for special characters
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

// Safe printing
bool printMessage(const String& content) {
if (!printer.connected()) return false;

    String mapped = mapSpecialCharacters(content);
    printer.print(mapped);
    return true;

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
