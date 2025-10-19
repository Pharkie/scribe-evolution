#include "hardware_buttons.h"
#include <web/web_server.h>
#include "printer.h"
#include <content/content_handlers.h>
#include <utils/content_actions.h>
#include <config/config.h>
#include <core/config_loader.h>
#include <core/shared_types.h>
#include <core/network.h>
#include <core/mqtt_handler.h>
#include <utils/time_utils.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#if ENABLE_LEDS
#include <leds/LedEffects.h>
#endif

// External declarations
extern PubSubClient mqttClient;
extern Message currentMessage;
extern SemaphoreHandle_t currentMessageMutex;

// ========================================
// ASYNC BUTTON ACTION MANAGEMENT
// ========================================

// Thread-safe task tracking with mutex protection
static volatile bool buttonActionRunning = false;
static SemaphoreHandle_t buttonActionMutex = nullptr;

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
bool executeButtonActionDirect(const char *actionType, bool shouldSetPrintFlag = true);
bool createButtonActionTask(int buttonIndex, bool isLongPress);

// ========================================
// HARDWARE BUTTON IMPLEMENTATION
// ========================================

// Button state array - dynamically sized based on numHardwareButtons from config.h
ButtonState buttonStates[sizeof(defaultButtons) / sizeof(defaultButtons[0])];

void initializeHardwareButtons()
{
    // Initialize mutex for thread-safe button action handling
    if (buttonActionMutex == nullptr)
    {
        buttonActionMutex = xSemaphoreCreateMutex();
        if (buttonActionMutex == nullptr)
        {
            LOG_ERROR("BUTTONS", "Failed to create button action mutex");
        }
    }

    LOG_NOTICE("BUTTONS", "Buttons: Initializing %d hardware buttons", numHardwareButtons);
    LOG_VERBOSE("BUTTONS", "Button count: %d", numHardwareButtons);
    LOG_VERBOSE("BUTTONS", "Button debounce: %lu ms", buttonDebounceMs);
    LOG_VERBOSE("BUTTONS", "Button long press: %lu ms", buttonLongPressMs);
    LOG_VERBOSE("BUTTONS", "Button active low: %s", buttonActiveLow ? "true" : "false");

    // Get runtime configuration ONCE after GPIO setup
    const RuntimeConfig &config = getRuntimeConfig();

    // GPIO safety validation before initialization
    for (int i = 0; i < numHardwareButtons; i++)
    {
        int gpio = config.buttonGpios[i];

        // Validate GPIO using centralized board configuration
        if (!isValidGPIO(gpio))
        {
            LOG_ERROR("BUTTONS", "Button %d GPIO %d: Invalid GPIO - not available on %s", i, gpio, BOARD_NAME);
            continue;
        }

        if (!isSafeGPIO(gpio))
        {
            LOG_WARNING("BUTTONS", "Button %d GPIO %d: %s", i, gpio, getGPIODescription(gpio));
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
        int gpio = config.buttonGpios[i];

        // Skip invalid GPIOs identified above - use board-specific validation
        if (!isValidGPIO(gpio))
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

        // Small delay for GPIO stabilization
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

        LOG_VERBOSE("BUTTONS", "Button %d GPIO %d initialized", i, config.buttonGpios[i]);
    }

    // Log button configuration (verbose detail)
    for (int i = 0; i < numHardwareButtons; i++)
    {
        LOG_VERBOSE("BUTTONS", "Button %d: GPIO %d -> Short: '%s', Long: '%s'",
                    i, config.buttonGpios[i],
                    config.buttonShortActions[i].c_str(),
                    config.buttonLongActions[i].c_str());

        // Feed watchdog after each log to prevent timeout
        esp_task_wdt_reset();
    }

    LOG_NOTICE("BUTTONS", "Buttons: ✅ All %d buttons ready", numHardwareButtons);
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
        int gpio = config.buttonGpios[i];

        // Skip buttons with invalid GPIOs (board-specific safety check)
        if (!isValidGPIO(gpio))
        {
            // Skip this button entirely - GPIO not valid for this board
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

                    LOG_VERBOSE("BUTTONS", "*** BUTTON %d PRESSED *** GPIO %d -> '%s'",
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
                            LOG_VERBOSE("BUTTONS", "*** BUTTON %d SHORT PRESS *** %lu ms -> '%s'",
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
                LOG_VERBOSE("BUTTONS", "*** BUTTON %d LONG PRESS *** %lu ms -> '%s'",
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
#if ENABLE_LEDS
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

    // Trigger configured LED effect for 1 cycle using autonomous defaults
    if (ledEffects().startEffectCyclesAuto(effectName, 1))
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
        LOG_VERBOSE("BUTTONS", "Button %d SHORT press started async processing", buttonIndex);
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
        LOG_VERBOSE("BUTTONS", "Button %d LONG press started async processing", buttonIndex);
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
    // Thread-safe check if another task is running
    if (buttonActionMutex == nullptr || 
        xSemaphoreTake(buttonActionMutex, pdMS_TO_TICKS(10)) != pdTRUE)
    {
        LOG_WARNING("BUTTONS", "Button action mutex unavailable - rejecting button %d press", buttonIndex);
        return false;
    }

    // Critical section: check and set running flag atomically
    if (buttonActionRunning)
    {
        xSemaphoreGive(buttonActionMutex);
        LOG_WARNING("BUTTONS", "Button action in progress - rejecting button %d press", buttonIndex);
        return false;
    }

    // Set flag while still holding mutex
    buttonActionRunning = true;
    xSemaphoreGive(buttonActionMutex);

    if (buttonIndex < 0 || buttonIndex >= numHardwareButtons)
    {
        // Reset flag on error since we already set it
        if (xSemaphoreTake(buttonActionMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            buttonActionRunning = false;
            xSemaphoreGive(buttonActionMutex);
        }
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
        // Reset flag on task creation failure
        if (xSemaphoreTake(buttonActionMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            buttonActionRunning = false;
            xSemaphoreGive(buttonActionMutex);
        }
        LOG_ERROR("BUTTONS", "Failed to create button action task");
        delete params;
        return false;
    }

    LOG_VERBOSE("BUTTONS", "Created async task for %s press on button %d: '%s'",
                isLongPress ? "long" : "short", buttonIndex, params->actionType.c_str());

    return true;
}

/**
 * @brief One-shot task function for processing a single button action
 */
void buttonActionTask(void *parameter)
{
    // Flag is already set in createButtonActionTask under mutex protection
    
    // Get parameters
    ButtonActionParams *params = (ButtonActionParams *)parameter;

    LOG_VERBOSE("BUTTONS", "Processing %s press for button %d: '%s'",
                params->isLongPress ? "long" : "short",
                params->buttonIndex, params->actionType.c_str());

    // Trigger LED effect immediately (non-blocking)
    triggerButtonLedEffect(params->buttonIndex, params->isLongPress);

    // FIXED: Execute action based on MQTT configuration
    // If MQTT topic is set, send ONLY via MQTT. If not set, print locally.
    if (params->mqttTopic.length() > 0)
    {
        // MQTT path: generate content and send via MQTT only
        if (params->actionType.length() > 0)
        {
            // Execute content action for MQTT (don't set print flag)
            bool success = executeButtonActionDirect(params->actionType.c_str(), false);

            if (success)
            {
                // Parse message content into header and body (format: "header\n\nbody")
                String message = currentMessage.message;
                int separatorPos = message.indexOf("\n\n");
                
                String header, body;
                if (separatorPos != -1) {
                    header = message.substring(0, separatorPos);
                    body = message.substring(separatorPos + 2);
                } else {
                    // Fallback: use action type as header if no separator found
                    header = params->actionType.c_str();
                    body = message;
                }
                
                // Use centralized MQTT publishing function
                bool mqttSuccess = publishMQTTMessage(params->mqttTopic, header, body);
                
                if (mqttSuccess) {
                    const RuntimeConfig &config = getRuntimeConfig();
                    int gpio = config.buttonGpios[params->buttonIndex];
                    LOG_NOTICE("BUTTONS", "Button %d (GPIO%d) sent %s via MQTT to: %s", 
                              params->buttonIndex + 1, gpio, params->actionType.c_str(), params->mqttTopic.c_str());
                } else {
                    LOG_ERROR("BUTTONS", "Failed to send button action via MQTT to topic: %s", params->mqttTopic.c_str());
                }
            }
            else if (!isMQTTEnabled())
            {
                LOG_WARNING("BUTTONS", "MQTT disabled, cannot send button action to topic: %s", params->mqttTopic.c_str());
            }
            else if (!mqttClient.connected())
            {
                LOG_WARNING("BUTTONS", "MQTT not connected, cannot send button action to topic: %s", params->mqttTopic.c_str());
            }
            else
            {
                LOG_WARNING("BUTTONS", "Content generation failed for MQTT button action: %s", params->actionType.c_str());
            }
        }
    }
    else
    {
        // Local printing path: no MQTT topic set, print directly
        if (params->actionType.length() > 0)
        {
            // Execute content action directly with timeout
            bool success = executeButtonActionDirect(params->actionType.c_str());

            if (success)
            {
                LOG_NOTICE("BUTTONS", "Button action completed for LOCAL printing: %s", params->actionType.c_str());
                // executeButtonActionDirect already sets shouldPrintLocally = true
            }
            else
            {
                LOG_WARNING("BUTTONS", "Button action failed or timed out: %s", params->actionType.c_str());
            }
        }
    }

    // Cleanup and mark complete
    delete params;

    // Thread-safe reset of running flag
    if (buttonActionMutex != nullptr && 
        xSemaphoreTake(buttonActionMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        buttonActionRunning = false;
        xSemaphoreGive(buttonActionMutex);
    }

    LOG_VERBOSE("BUTTONS", "Button action task completed, deleting task");

    // Delete this task
    vTaskDelete(NULL);
}

/**
 * @brief Direct execution of button actions without HTTP layer
 */
bool executeButtonActionDirect(const char *actionType, bool shouldSetPrintFlag)
{
    if (!actionType || strlen(actionType) == 0)
    {
        LOG_ERROR("BUTTONS", "Invalid action type: null or empty");
        return false;
    }

    // Skip internet-dependent actions when WiFi is not connected
    if (isAPMode() || WiFi.status() != WL_CONNECTED)
    {
        LOG_WARNING("BUTTONS", "Skipping button action '%s' - no internet connection (WiFi status: %d)", 
                    actionType, WiFi.status());
        return false;
    }

    LOG_VERBOSE("BUTTONS", "Executing button action directly: %s (print flag: %s)", 
                actionType, shouldSetPrintFlag ? "true" : "false");

    String actionString = String(actionType);
    
    // Convert action type string to ContentActionType enum using utility function
    ContentActionType contentAction = stringToActionType(actionString);

    // Get sender information for content generation
    const RuntimeConfig &config = getRuntimeConfig();
    String senderName = (shouldSetPrintFlag == false && config.deviceOwner.length() > 0) ? 
                        config.deviceOwner : "";
    
    // Execute content action directly with button-specific timeout
    ContentActionResult result = executeContentActionWithTimeout(
        contentAction, "", senderName, BUTTON_ACTION_TIMEOUT_MS);

    if (result.success && result.header.length() > 0 && result.body.length() > 0)
    {
        // Format content for local printing (header + body)
        String formattedContent = result.header + "\n\n" + result.body;

        // Set currentMessage content and timestamp (protected by mutex for multi-core safety)
        if (xSemaphoreTake(currentMessageMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            currentMessage.message = formattedContent;
            currentMessage.timestamp = getFormattedDateTime();
            currentMessage.shouldPrintLocally = shouldSetPrintFlag;
            xSemaphoreGive(currentMessageMutex);

            LOG_VERBOSE("BUTTONS", "Content generated (%d chars), shouldPrintLocally = %s",
                        formattedContent.length(), shouldSetPrintFlag ? "true" : "false");
            return true;
        }
        else
        {
            LOG_ERROR("BUTTONS", "Failed to acquire mutex for currentMessage");
            return false;
        }
    }
    else
    {
        LOG_ERROR("BUTTONS", "Failed to generate content for action: %s", actionType);
        return false;
    }
}
