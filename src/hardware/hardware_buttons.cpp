#include "hardware_buttons.h"
#include "../web/web_server.h"
#include "printer.h"
#include "../utils/content_actions.h"
#include "../core/config_loader.h"
#include "../core/shared_types.h"
#include "../utils/time_utils.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef ENABLE_LEDS
#include "../leds/LedEffects.h"
extern LedEffects ledEffects;
#endif

// External declarations
extern PubSubClient mqttClient;
extern Message currentMessage;

// ========================================
// ASYNC BUTTON ACTION MANAGEMENT
// ========================================

// Simple task tracking
static volatile bool buttonActionRunning = false;

// Task configuration
static const int BUTTON_TASK_STACK_SIZE = 8192;   // 8KB stack
static const int BUTTON_TASK_PRIORITY = 1;        // Lower priority than main loop
static const int BUTTON_ACTION_TIMEOUT_MS = 3000; // 3s timeout

// Task parameter structure
struct ButtonActionParams
{
    int buttonIndex;
    bool isLongPress;
    String actionType;
    String mqttTopic;
    String ledEffect;
};

// Forward declarations
void buttonActionTask(void *parameter);
bool executeButtonActionDirect(const char *actionType);
bool createButtonActionTask(int buttonIndex, bool isLongPress);

// ========================================
// HARDWARE BUTTON IMPLEMENTATION
// ========================================

// Button state array - dynamically sized based on numHardwareButtons from config.h
ButtonState buttonStates[sizeof(defaultButtons) / sizeof(defaultButtons[0])];

void initializeHardwareButtons()
{
    LOG_NOTICE("BUTTONS", "=== INITIALIZING HARDWARE BUTTONS ===");
    LOG_VERBOSE("BUTTONS", "Button count: %d", numHardwareButtons);
    LOG_VERBOSE("BUTTONS", "Button debounce: %lu ms", buttonDebounceMs);
    LOG_VERBOSE("BUTTONS", "Button long press: %lu ms", buttonLongPressMs);
    LOG_VERBOSE("BUTTONS", "Button active low: %s", buttonActiveLow ? "true" : "false");

    // ESP32-C3 GPIO safety validation before initialization
    for (int i = 0; i < numHardwareButtons; i++)
    {
        int gpio = defaultButtons[i].gpio;

        // ESP32-C3 GPIO range check (0-21)
        if (gpio < 0 || gpio > 21)
        {
            LOG_ERROR("BUTTONS", "Button %d GPIO %d: Invalid GPIO (ESP32-C3 range: 0-21)", i, gpio);
            continue;
        }

        // Warn about potentially problematic GPIOs
        if (gpio == 0 || gpio == 2 || gpio == 3)
        {
            LOG_WARNING("BUTTONS", "Button %d GPIO %d: Strapping pin - may cause boot issues", i, gpio);
        }
        if (gpio == 1)
        {
            LOG_WARNING("BUTTONS", "Button %d GPIO %d: UART0 TX - may cause programming issues", i, gpio);
        }
        if (gpio >= 8 && gpio <= 10)
        {
            LOG_WARNING("BUTTONS", "Button %d GPIO %d: Flash connection - may cause stability issues", i, gpio);
        }
        if (gpio == 18 || gpio == 19)
        {
            LOG_ERROR("BUTTONS", "Button %d GPIO %d: USB pins not available for general use", i, gpio);
            continue;
        }
    }

    // Initialize GPIO pins with error handling
    for (int i = 0; i < numHardwareButtons; i++)
    {
        int gpio = defaultButtons[i].gpio;

        // Skip invalid GPIOs identified above
        if (gpio < 0 || gpio > 21 || gpio == 18 || gpio == 19)
        {
            LOG_ERROR("BUTTONS", "Skipping button %d initialization (invalid GPIO %d)", i, gpio);
            // Initialize state as inactive
            buttonStates[i].currentState = buttonActiveLow ? HIGH : LOW;
            buttonStates[i].lastState = buttonStates[i].currentState;
            buttonStates[i].pressed = false;
            buttonStates[i].longPressTriggered = false;
            buttonStates[i].lastDebounceTime = 0;
            buttonStates[i].pressStartTime = 0;
            buttonStates[i].lastPressTime = 0;
            buttonStates[i].pressCount = 0;
            buttonStates[i].windowStartTime = 0;
            continue;
        }

        // Configure GPIO pin with error protection
        LOG_VERBOSE("BUTTONS", "Configuring button %d GPIO %d...", i, gpio);
        pinMode(gpio, buttonActiveLow ? INPUT_PULLUP : INPUT_PULLDOWN);

        // Small delay for GPIO stabilization on ESP32-C3
        delay(10);

        // Initialize button state with safe reading
        buttonStates[i].currentState = digitalRead(gpio);
        buttonStates[i].lastState = buttonStates[i].currentState;
        buttonStates[i].pressed = false;
        buttonStates[i].longPressTriggered = false;
        buttonStates[i].lastDebounceTime = 0;
        buttonStates[i].pressStartTime = 0;
        buttonStates[i].lastPressTime = 0;
        buttonStates[i].pressCount = 0;
        buttonStates[i].windowStartTime = 0;

        // Feed watchdog after each button to prevent timeout
        esp_task_wdt_reset();

        LOG_VERBOSE("BUTTONS", "Button %d GPIO %d initialized", i, defaultButtons[i].gpio);
    }

    // Get runtime configuration ONCE after GPIO setup
    const RuntimeConfig &config = getRuntimeConfig();

    // Log button configuration
    for (int i = 0; i < numHardwareButtons; i++)
    {
        LOG_NOTICE("BUTTONS", "Button %d: GPIO %d -> Short: '%s', Long: '%s'",
                   i, defaultButtons[i].gpio,
                   config.buttonShortActions[i].c_str(),
                   config.buttonLongActions[i].c_str());

        // Feed watchdog after each log to prevent timeout
        esp_task_wdt_reset();
    }

    LOG_NOTICE("BUTTONS", "Hardware buttons initialized successfully");
}

void checkHardwareButtons()
{
    unsigned long currentTime = millis();

    // Feed watchdog at start of button check
    esp_task_wdt_reset();

    // Get runtime configuration ONCE to avoid repeated calls
    const RuntimeConfig &config = getRuntimeConfig();

    for (int i = 0; i < numHardwareButtons; i++)
    {
        int gpio = defaultButtons[i].gpio;

        // Skip buttons with invalid GPIOs (safety check)
        if (gpio < 0 || gpio > 21 || gpio == 18 || gpio == 19)
        {
            // Skip this button entirely - GPIO not safe for ESP32-C3
            continue;
        }

        // Safe GPIO reading with bounds checking
        bool reading = digitalRead(gpio);

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

                    LOG_NOTICE("BUTTONS", "*** BUTTON %d PRESSED *** GPIO %d -> '%s'",
                               i, gpio, config.buttonShortActions[i].c_str());
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

// ========================================
// LED EFFECTS (kept for compatibility with async task)
// ========================================

void triggerButtonLedEffect(int buttonIndex, bool isLongPress)
{
#ifdef ENABLE_LEDS
    // Get runtime configuration to access configured LED effects (avoid repeated calls)
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

    // Feed watchdog at start of button handling
    esp_task_wdt_reset();

    // Check rate limiting
    unsigned long currentTime = millis();
    if (isButtonRateLimited(buttonIndex, currentTime))
    {
        LOG_WARNING("BUTTONS", "Button %d short press RATE LIMITED", buttonIndex);
        return; // Rate limited, ignore this press
    }

    // **KEY CHANGE**: Process action asynchronously instead of immediate execution
    // This keeps the main loop responsive and prevents blocking operations
    bool started = createButtonActionTask(buttonIndex, false);

    if (started)
    {
        LOG_NOTICE("BUTTONS", "Button %d SHORT press started async processing", buttonIndex);
    }
    else
    {
        LOG_WARNING("BUTTONS", "Failed to start async task for button %d SHORT press (task busy?)", buttonIndex);
    }

    // Feed watchdog after quick, non-blocking button handling
    esp_task_wdt_reset();
}

void handleButtonLongPress(int buttonIndex)
{
    if (buttonIndex < 0 || buttonIndex >= numHardwareButtons)
    {
        LOG_ERROR("BUTTONS", "Invalid button index: %d", buttonIndex);
        return;
    }

    LOG_VERBOSE("BUTTONS", "=== HANDLING BUTTON %d LONG PRESS ===", buttonIndex);

    // Feed watchdog at start of button handling
    esp_task_wdt_reset();

    // Check rate limiting
    unsigned long currentTime = millis();
    if (isButtonRateLimited(buttonIndex, currentTime))
    {
        LOG_WARNING("BUTTONS", "Button %d long press RATE LIMITED", buttonIndex);
        return; // Rate limited, ignore this press
    }

    // **KEY CHANGE**: Process action asynchronously instead of immediate execution
    // This keeps the main loop responsive and prevents blocking operations
    bool started = createButtonActionTask(buttonIndex, true);

    if (started)
    {
        LOG_NOTICE("BUTTONS", "Button %d LONG press started async processing", buttonIndex);
    }
    else
    {
        LOG_WARNING("BUTTONS", "Failed to start async task for button %d LONG press (task busy?)", buttonIndex);
    }

    // Feed watchdog after quick, non-blocking button handling
    esp_task_wdt_reset();
}

// ========================================
// ASYNC BUTTON ACTION IMPLEMENTATION
// ========================================

/**
 * @brief Create async task to handle button action (non-blocking)
 */
bool createButtonActionTask(int buttonIndex, bool isLongPress)
{
    // Rate limiting: reject if another task is running
    if (buttonActionRunning)
    {
        LOG_WARNING("BUTTONS", "Button action in progress - rejecting button %d press", buttonIndex);
        return false;
    }

    if (buttonIndex < 0 || buttonIndex >= numHardwareButtons)
    {
        LOG_ERROR("BUTTONS", "Invalid button index: %d", buttonIndex);
        return false;
    }

    // Get configuration for this button immediately
    const RuntimeConfig &config = getRuntimeConfig();

    // Create task parameters (will be freed by task)
    ButtonActionParams *params = new ButtonActionParams();
    params->buttonIndex = buttonIndex;
    params->isLongPress = isLongPress;

    if (isLongPress)
    {
        params->actionType = config.buttonLongActions[buttonIndex];
        params->mqttTopic = config.buttonLongMqttTopics[buttonIndex];
        params->ledEffect = config.buttonLongLedEffects[buttonIndex];
    }
    else
    {
        params->actionType = config.buttonShortActions[buttonIndex];
        params->mqttTopic = config.buttonShortMqttTopics[buttonIndex];
        params->ledEffect = config.buttonShortLedEffects[buttonIndex];
    }

    // Create one-shot task to handle this action
    TaskHandle_t taskHandle = NULL;
    BaseType_t result = xTaskCreate(
        buttonActionTask,       // Task function
        "ButtonAction",         // Task name
        BUTTON_TASK_STACK_SIZE, // Stack size
        params,                 // Parameters (will be freed by task)
        BUTTON_TASK_PRIORITY,   // Priority
        &taskHandle             // Task handle
    );

    if (result != pdPASS)
    {
        LOG_ERROR("BUTTONS", "Failed to create button action task");
        delete params;
        return false;
    }

    LOG_NOTICE("BUTTONS", "Created async task for %s press on button %d: '%s'",
               isLongPress ? "long" : "short", buttonIndex, params->actionType.c_str());

    return true;
}

/**
 * @brief One-shot task function for processing a single button action
 */
void buttonActionTask(void *parameter)
{
    buttonActionRunning = true;

    // Get parameters
    ButtonActionParams *params = (ButtonActionParams *)parameter;

    LOG_NOTICE("BUTTONS", "Processing %s press for button %d: '%s'",
               params->isLongPress ? "long" : "short",
               params->buttonIndex, params->actionType.c_str());

    // Trigger LED effect immediately (non-blocking)
    triggerButtonLedEffect(params->buttonIndex, params->isLongPress);

    // Process the action directly (no HTTP calls!)
    if (params->actionType.length() > 0)
    {
        // Execute content action directly with timeout
        bool success = executeButtonActionDirect(params->actionType.c_str());

        if (success)
        {
            LOG_NOTICE("BUTTONS", "Button action completed successfully: %s",
                       params->actionType.c_str());
        }
        else
        {
            LOG_WARNING("BUTTONS", "Button action failed or timed out: %s",
                        params->actionType.c_str());
        }
    }

    // Handle MQTT if specified (placeholder)
    if (params->mqttTopic.length() > 0)
    {
        LOG_WARNING("BUTTONS", "MQTT functionality not yet implemented for buttons: %s",
                    params->mqttTopic.c_str());
    }

    // Cleanup and mark complete
    delete params;
    buttonActionRunning = false;

    LOG_VERBOSE("BUTTONS", "Button action task completed, deleting task");

    // Delete this task
    vTaskDelete(NULL);
}

/**
 * @brief Direct execution of button actions without HTTP layer
 */
bool executeButtonActionDirect(const char *actionType)
{
    if (!actionType || strlen(actionType) == 0)
    {
        LOG_ERROR("BUTTONS", "Invalid action type: null or empty");
        return false;
    }

    LOG_NOTICE("BUTTONS", "Executing button action directly: %s", actionType);

    // Convert action type string to ContentActionType enum using utility function
    ContentActionType contentAction = stringToActionType(String(actionType));

    // Execute content action directly with button-specific timeout
    ContentActionResult result = executeContentActionWithTimeout(
        contentAction, "", "", BUTTON_ACTION_TIMEOUT_MS);

    if (result.success && result.content.length() > 0)
    {
        // Queue content for printing (this sets currentMessage)
        currentMessage.message = result.content;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;

        LOG_NOTICE("BUTTONS", "Content queued for printing (%d chars)", result.content.length());
        return true;
    }
    else
    {
        LOG_ERROR("BUTTONS", "Failed to generate content for action: %s", actionType);
        return false;
    }
}

// ========================================
// DEPRECATED FUNCTIONS
// ========================================

// Execute button endpoint using shared content action utilities
void executeButtonEndpoint(const char *endpoint)
{
    if (!endpoint || strlen(endpoint) == 0)
    {
        LOG_ERROR("BUTTONS", "Invalid endpoint: null or empty");
        return;
    }

    LOG_NOTICE("BUTTONS", "Executing button endpoint: %s", endpoint);

    // Feed watchdog before starting content generation
    esp_task_wdt_reset();

    // Convert endpoint to content action type
    String endpointStr = String(endpoint);
    ContentActionType actionType = endpointToActionType(endpointStr);

    // Validate known endpoint
    if (actionTypeToString(actionType) == "UNKNOWN" && endpointStr != "/api/joke")
    {
        LOG_WARNING("BUTTONS", "Unknown button endpoint: %s", endpoint);
        return;
    }

    // Feed watchdog before content generation
    esp_task_wdt_reset();

    // Execute content action and queue for printing
    bool success = executeAndQueueContent(actionType);

    // Feed watchdog after content generation
    esp_task_wdt_reset();

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

    // Feed watchdog after printing
    esp_task_wdt_reset();
}
