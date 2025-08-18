/**
 * @file settings-utils.js
 * @brief Utility functions for settings page
 * @description Helper functions for UI interactions and formatting
 */

/**
 * Toggle visibility of Unbidden Ink configuration panel
 */
function toggleUnbiddenInkSettings() {
    const enabled = document.getElementById('unbidden-ink-enabled').checked;
    const settings = document.getElementById('unbidden-ink-settings');
    const chatgptApiToken = document.getElementById('chatgpt-api-token');
    
    if (settings) {
        if (enabled) {
            settings.style.display = 'block';
            // Make ChatGPT API token required when Unbidden Ink is enabled
            if (chatgptApiToken) {
                chatgptApiToken.setAttribute('required', 'required');
            }
        } else {
            settings.style.display = 'none';
            // Remove required attribute when Unbidden Ink is disabled
            if (chatgptApiToken) {
                chatgptApiToken.removeAttribute('required');
            }
        }
    }
}

/**
 * Navigate back to the main page
 */
function goBack() {
    window.location.href = '/';
}

/**
 * Update the next scheduled time display
 * @param {string} nextScheduled - ISO date string of next scheduled time
 */
function updateNextScheduledDisplay(nextScheduled) {
    const nextScheduledElement = document.getElementById('next-scheduled');
    if (nextScheduledElement && nextScheduled) {
        try {
            const date = new Date(nextScheduled);
            nextScheduledElement.textContent = date.toLocaleString();
        } catch (error) {
            console.error('Invalid date format:', nextScheduled);
            nextScheduledElement.textContent = 'Invalid date';
        }
    }
}

/**
 * Format time for display
 * @param {number} hour - Hour (0-23)
 * @returns {string} Formatted time string
 */
function formatTime(hour) {
    const period = hour >= 12 ? 'PM' : 'AM';
    const displayHour = hour === 0 ? 12 : hour > 12 ? hour - 12 : hour;
    return `${displayHour}:00 ${period}`;
}

/**
 * Update time range display (if time range slider is present)
 */
function updateTimeRange() {
    const startHour = document.getElementById('time-start')?.value || 8;
    const endHour = document.getElementById('time-end')?.value || 22;
    
    const startDisplay = document.getElementById('start-time-display');
    const endDisplay = document.getElementById('end-time-display');
    
    if (startDisplay) {
        startDisplay.textContent = formatTime(parseInt(startHour));
    }
    if (endDisplay) {
        endDisplay.textContent = formatTime(parseInt(endHour));
    }
}

/**
 * Initialize settings page UI components
 */
function initializeUI() {
    // Set up event listeners for dynamic UI elements
    const unbiddenInkEnabled = document.getElementById('unbidden-ink-enabled');
    if (unbiddenInkEnabled) {
        unbiddenInkEnabled.addEventListener('change', toggleUnbiddenInkSettings);
    }
    
    const timeStart = document.getElementById('time-start');
    const timeEnd = document.getElementById('time-end');
    
    if (timeStart && timeEnd) {
        timeStart.addEventListener('input', updateTimeRange);
        timeEnd.addEventListener('input', updateTimeRange);
        updateTimeRange(); // Initial update
    }
    
    // Initialize Unbidden Ink settings visibility
    toggleUnbiddenInkSettings();
}

/**
 * Validate form inputs before submission
 * @returns {Object} Validation result with isValid boolean and errors array
 */
function validateForm() {
    const errors = [];
    
    // WiFi timeout validation
    const wifiTimeout = parseInt(document.getElementById('wifi-timeout').value);
    if (isNaN(wifiTimeout) || wifiTimeout < 5 || wifiTimeout > 120) {
        errors.push('WiFi timeout must be between 5 and 120 seconds');
    }
    
    // MQTT port validation
    const mqttPort = parseInt(document.getElementById('mqtt-port').value);
    if (isNaN(mqttPort) || mqttPort < 1 || mqttPort > 65535) {
        errors.push('MQTT port must be between 1 and 65535');
    }
    
    // Max characters validation
    const maxChars = parseInt(document.getElementById('max-characters').value);
    if (isNaN(maxChars) || maxChars < 50 || maxChars > 2000) {
        errors.push('Max characters must be between 50 and 2000');
    }
    
    // Unbidden Ink frequency validation
    const frequency = parseInt(document.getElementById('frequency-minutes').value);
    if (isNaN(frequency) || frequency < 5 || frequency > 1440) {
        errors.push('Unbidden Ink frequency must be between 5 and 1440 minutes');
    }
    
    return {
        isValid: errors.length === 0,
        errors: errors
    };
}

/**
 * Show validation errors to the user
 * @param {Array} errors - Array of error messages
 */
function showValidationErrors(errors) {
    const errorMessage = 'Please fix the following errors:\n\n' + errors.join('\n');
    if (window.SettingsCore && window.SettingsCore.showMessage) {
        window.SettingsCore.showMessage(errorMessage, 'error');
    } else {
        alert(errorMessage);
    }
}

/**
 * Reset form to default values
 */
function resetForm() {
    if (confirm('Are you sure you want to reset all settings to defaults? This cannot be undone.')) {
        // Clear all form fields
        document.querySelectorAll('input[type="text"], input[type="number"], input[type="password"], textarea').forEach(input => {
            input.value = '';
        });
        
        document.querySelectorAll('input[type="checkbox"]').forEach(checkbox => {
            checkbox.checked = false;
        });
        
        // Show confirmation
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage('Form reset to defaults', 'info');
        }
    }
}

// Make utility functions available globally
window.SettingsUtils = {
    toggleUnbiddenInkSettings,
    goBack,
    updateNextScheduledDisplay,
    formatTime,
    updateTimeRange,
    initializeUI,
    validateForm,
    showValidationErrors,
    resetForm
};
