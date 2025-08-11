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

// Global button state array
static ButtonState buttonStates[4]; // Exactly 4 buttons

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
 * @brief Trigger an endpoint from hardware button press
 */
void triggerEndpointFromButton(const char *endpoint);

/**
 * @brief Get button configuration info as JSON string
 */
String getButtonConfigJson();

#endif
