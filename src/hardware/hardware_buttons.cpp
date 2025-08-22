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
#include <HTTPClient.h>
#include <WiFi.h>

// External declarations
extern PubSubClient mqttClient;
extern Message currentMessage;

// ========================================
// HARDWARE BUTTON IMPLEMENTATION
// ========================================

void initializeHardwareButtons()
{
    LOG_NOTICE("BUTTONS", "=== INITIALIZING HARDWARE BUTTONS ===");
    LOG_VERBOSE("BUTTONS", "Button count: %d", numHardwareButtons);
    LOG_VERBOSE("BUTTONS", "Button debounce: %lu ms", buttonDebounceMs);
    LOG_VERBOSE("BUTTONS", "Button long press: %lu ms", buttonLongPressMs);
    LOG_VERBOSE("BUTTONS", "Button active low: %s", buttonActiveLow ? "true" : "false");

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
        LOG_NOTICE("BUTTONS", "Button %d: GPIO %d -> Short: '%s', Long: '%s'",
                   i, defaultButtons[i].gpio,
                   config.buttonShortActions[i].c_str(),
                   config.buttonLongActions[i].c_str());
    }

    // Feed watchdog after hardware button initialization
    esp_task_wdt_reset();

    LOG_NOTICE("BUTTONS", "Hardware buttons initialized successfully");
}

void checkHardwareButtons()
{
    unsigned long currentTime = millis();

    // Feed watchdog at start of button check
    esp_task_wdt_reset();

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
                    LOG_NOTICE("BUTTONS", "*** BUTTON %d PRESSED *** GPIO %d -> '%s'",
                               i, defaultButtons[i].gpio, config.buttonShortActions[i].c_str());
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
                            LOG_NOTICE("BUTTONS", "*** BUTTON %d SHORT PRESS *** %lu ms -> '%s'",
                                       i, pressDuration, config.buttonShortActions[i].c_str());
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
                LOG_NOTICE("BUTTONS", "*** BUTTON %d LONG PRESS *** %lu ms -> '%s'",
                           i, pressDuration, config.buttonLongActions[i].c_str());
                handleButtonLongPress(i);
            }
        }

        buttonStates[i].lastState = reading;
    }

    // Feed watchdog at end of button check
    esp_task_wdt_reset();
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

    LOG_VERBOSE("BUTTONS", "=== HANDLING BUTTON %d SHORT PRESS ===", buttonIndex);

    // Check rate limiting
    unsigned long currentTime = millis();
    if (isButtonRateLimited(buttonIndex, currentTime))
    {
        LOG_WARNING("BUTTONS", "Button %d short press RATE LIMITED", buttonIndex);
        return; // Rate limited, ignore this press
    }

    // Get runtime configuration
    const RuntimeConfig &config = getRuntimeConfig();
    const String &shortAction = config.buttonShortActions[buttonIndex];
    const String &shortMqttTopic = config.buttonShortMqttTopics[buttonIndex];

    LOG_NOTICE("BUTTONS", "Button %d triggering SHORT action: '%s'", buttonIndex, shortAction.c_str());

    // Trigger local web endpoint (same as web interface)
    handleButtonAction(shortAction.c_str(), shortMqttTopic.c_str());
}

void handleButtonLongPress(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= numHardwareButtons)
    {
        LOG_ERROR("BUTTONS", "Invalid button index: %d", buttonIndex);
        return;
    }

    LOG_VERBOSE("BUTTONS", "=== HANDLING BUTTON %d LONG PRESS ===", buttonIndex);

    // Check rate limiting
    unsigned long currentTime = millis();
    if (isButtonRateLimited(buttonIndex, currentTime))
    {
        LOG_WARNING("BUTTONS", "Button %d long press RATE LIMITED", buttonIndex);
        return; // Rate limited, ignore this press
    }

    // Get runtime configuration
    const RuntimeConfig &config = getRuntimeConfig();
    const String &longAction = config.buttonLongActions[buttonIndex];
    const String &longMqttTopic = config.buttonLongMqttTopics[buttonIndex];

    LOG_NOTICE("BUTTONS", "Button %d triggering LONG action: '%s'", buttonIndex, longAction.c_str());

    // Trigger local web endpoint (same as web interface)
    handleButtonAction(longAction.c_str(), longMqttTopic.c_str());
}

// Make async local HTTP request to web endpoint (non-blocking)
void makeAsyncLocalRequest(const char *endpoint)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_ERROR("BUTTONS", "Cannot make local request - WiFi not connected");
        return;
    }

    // Use local IP address for the request
    String localIP = WiFi.localIP().toString();
    String url = "http://" + localIP + endpoint;

    LOG_NOTICE("BUTTONS", "Making local HTTP request to: %s", url.c_str());

    // Create HTTPClient for quick local request
    HTTPClient http;

    if (!http.begin(url))
    {
        LOG_ERROR("BUTTONS", "Failed to initialize HTTP client for: %s", url.c_str());
        return;
    }

    // Set short timeout for local requests
    http.setTimeout(5000); // 5 second timeout for local requests
    http.addHeader("User-Agent", "ScribeHardwareButton/1.0");

    LOG_VERBOSE("BUTTONS", "HTTP client configured, making GET request...");

    // Make the request - this should be quick for local requests
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
        LOG_NOTICE("BUTTONS", "Local request SUCCESS: HTTP %d for %s", httpResponseCode, endpoint);

        // Log response for debugging (but don't process it)
        if (httpResponseCode == 200)
        {
            String response = http.getString();
            LOG_VERBOSE("BUTTONS", "Response length: %d bytes", response.length());
        }
    }
    else
    {
        LOG_ERROR("BUTTONS", "Local request FAILED: HTTP %d for %s", httpResponseCode, endpoint);
    }

    // Cleanup immediately
    http.end();

    // Feed watchdog after HTTP operation
    esp_task_wdt_reset();

    LOG_VERBOSE("BUTTONS", "HTTP request completed and cleaned up");
}

// Handle button action - trigger local web endpoint (same as web interface)
void handleButtonAction(const char *endpoint, const char *mqttTopic)
{
    if (strlen(endpoint) == 0)
    {
        LOG_VERBOSE("BUTTONS", "Empty endpoint - no action");
        return;
    }

    LOG_NOTICE("BUTTONS", "=== TRIGGERING WEB ENDPOINT: %s ===", endpoint);

    // Make async HTTP request to local web server (same as web interface)
    makeAsyncLocalRequest(endpoint);

    LOG_NOTICE("BUTTONS", "Button web request initiated for: %s", endpoint);

    // Handle MQTT if topic is specified (future enhancement)
    if (strlen(mqttTopic) > 0)
    {
        LOG_WARNING("BUTTONS", "MQTT functionality not yet implemented for buttons: %s", mqttTopic);
    }
}
