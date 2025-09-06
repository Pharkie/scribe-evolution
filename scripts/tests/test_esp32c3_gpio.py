#!/usr/bin/env python3
"""
ESP32-C3 GPIO Hardware Test Micropython Script
Tests button GPIOs, LED strip, status LED, and UART to isolate hardware vs software issues.

Based on current config.h settings:
- Button GPIOs: 5, 6, 7, 9 (INPUT_PULLUP, active LOW)
- Status LED: GPIO 8 (built-in LED)
- LED Strip: GPIO 20 (data pin)
- UART TX: GPIO 21 (printer communication)

Usage:
1. Connect ESP32-C3 with MicroPython firmware via USB
2. Upload this script to the ESP32 (e.g., using Thonny, ampy, rshell, or WebREPL)
3. Run the script on the ESP32 using the MicroPython REPL or your chosen tool
4. Follow interactive prompts in the serial console
5. Press buttons and observe results
"""

import time
import sys
import threading
import signal
from datetime import datetime

# Check if running on ESP32 or computer
try:
    import machine
    import utime
    from machine import Pin, UART

    ON_ESP32 = True
    print("Running on ESP32 MicroPython")
except ImportError:
    ON_ESP32 = False
    print("Running on computer - will connect to ESP32 via serial")
    try:
        import serial
        import serial.tools.list_ports

        print("✅ Serial communication libraries available")
    except ImportError:
        print("ERROR: Need pyserial for computer-based testing")
        print("Install with: pip3 install pyserial")
        sys.exit(1)

# GPIO Configuration from config.h
BUTTON_GPIOS = [5, 6, 7, 9]  # Button GPIOs (INPUT_PULLUP, active LOW)
BUTTON_NAMES = ["JOKE", "RIDDLE", "QUOTE", "QUIZ"]
STATUS_LED_GPIO = 8  # Built-in LED on ESP32-C3
LED_STRIP_GPIO = 20  # LED strip data pin
UART_TX_GPIO = 21  # Printer UART TX
UART_RX_GPIO = -1  # Not used (one-way communication)

# Test configuration
DEBOUNCE_MS = 50
POLL_INTERVAL_MS = 10
TEST_DURATION_SECONDS = 60

# Global variables
running = True
button_states = {}
button_last_change = {}
button_press_count = {}


def signal_handler(sig, frame):
    """Handle Ctrl+C gracefully"""
    global running
    print("\n\nStopping GPIO test...")
    running = False


def log_message(level, message):
    """Log message with timestamp"""
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print(f"[{timestamp}] [{level}] {message}")


def test_esp32_gpio():
    """Main GPIO test function for ESP32 MicroPython"""
    log_message("INFO", "=== ESP32-C3 GPIO Hardware Test ===")
    log_message("INFO", f"Button GPIOs: {BUTTON_GPIOS}")
    log_message("INFO", f"Status LED: GPIO {STATUS_LED_GPIO}")
    log_message("INFO", f"LED Strip: GPIO {LED_STRIP_GPIO}")
    log_message("INFO", f"UART TX: GPIO {UART_TX_GPIO}")

    # Initialize GPIOs
    buttons = []
    for i, gpio in enumerate(BUTTON_GPIOS):
        try:
            # INPUT_PULLUP equivalent: Pin.IN with Pin.PULL_UP
            pin = Pin(gpio, Pin.IN, Pin.PULL_UP)
            buttons.append(pin)
            button_states[i] = pin.value()
            button_last_change[i] = utime.ticks_ms()
            button_press_count[i] = 0
            log_message(
                "INFO", f"Button {i} ({BUTTON_NAMES[i]}) initialized on GPIO {gpio}"
            )
        except Exception as e:
            log_message("ERROR", f"Failed to initialize button GPIO {gpio}: {e}")
            return False

    # Initialize status LED
    try:
        status_led = Pin(STATUS_LED_GPIO, Pin.OUT)
        status_led.off()  # Start with LED off
        log_message("INFO", f"Status LED initialized on GPIO {STATUS_LED_GPIO}")
    except Exception as e:
        log_message(
            "ERROR", f"Failed to initialize status LED GPIO {STATUS_LED_GPIO}: {e}"
        )
        status_led = None

    # Initialize LED strip pin (just as output, no data)
    try:
        led_strip = Pin(LED_STRIP_GPIO, Pin.OUT)
        led_strip.off()
        log_message("INFO", f"LED strip pin initialized on GPIO {LED_STRIP_GPIO}")
    except Exception as e:
        log_message(
            "ERROR", f"Failed to initialize LED strip GPIO {LED_STRIP_GPIO}: {e}"
        )
        led_strip = None

    # Initialize UART (test communication ability)
    try:
        uart = UART(0, baudrate=115200, tx=UART_TX_GPIO, rx=UART_RX_GPIO)
        log_message("INFO", f"UART initialized on TX {UART_TX_GPIO}")
    except Exception as e:
        log_message("ERROR", f"Failed to initialize UART: {e}")
        uart = None

    log_message("INFO", "=== GPIO Test Started ===")
    log_message("INFO", "Press buttons to test. Monitoring for crashes...")
    log_message("INFO", f"Test will run for {TEST_DURATION_SECONDS} seconds")

    start_time = utime.ticks_ms()
    led_blink_time = utime.ticks_ms()
    uart_test_time = utime.ticks_ms()
    status_report_time = utime.ticks_ms()

    test_count = 0

    try:
        while utime.ticks_diff(utime.ticks_ms(), start_time) < (
            TEST_DURATION_SECONDS * 1000
        ):
            current_time = utime.ticks_ms()
            test_count += 1

            # Check all buttons
            for i, button in enumerate(buttons):
                try:
                    current_state = button.value()

                    # Detect state change
                    if current_state != button_states[i]:
                        # Debounce check
                        if (
                            utime.ticks_diff(current_time, button_last_change[i])
                            > DEBOUNCE_MS
                        ):
                            old_state = button_states[i]
                            button_states[i] = current_state
                            button_last_change[i] = current_time

                            # Log state change (active LOW, so 0 = pressed)
                            if current_state == 0 and old_state == 1:
                                button_press_count[i] += 1
                                log_message(
                                    "PRESS",
                                    f"*** BUTTON {i} ({BUTTON_NAMES[i]}) PRESSED *** GPIO {BUTTON_GPIOS[i]} -> LOW (Count: {button_press_count[i]})",
                                )

                                # Flash LED strip on button press
                                if led_strip:
                                    led_strip.on()
                                    utime.sleep_ms(50)
                                    led_strip.off()

                            elif current_state == 1 and old_state == 0:
                                log_message(
                                    "RELEASE",
                                    f"Button {i} ({BUTTON_NAMES[i]}) RELEASED - GPIO {BUTTON_GPIOS[i]} -> HIGH",
                                )

                except Exception as e:
                    log_message(
                        "ERROR", f"Error reading button {i} GPIO {BUTTON_GPIOS[i]}: {e}"
                    )

            # Blink status LED every 500ms to show we're alive
            if status_led and utime.ticks_diff(current_time, led_blink_time) > 500:
                status_led.on()
                utime.sleep_ms(10)
                status_led.off()
                led_blink_time = current_time

            # Test UART every 5 seconds
            if uart and utime.ticks_diff(current_time, uart_test_time) > 5000:
                try:
                    test_message = f"GPIO Test Alive - Cycle {test_count}\n"
                    uart.write(test_message.encode())
                    log_message(
                        "UART", f"UART test message sent: {test_message.strip()}"
                    )
                    uart_test_time = current_time
                except Exception as e:
                    log_message("ERROR", f"UART test failed: {e}")

            # Status report every 10 seconds
            if utime.ticks_diff(current_time, status_report_time) > 10000:
                total_presses = sum(button_press_count.values())
                log_message(
                    "STATUS",
                    f"Test cycles: {test_count}, Total button presses: {total_presses}",
                )
                for i in range(len(BUTTON_GPIOS)):
                    log_message(
                        "STATUS",
                        f"  Button {i} ({BUTTON_NAMES[i]}): {button_press_count[i]} presses",
                    )
                status_report_time = current_time

            # Small delay to prevent overwhelming
            utime.sleep_ms(POLL_INTERVAL_MS)

    except KeyboardInterrupt:
        log_message("INFO", "Test interrupted by user")
    except Exception as e:
        log_message("ERROR", f"Unexpected error during test: {e}")

    # Final report
    log_message("INFO", "=== GPIO Test Complete ===")
    log_message("INFO", f"Total test cycles: {test_count}")
    total_presses = sum(button_press_count.values())
    log_message("INFO", f"Total button presses: {total_presses}")

    for i in range(len(BUTTON_GPIOS)):
        log_message(
            "INFO",
            f"Button {i} ({BUTTON_NAMES[i]}) on GPIO {BUTTON_GPIOS[i]}: {button_press_count[i]} presses",
        )

    if total_presses > 0:
        log_message(
            "SUCCESS", "✅ Button press detection working - no crashes detected!"
        )
        log_message(
            "INFO",
            "This suggests the crash issue is in the ESP32 firmware/software, not hardware",
        )
    else:
        log_message(
            "WARNING",
            "⚠️  No button presses detected - check wiring or press buttons during test",
        )

