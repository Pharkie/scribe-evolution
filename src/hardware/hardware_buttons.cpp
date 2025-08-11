#include "hardware_buttons.h"
#include "../web/web_server.h"
#include "printer.h"
#include "../content/content_handlers.h"
#include "../core/config_loader.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

// ========================================
// HARDWARE BUTTON IMPLEMENTATION
// ========================================

void initializeHardwareButtons()
{
    LOG_VERBOSE("BUTTONS", "Initializing %d hardware buttons", numHardwareButtons);

    for (int i = 0; i < numHardwareButtons; i++)
    {
        // Configure GPIO pin using buttonGPIOs array
        pinMode(buttonGPIOs[i], buttonActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);

        // Initialize button state
        buttonStates[i].currentState = digitalRead(buttonGPIOs[i]);
        buttonStates[i].lastState = buttonStates[i].currentState;
        buttonStates[i].pressed = false;
        buttonStates[i].longPressTriggered = false;
        buttonStates[i].lastDebounceTime = 0;
        buttonStates[i].pressStartTime = 0;
        buttonStates[i].lastPressTime = 0;
        buttonStates[i].pressCount = 0;
        buttonStates[i].windowStartTime = 0;

        // Get runtime configuration for logging
        const RuntimeConfig& config = getRuntimeConfig();
        LOG_VERBOSE("BUTTONS", "Button %d: GPIO %d -> %s (short), %s (long)",
                    i, buttonGPIOs[i], config.buttonShortActions[i].c_str(), config.buttonLongActions[i].c_str());
    }

    // Feed watchdog after hardware button initialization
    esp_task_wdt_reset();

    LOG_NOTICE("BUTTONS", "Hardware buttons initialized");
}

void checkHardwareButtons()
{
    unsigned long currentTime = millis();

    for (int i = 0; i < numHardwareButtons; i++)
    {
        // Read current state using buttonGPIOs array
        bool reading = digitalRead(buttonGPIOs[i]);

        // Check if state changed (for debouncing)
        if (reading != buttonStates[i].lastState)
        {
            buttonStates[i].lastDebounceTime = currentTime;
        }

        // If enough time has passed since last change, update current state
        if ((currentTime - buttonStates[i].lastDebounceTime) > buttonDebounceMs)
        {
            // If state has changed from last stable state
            if (reading != buttonStates[i].currentState)
            {
                buttonStates[i].currentState = reading;

                // Detect press (depends on buttonActiveLow setting)
                bool isPressed = buttonActiveLow ? (reading == LOW) : (reading == HIGH);

                if (isPressed && !buttonStates[i].pressed)
                {
                    // Button pressed
                    buttonStates[i].pressed = true;
                    buttonStates[i].longPressTriggered = false;
                    buttonStates[i].pressStartTime = currentTime;

                    // Get runtime configuration for logging
                    const RuntimeConfig& config = getRuntimeConfig();
                    LOG_VERBOSE("BUTTONS", "Button %d pressed: %s", i, config.buttonShortActions[i].c_str());
                }
                else if (!isPressed && buttonStates[i].pressed)
                {
                    // Button released
                    buttonStates[i].pressed = false;
                    unsigned long pressDuration = currentTime - buttonStates[i].pressStartTime;

                    // Only trigger short press if long press wasn't already triggered
                    if (!buttonStates[i].longPressTriggered)
                    {
                        if (pressDuration < buttonLongPressMs)
                        {
                            // Get runtime configuration for logging
                            const RuntimeConfig& config = getRuntimeConfig();
                            LOG_NOTICE("BUTTONS", "Button %d short press: %s", i, config.buttonShortActions[i].c_str());
                            handleButtonPress(i);
                        }
                    }

                    LOG_VERBOSE("BUTTONS", "Button %d released after %lu ms", i, pressDuration);
                }
            }
        }

        // Check for long press while button is held down
        if (buttonStates[i].pressed && !buttonStates[i].longPressTriggered)
        {
            unsigned long pressDuration = currentTime - buttonStates[i].pressStartTime;
            if (pressDuration >= buttonLongPressMs)
            {
                buttonStates[i].longPressTriggered = true;
                // Get runtime configuration for logging
                const RuntimeConfig& config = getRuntimeConfig();
                LOG_NOTICE("BUTTONS", "Button %d long press: %s", i, config.buttonLongActions[i].c_str());
                handleButtonLongPress(i);
            }
        }

        buttonStates[i].lastState = reading;
    }
}

// Rate limiting check for hardware buttons
bool isButtonRateLimited(int buttonIndex, unsigned long currentTime)
{
    ButtonState &state = buttonStates[buttonIndex];

    // Check minimum interval since last press
    if ((currentTime - state.lastPressTime) < buttonMinInterval)
    {
        LOG_WARNING("BUTTONS", "Button %d rate limited: too soon (last press %lu ms ago)",
                    buttonIndex, currentTime - state.lastPressTime);
        return true;
    }

    // Reset window if it's been too long since window started
    if ((currentTime - state.windowStartTime) >= buttonRateLimitWindow)
    {
        state.windowStartTime = currentTime;
        state.pressCount = 0;
    }

    // Check if we've exceeded max presses per window
    if (state.pressCount >= buttonMaxPerMinute)
    {
        LOG_WARNING("BUTTONS", "Button %d rate limited: max presses reached (%d/%d in current window)",
                    buttonIndex, state.pressCount, buttonMaxPerMinute);
        return true;
    }

    // Update rate limiting state
    state.lastPressTime = currentTime;
    state.pressCount++;

    LOG_VERBOSE("BUTTONS", "Button %d rate check passed: %d/%d presses in window",
                buttonIndex, state.pressCount, buttonMaxPerMinute);
    return false;
}

void handleButtonPress(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= numHardwareButtons)
    {
        LOG_ERROR("BUTTONS", "Invalid button index: %d", buttonIndex);
        return;
    }

    // Check rate limiting
    unsigned long currentTime = millis();
    if (isButtonRateLimited(buttonIndex, currentTime))
    {
        return; // Rate limited, ignore this press
    }

    // Get runtime configuration
    const RuntimeConfig& config = getRuntimeConfig();
    const String& shortAction = config.buttonShortActions[buttonIndex];

    LOG_VERBOSE("BUTTONS", "Triggering short press endpoint: %s", shortAction.c_str());
    triggerEndpointFromButton(shortAction.c_str());
}

void handleButtonLongPress(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= numHardwareButtons)
    {
        LOG_ERROR("BUTTONS", "Invalid button index: %d", buttonIndex);
        return;
    }

    // Check rate limiting
    unsigned long currentTime = millis();
    if (isButtonRateLimited(buttonIndex, currentTime))
    {
        return; // Rate limited, ignore this press
    }

    // Get runtime configuration
    const RuntimeConfig& config = getRuntimeConfig();
    const String& longAction = config.buttonLongActions[buttonIndex];

    LOG_VERBOSE("BUTTONS", "Triggering long press endpoint: %s", longAction.c_str());
    triggerEndpointFromButton(longAction.c_str());
}

void triggerEndpointFromButton(const char *endpoint)
{
    // Map endpoint strings to handler functions
    LOG_VERBOSE("BUTTONS", "Hardware button triggered: %s", endpoint);

    if (strcmp(endpoint, "/riddle") == 0)
    {
        handleRiddle();
    }
    else if (strcmp(endpoint, "/joke") == 0)
    {
        handleJoke();
    }
    else if (strcmp(endpoint, "/quote") == 0)
    {
        handleQuote();
    }
    else if (strcmp(endpoint, "/quiz") == 0)
    {
        handleQuiz();
    }
    else if (strcmp(endpoint, "/print-test") == 0)
    {
        handlePrintTest();
    }
    else if (strcmp(endpoint, "/test-print") == 0)
    {
        handlePrintTest(); // Same handler for both endpoint variations
    }
    else if (strcmp(endpoint, "/unbidden-ink") == 0)
    {
        handleUnbiddenInk();
    }
    else if (strcmp(endpoint, "/keep-going") == 0)
    {
        // Keep-going endpoint - generate random content
        // For now, let's default to joke
        handleJoke();
    }
    else if (strlen(endpoint) == 0)
    {
        // Empty endpoint - do nothing
        LOG_VERBOSE("BUTTONS", "Empty endpoint - no action");
    }
    else
    {
        LOG_ERROR("BUTTONS", "Unknown endpoint: %s", endpoint);
    }
}

String getButtonConfigJson()
{
    DynamicJsonDocument doc(jsonDocumentSize);
    JsonArray buttons = doc.createNestedArray("buttons");
    
    // Get runtime configuration
    const RuntimeConfig& config = getRuntimeConfig();

    for (int i = 0; i < numHardwareButtons; i++)
    {
        JsonObject button = buttons.createNestedObject();
        button["index"] = i;
        button["gpio"] = buttonGPIOs[i];
        button["short_endpoint"] = config.buttonShortActions[i];
        button["long_endpoint"] = config.buttonLongActions[i];

        button["currentState"] = buttonStates[i].currentState;
        button["pressed"] = buttonStates[i].pressed;
        button["longPressTriggered"] = buttonStates[i].longPressTriggered;
    }

    doc["num_buttons"] = numHardwareButtons;
    doc["active_low"] = buttonActiveLow;
    doc["debounce_ms"] = buttonDebounceMs;
    doc["long_press_ms"] = buttonLongPressMs;
    doc["min_interval_ms"] = buttonMinInterval;
    doc["max_per_minute"] = buttonMaxPerMinute;

    String result;
    serializeJson(doc, result);
    return result;
}
