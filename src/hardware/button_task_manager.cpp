#include "button_task_manager.h"
#include "../core/config_loader.h"
#include "../utils/content_actions.h"
#include "../utils/time_utils.h"
#include "../hardware/printer.h"
#include "../hardware/hardware_buttons.h"
#include "../core/shared_types.h"
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef ENABLE_LEDS
#include "../leds/LedEffects.h"
extern LedEffects ledEffects;
#endif

// ========================================
// FORWARD DECLARATIONS
// ========================================
bool executeButtonActionDirect(const char *actionType);
void buttonActionTask(void *parameter);

// ========================================
// BUTTON TASK MANAGER IMPLEMENTATION
// ========================================

// Simple task tracking
static volatile bool buttonTaskRunning = false;

// Configuration
static const int BUTTON_TASK_STACK_SIZE = 8192;   // 8KB stack for HTTP operations
static const int BUTTON_TASK_PRIORITY = 1;        // Lower priority than main loop
static const int BUTTON_ACTION_TIMEOUT_MS = 3000; // 3s timeout for button-triggered HTTP calls

void initializeButtonTaskManager()
{
    LOG_NOTICE("BUTTON_TASK", "Initialized async button task manager (no queue)");
}

bool processButtonActionAsync(int buttonIndex, bool isLongPress)
{
    // Rate limiting: reject if another task is running
    if (buttonTaskRunning)
    {
        LOG_WARNING("BUTTON_TASK", "Button action in progress - rejecting button %d press", buttonIndex);
        return false;
    }

    if (buttonIndex < 0 || buttonIndex >= numHardwareButtons)
    {
        LOG_ERROR("BUTTON_TASK", "Invalid button index: %d", buttonIndex);
        return false;
    }

    // Get configuration for this button immediately
    const RuntimeConfig &config = getRuntimeConfig();

    // Create task parameters (will be freed by task)
    struct ButtonTaskParams
    {
        int buttonIndex;
        bool isLongPress;
        String actionType;
        String mqttTopic;
        String ledEffect;
    };

    ButtonTaskParams *params = new ButtonTaskParams();
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
        LOG_ERROR("BUTTON_TASK", "Failed to create button action task");
        delete params;
        return false;
    }

    LOG_NOTICE("BUTTON_TASK", "Started async task for %s press on button %d: '%s'",
               isLongPress ? "long" : "short", buttonIndex, params->actionType.c_str());

    return true;
}

bool isButtonTaskBusy()
{
    return buttonTaskRunning;
}

/**
 * @brief One-shot task function for processing a single button action
 * Task deletes itself when complete
 */
void buttonActionTask(void *parameter)
{
    buttonTaskRunning = true;

    // Get parameters
    struct ButtonTaskParams
    {
        int buttonIndex;
        bool isLongPress;
        String actionType;
        String mqttTopic;
        String ledEffect;
    };
    ButtonTaskParams *params = (ButtonTaskParams *)parameter;

    LOG_NOTICE("BUTTON_TASK", "Processing %s press for button %d: '%s'",
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
            LOG_NOTICE("BUTTON_TASK", "Button action completed successfully: %s",
                       params->actionType.c_str());
        }
        else
        {
            LOG_WARNING("BUTTON_TASK", "Button action failed or timed out: %s",
                        params->actionType.c_str());
        }
    }

    // Handle MQTT if specified
    if (params->mqttTopic.length() > 0)
    {
        LOG_WARNING("BUTTON_TASK", "MQTT functionality not yet implemented for buttons: %s",
                    params->mqttTopic.c_str());
    }

    // Cleanup and mark complete
    delete params;
    buttonTaskRunning = false;

    LOG_VERBOSE("BUTTON_TASK", "Button action task completed, deleting task");

    // Delete this task
    vTaskDelete(NULL);
}

/**
 * @brief Direct execution of button actions without HTTP layer
 * @param actionType The content action type (JOKE, RIDDLE, etc.)
 * @return true if action completed successfully
 */
bool executeButtonActionDirect(const char *actionType)
{
    if (!actionType || strlen(actionType) == 0)
    {
        LOG_ERROR("BUTTON_TASK", "Invalid action type: null or empty");
        return false;
    }

    LOG_NOTICE("BUTTON_TASK", "Executing button action directly: %s", actionType);

    // Convert action type string to ContentActionType enum using utility function
    ContentActionType contentAction = stringToActionType(String(actionType));

    // Execute content action directly with button-specific timeout
    ContentActionResult result = executeContentActionWithTimeout(
        contentAction, "", "", BUTTON_ACTION_TIMEOUT_MS);

    if (result.success && result.content.length() > 0)
    {
        // Queue content for printing (this sets currentMessage)
        extern Message currentMessage;
        currentMessage.message = result.content;
        currentMessage.timestamp = getFormattedDateTime();
        currentMessage.shouldPrintLocally = true;

        LOG_NOTICE("BUTTON_TASK", "Content queued for printing (%d chars)", result.content.length());
        return true;
    }
    else
    {
        LOG_ERROR("BUTTON_TASK", "Failed to generate content for action: %s", actionType);
        return false;
    }
}
