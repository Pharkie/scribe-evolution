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
#include <WiFi.h>
#include <vector>

#ifdef ENABLE_LEDS
#include "../leds/LedEffects.h"
extern LedEffects ledEffects;
#endif

// External declarations
extern PubSubClient mqttClient;
extern Message currentMessage;

// ========================================
// BUTTON ACTION QUEUE SYSTEM
// ========================================

// Action queue for async processing
static std::vector<ButtonAction> buttonActionQueue;
static unsigned long lastActionProcessTime = 0;
static const unsigned long ACTION_PROCESS_INTERVAL = 100; // Process actions every 100ms
static const unsigned long ACTION_TIMEOUT_MS = 30000; // 30 second timeout for queued actions
static const size_t MAX_QUEUE_SIZE = 20; // Prevent memory issues

// ========================================
// HARDWARE BUTTON IMPLEMENTATION
// ========================================

void initializeButtonActionQueue()
{
    buttonActionQueue.clear();
    buttonActionQueue.reserve(MAX_QUEUE_SIZE);
    lastActionProcessTime = millis();
    LOG_VERBOSE("BUTTONS", "Button action queue initialized (max size: %d)", MAX_QUEUE_SIZE);
}

void initializeHardwareButtons()
{
    LOG_NOTICE("BUTTONS", "=== INITIALIZING HARDWARE BUTTONS ===");
    LOG_VERBOSE("BUTTONS", "Button count: %d", numHardwareButtons);
    LOG_VERBOSE("BUTTONS", "Button debounce: %lu ms", buttonDebounceMs);
    LOG_VERBOSE("BUTTONS", "Button long press: %lu ms", buttonLongPressMs);
    LOG_VERBOSE("BUTTONS", "Button active low: %s", buttonActiveLow ? "true" : "false");

    // Initialize action queue system
    initializeButtonActionQueue();

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

void processButtonActionQueue()
{
    unsigned long currentTime = millis();
    
    // Throttle processing to avoid overwhelming the system
    if (currentTime - lastActionProcessTime < ACTION_PROCESS_INTERVAL)
    {
        return;
    }
    lastActionProcessTime = currentTime;
    
    // Feed watchdog
    esp_task_wdt_reset();
    
    if (buttonActionQueue.empty())
    {
        return;
    }
    
    // Process one action at a time to keep it non-blocking
    ButtonAction action = buttonActionQueue.front();
    buttonActionQueue.erase(buttonActionQueue.begin());
    
    // Check if action has timed out
    if (currentTime - action.timestamp > ACTION_TIMEOUT_MS)
    {
        LOG_WARNING("BUTTONS", "Button action timed out: %s (queued for %lu ms)", 
                   action.endpoint.c_str(), currentTime - action.timestamp);
        return;
    }
    
    LOG_VERBOSE("BUTTONS", "Processing queued button action: %s (queue size: %d)", 
               action.endpoint.c_str(), buttonActionQueue.size());
    
    // Execute the action directly without HTTP calls
    if (action.endpoint.length() > 0)
    {
        executeButtonEndpoint(action.endpoint.c_str());
    }
    
    // Handle MQTT if topic is specified (future enhancement)
    if (action.mqttTopic.length() > 0)
    {
        LOG_WARNING("BUTTONS", "MQTT functionality not yet implemented for buttons: %s", action.mqttTopic.c_str());
    }
}

void queueButtonAction(const String &endpoint, const String &mqttTopic, int buttonIndex, bool isLongPress)
{
    if (endpoint.length() == 0)
    {
        LOG_VERBOSE("BUTTONS", "Empty endpoint - no action queued");
        return;
    }
    
    // Check queue size limit
    if (buttonActionQueue.size() >= MAX_QUEUE_SIZE)
    {
        LOG_ERROR("BUTTONS", "Button action queue full (%d items) - dropping action: %s", 
                 buttonActionQueue.size(), endpoint.c_str());
        return;
    }
    
    // Create action
    ButtonAction action;
    action.endpoint = endpoint;
    action.mqttTopic = mqttTopic;
    action.timestamp = millis();
    action.buttonIndex = buttonIndex;
    action.isLongPress = isLongPress;
    
    // Add to queue
    buttonActionQueue.push_back(action);
    
    LOG_NOTICE("BUTTONS", "=== QUEUED BUTTON ACTION: %s === (button %d, %s, queue size: %d)",
               endpoint.c_str(), buttonIndex, isLongPress ? "LONG" : "SHORT", buttonActionQueue.size());
}

void triggerButtonLedEffect(int buttonIndex, bool isLongPress)
{
#ifdef ENABLE_LEDS
    // Get runtime configuration to access configured LED effects
    const RuntimeConfig &config = getRuntimeConfig();
    
    String effectName;
    if (isLongPress)
    {
        effectName = config.buttonLongLedEffects[buttonIndex];
    }
    else
    {
        effectName = config.buttonShortLedEffects[buttonIndex];
    }
    
    // Skip if effect is disabled
    if (effectName == "none" || effectName.length() == 0)
    {
        LOG_VERBOSE("BUTTONS", "LED effect disabled for button %d (%s press)", 
                   buttonIndex, isLongPress ? "long" : "short");
        return;
    }
    
    // Trigger configured LED effect for 1 cycle with appropriate colors
    CRGB color = CRGB::Green; // Default to green for most effects
    if (effectName == "rainbow")
        color = CRGB::White; // Rainbow uses its own colors
    else if (effectName == "pulse")
        color = CRGB::Blue;
    else if (effectName == "matrix")
        color = CRGB::Green;
    else if (effectName == "twinkle")
        color = CRGB::Yellow;
    
    if (ledEffects.startEffectCycles(effectName, 1, color))
    {
        LOG_VERBOSE("BUTTONS", "LED effect triggered for button %d (%s press): %s, 1 cycle",
                   buttonIndex, isLongPress ? "long" : "short", effectName.c_str());
    }
    else
    {
        LOG_WARNING("BUTTONS", "Failed to trigger LED effect '%s' for button %d", 
                   effectName.c_str(), buttonIndex);
    }
#else
    LOG_VERBOSE("BUTTONS", "LED effects disabled - no effect for button %d", buttonIndex);
#endif
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

    // Trigger immediate LED effect (non-blocking)
    triggerButtonLedEffect(buttonIndex, false);

    // Queue action for async processing (non-blocking)
    queueButtonAction(shortAction, shortMqttTopic, buttonIndex, false);
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

    // Trigger immediate LED effect (non-blocking)
    triggerButtonLedEffect(buttonIndex, true);

    // Queue action for async processing (non-blocking)
    queueButtonAction(longAction, longMqttTopic, buttonIndex, true);
}

// Execute button endpoint directly without HTTP calls
void executeButtonEndpoint(const char *endpoint)
{
    LOG_NOTICE("BUTTONS", "Executing button endpoint: %s", endpoint);
    
    bool success = false;
    
    if (strcmp(endpoint, "/api/joke") == 0)
    {
        success = generateAndQueueJoke();
    }
    else if (strcmp(endpoint, "/api/riddle") == 0)
    {
        success = generateAndQueueRiddle();
    }
    else if (strcmp(endpoint, "/api/quote") == 0)
    {
        success = generateAndQueueQuote();
    }
    else if (strcmp(endpoint, "/api/quiz") == 0)
    {
        success = generateAndQueueQuiz();
    }
    else if (strcmp(endpoint, "/api/character-test") == 0)
    {
        success = generateAndQueuePrintTest();
    }
    else if (strcmp(endpoint, "/api/news") == 0)
    {
        success = generateAndQueueNews();
    }
    else
    {
        LOG_WARNING("BUTTONS", "Unknown button endpoint: %s", endpoint);
        return;
    }
    
    if (success)
    {
        // Directly print the queued content
        printMessage();
        LOG_NOTICE("BUTTONS", "Button action executed successfully: %s", endpoint);
    }
    else
    {
        LOG_ERROR("BUTTONS", "Failed to generate content for button endpoint: %s", endpoint);
    }
}
