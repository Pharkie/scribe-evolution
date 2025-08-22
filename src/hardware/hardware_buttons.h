#ifndef HARDWARE_BUTTONS_H
#define HARDWARE_BUTTONS_H

#include "../core/config.h"
#include "../core/logging.h"
#include <Arduino.h>

// ========================================
// HARDWARE BUTTON MANAGEMENT
// ========================================

// Button action queue structure
struct ButtonAction
{
    String endpoint;           // API endpoint to call
    String mqttTopic;         // MQTT topic (future enhancement)
    unsigned long timestamp;  // When action was queued
    int buttonIndex;          // Which button triggered this action
    bool isLongPress;         // Short or long press
};

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
 * @brief Initialize button action queue system
 */
void initializeButtonActionQueue();

/**
 * @brief Process queued button actions (non-blocking)
 * Call this regularly from main loop
 */
void processButtonActionQueue();

/**
 * @brief Queue a button action for async processing
 * @param endpoint The API endpoint path to request
 * @param mqttTopic The MQTT topic to send to (empty string for local print only)
 * @param buttonIndex Which button triggered this action
 * @param isLongPress Whether this was a long press
 */
void queueButtonAction(const String &endpoint, const String &mqttTopic, int buttonIndex, bool isLongPress);

/**
 * @brief Trigger LED effect for button press (immediate, non-blocking)
 * @param buttonIndex Which button was pressed
 * @param isLongPress Whether this was a long press
 */
void triggerButtonLedEffect(int buttonIndex, bool isLongPress);

/**
 * @brief Make async local HTTP request to web endpoint (DEPRECATED - use queue system)
 * @param endpoint The API endpoint path to request
 */
void makeAsyncLocalRequest(const char *endpoint);

#endif
