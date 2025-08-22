#include "hardware_buttons.h"
#include "../web/web_server.h"
#include "printer.h"
#include "../content/content_handlers.h"
#include "../content/content_generators.h"
#include "../core/config_loader.h"
#include "../utils/time_utils.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <PubSubClient.h>

// External declarations
extern PubSubClient mqttClient;
extern Message currentMessage;

// ========================================
// HARDWARE BUTTON IMPLEMENTATION
// ========================================

void initializeHardwareButtons()
{
    LOG_VERBOSE("BUTTONS", "Initializing %d hardware buttons", numHardwareButtons);

    for (int i = 0; i < numHardwareButtons; i++)
    {
        // Configure GPIO pin using defaultButtons array
        pinMode(defaultButtons[i].gpio, buttonActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);

        // Initialize button state
        buttonStates[i].currentState = digitalRead(defaultButtons[i].gpio);
        buttonStates[i].lastState = buttonStates[i].currentState;
        buttonStates[i].pressed = false;
        buttonStates[i].longPressTriggered = false;
        buttonStates[i].lastDebounceTime = 0;
        buttonStates[i].pressStartTime = 0;
        buttonStates[i].lastPressTime = 0;
        buttonStates[i].pressCount = 0;
        buttonStates[i].windowStartTime = 0;

        // Get runtime configuration for logging
        const RuntimeConfig &config = getRuntimeConfig();
        LOG_VERBOSE("BUTTONS", "Button %d: GPIO %d -> %s (short), %s (long)",
                    i, defaultButtons[i].gpio, config.buttonShortActions[i].c_str(), config.buttonLongActions[i].c_str());
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
        // Read current state using defaultButtons array
        bool reading = digitalRead(defaultButtons[i].gpio);

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
                    const RuntimeConfig &config = getRuntimeConfig();
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
                            const RuntimeConfig &config = getRuntimeConfig();
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
                const RuntimeConfig &config = getRuntimeConfig();
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
    const RuntimeConfig &config = getRuntimeConfig();
    const String &shortAction = config.buttonShortActions[buttonIndex];
    const String &shortMqttTopic = config.buttonShortMqttTopics[buttonIndex];

    LOG_VERBOSE("BUTTONS", "Triggering short press endpoint: %s", shortAction.c_str());

    // Generate content and print locally - same as web endpoints but direct
    handleButtonAction(shortAction.c_str(), shortMqttTopic.c_str());
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
    const RuntimeConfig &config = getRuntimeConfig();
    const String &longAction = config.buttonLongActions[buttonIndex];
    const String &longMqttTopic = config.buttonLongMqttTopics[buttonIndex];

    LOG_VERBOSE("BUTTONS", "Triggering long press endpoint: %s", longAction.c_str());

    // Generate content and print locally - same as web endpoints but direct
    handleButtonAction(longAction.c_str(), longMqttTopic.c_str());
}

// Handle button action - generate content and print locally
void handleButtonAction(const char *endpoint, const char *mqttTopic)
{
    if (strlen(endpoint) == 0)
    {
        LOG_VERBOSE("BUTTONS", "Empty endpoint - no action");
        return;
    }

    LOG_VERBOSE("BUTTONS", "Hardware button action: %s", endpoint);

    String content;
    String actionHeader;

    // Generate content directly using the same generators as web endpoints
    if (strcmp(endpoint, "/api/riddle") == 0)
    {
        content = generateRiddleContent();
        actionHeader = "RIDDLE";
    }
    else if (strcmp(endpoint, "/api/joke") == 0)
    {
        content = generateJokeContent();
        actionHeader = "JOKE";
    }
    else if (strcmp(endpoint, "/api/quote") == 0)
    {
        content = generateQuoteContent();
        actionHeader = "QUOTE";
    }
    else if (strcmp(endpoint, "/api/quiz") == 0)
    {
        content = generateQuizContent();
        actionHeader = "QUIZ";
    }
    else if (strcmp(endpoint, "/api/news") == 0)
    {
        content = generateNewsContent();
        actionHeader = "NEWS";
    }
    else if (strcmp(endpoint, "/api/character-test") == 0 || strcmp(endpoint, "/api/test-print") == 0)
    {
        content = loadPrintTestContent();
        actionHeader = "TEST PRINT";
    }
    else if (strcmp(endpoint, "/api/unbidden-ink") == 0)
    {
        content = generateUnbiddenInkContent();
        actionHeader = "UNBIDDEN INK";
    }
    else
    {
        LOG_ERROR("BUTTONS", "Unknown endpoint: %s", endpoint);
        return;
    }

    // If content generation failed, log and return
    if (content.length() == 0)
    {
        LOG_ERROR("BUTTONS", "Failed to generate content for: %s", endpoint);
        return;
    }

    // Format with header and send to local printer (same as /api/print-local)
    String formattedContent = formatContentWithHeader(actionHeader, content, "");
    String timestamp = getFormattedDateTime();

    // Print directly to local printer
    printWithHeader(timestamp, formattedContent);

    LOG_NOTICE("BUTTONS", "Button action completed: %s", endpoint);

    // Handle MQTT if topic is specified (future enhancement)
    if (strlen(mqttTopic) > 0)
    {
        LOG_WARNING("BUTTONS", "MQTT functionality not yet implemented for buttons: %s", mqttTopic);
    }
}
