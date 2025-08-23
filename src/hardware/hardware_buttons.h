#ifndef HARDWARE_BUTTONS_H
#define HARDWARE_BUTTONS_H

#include "../core/config.h"
#include "../core/logging.h"
#include <Arduino.h>

// ========================================
// HARDWARE BUTTON MANAGEMENT
// ========================================

// Button state tracking
struct ButtonState
{
    bool currentState;              // Current GPIO reading
    bool lastState;                 // Previous GPIO reading
    bool pressed;                   // Debounced press detected
    bool longPressTriggered;        // Long press action already triggered
    unsigned long lastDebounceTime; // Last time the output pin toggled
    unsigned long pressStartTime;   // When press started (for long press)
    unsigned long lastPressTime;    // Last successful press time (for rate limiting)
    unsigned int pressCount;        // Presses within current rate limit window
    unsigned long windowStartTime;  // Start of current rate limit window
};

// Global button state array - size matches numHardwareButtons from config.h
extern ButtonState buttonStates[];

/**
 * @brief Initialize hardware buttons
 * Sets up GPIO pins and initial state
 */
void initializeHardwareButtons();

void checkHardwareButtons();

/**
 * @brief Check if a button press should be rate limited
 */
bool isButtonRateLimited(int buttonIndex, unsigned long currentTime);

void handleButtonPress(int buttonIndex);
void handleButtonLongPress(int buttonIndex);

/**
 * @brief Trigger LED effect for button press (immediate, non-blocking)
 * @param buttonIndex Which button was pressed
 * @param isLongPress Whether this was a long press
 */
void triggerButtonLedEffect(int buttonIndex, bool isLongPress);

/**
 * @brief Execute button endpoint directly without HTTP calls (DEPRECATED)
 * @param endpoint The API endpoint path to execute
 * @deprecated This function uses blocking HTTP operations - DO NOT USE
 * @warning Use direct button actions (JOKE, RIDDLE, etc.) instead of HTTP endpoints
 */
void executeButtonEndpoint(const char *endpoint) __attribute__((deprecated));

#endif
