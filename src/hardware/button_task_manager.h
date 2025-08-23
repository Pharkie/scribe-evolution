#ifndef BUTTON_TASK_MANAGER_H
#define BUTTON_TASK_MANAGER_H

#include "../core/config.h"
#include "../core/logging.h"
#include <Arduino.h>

// ========================================
// SAFE ASYNC BUTTON TASK MANAGEMENT
// ========================================

/**
 * @brief Initialize the button task manager
 * No background tasks or queues - just utility functions
 */
void initializeButtonTaskManager();

/**
 * @brief Process a button action immediately and asynchronously
 * This function spawns an async task to handle the button press
 * @param buttonIndex Which button was pressed
 * @param isLongPress Whether this was a long press
 * @return true if task was created successfully
 */
bool processButtonActionAsync(int buttonIndex, bool isLongPress);

/**
 * @brief Check if any button task is currently running
 * @return true if a button action is being processed
 */
bool isButtonTaskBusy();

#endif // BUTTON_TASK_MANAGER_H
