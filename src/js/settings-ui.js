/**
 * @file settings-ui.js
 * @brief UI manipulation module - handles all DOM interactions and form management
 * @description Focused module for user interface operations, form population, and message display
 */

/**
 * Populate all form sections with configuration data
 * @param {Object} config - Configuration object from server
 */
function populateForm(config) {
    // Device configuration
    setElementValue('device-owner', config.device?.owner || '');
    setElementValue('timezone', config.device?.timezone || '');
    
    // WiFi configuration
    setElementValue('wifi-ssid', config.wifi?.ssid || '');
    setElementValue('wifi-password', config.wifi?.password || '');
    setElementValue('wifi-timeout', (config.wifi?.connect_timeout || 15000) / 1000); // Convert from milliseconds
    
    // MQTT configuration
    setElementValue('mqtt-server', config.mqtt?.server || '');
    setElementValue('mqtt-port', config.mqtt?.port || 1883);
    setElementValue('mqtt-username', config.mqtt?.username || '');
    setElementValue('mqtt-password', config.mqtt?.password || '');
    
    // Validation configuration
    setElementValue('max-characters', config.validation?.maxCharacters || 500);
    
    // ChatGPT configuration
    setElementValue('chatgpt-api-token', config.apis?.chatgptApiToken || '');
    
    // Unbidden Ink configuration
    setElementChecked('unbidden-ink-enabled', config.unbiddenInk?.enabled || false);
    
    // Time range sliders
    setElementValue('time-start', config.unbiddenInk?.startHour || 8);
    setElementValue('time-end', config.unbiddenInk?.endHour || 22);
    updateTimeRange();
    
    // Frequency and prompt
    setElementValue('frequency-minutes', config.unbiddenInk?.frequencyMinutes || 60);
    setElementValue('unbidden-ink-prompt', config.unbiddenInk?.prompt || '');
    
    // Button configuration
    for (let i = 0; i < 4; i++) {
        const buttonNum = i + 1;
        setElementValue(`button${buttonNum}-short`, config.buttons?.[`button${buttonNum}`]?.shortAction || '');
        setElementValue(`button${buttonNum}-long`, config.buttons?.[`button${buttonNum}`]?.longAction || '');
        
        // MQTT topic fields (optional)
        setElementValue(`button${buttonNum}-short-mqtt-topic`, config.buttons?.[`button${buttonNum}`]?.shortMqttTopic || '');
        setElementValue(`button${buttonNum}-long-mqtt-topic`, config.buttons?.[`button${buttonNum}`]?.longMqttTopic || '');
    }
    
    // Update next scheduled time display
    updateNextScheduledDisplay(config.status?.unbiddenInk?.nextScheduled);
}

/**
 * Collect all form data into a configuration object
 * @returns {Object} Complete configuration object from form data
 */
function collectFormData() {
    const formData = {
        // Device configuration
        device: {
            owner: getElementValue('device-owner'),
            timezone: getElementValue('timezone')
        },
        // WiFi configuration
        wifi: {
            ssid: getElementValue('wifi-ssid'),
            password: getElementValue('wifi-password'),
            connect_timeout: getElementIntValue('wifi-timeout', 15) * 1000 // Convert to milliseconds
        },
        // MQTT configuration
        mqtt: {
            server: getElementValue('mqtt-server'),
            port: getElementIntValue('mqtt-port', 1883),
            username: getElementValue('mqtt-username'),
            password: getElementValue('mqtt-password')
        },
        // Validation configuration
        validation: {
            maxCharacters: getElementIntValue('max-characters', 500)
        },
        // APIs configuration
        apis: {
            chatgptApiToken: getElementValue('chatgpt-api-token')
        },
        // Unbidden Ink configuration
        unbiddenInk: {
            enabled: getElementChecked('unbidden-ink-enabled'),
            startHour: getElementIntValue('time-start', 8),
            endHour: getElementIntValue('time-end', 22),
            frequencyMinutes: getElementIntValue('frequency-minutes', 60),
            prompt: getElementValue('unbidden-ink-prompt')
        },
        // Button configuration
        buttons: {}
    };
    
    // Collect button configurations
    for (let i = 0; i < 4; i++) {
        const buttonNum = i + 1;
        const buttonKey = `button${buttonNum}`;
        
        formData.buttons[buttonKey] = {
            shortAction: getElementValue(`button${buttonNum}-short`),
            longAction: getElementValue(`button${buttonNum}-long`),
            shortMqttTopic: getElementValue(`button${buttonNum}-short-mqtt-topic`),
            longMqttTopic: getElementValue(`button${buttonNum}-long-mqtt-topic`)
        };
    }
    
    // LED configuration (from LED module)
    if (window.SettingsLED && window.SettingsLED.collectLedConfig) {
        formData.leds = window.SettingsLED.collectLedConfig();
    } else {
        // Fallback basic LED config
        formData.leds = {
            pin: getElementIntValue('led-pin', 4),
            count: getElementIntValue('led-count', 30),
            brightness: getElementIntValue('led-brightness', 64),
            refreshRate: getElementIntValue('led-refresh-rate', 60)
        };
    }
    
    return formData;
}

/**
 * Show/hide settings sections for navigation
 * @param {string} sectionName - Name of section to show
 */
function showSettingsSection(sectionName) {
    // Hide all sections
    const sections = document.querySelectorAll('.settings-section');
    sections.forEach(section => {
        section.classList.add('hidden');
    });
    
    // Show the selected section
    const targetSection = document.getElementById(`${sectionName}-section`);
    if (targetSection) {
        targetSection.classList.remove('hidden');
    }
    
    // Update navigation buttons
    const navButtons = document.querySelectorAll('.section-nav-btn');
    navButtons.forEach(button => {
        button.classList.remove('active');
    });
    
    // Find and activate the correct nav button
    const activeButton = document.querySelector(`[onclick="showSettingsSection('${sectionName}')"]`);
    if (activeButton) {
        activeButton.classList.add('active');
    }
}

/**
 * Hide loading state and show settings form
 */
function hideLoadingState() {
    const loadingState = document.getElementById('loading-state');
    const settingsNavigation = document.getElementById('settings-navigation');
    const settingsForm = document.getElementById('settings-form');
    
    if (loadingState) {
        loadingState.style.display = 'none';
    }
    
    if (settingsNavigation) {
        settingsNavigation.classList.remove('hidden');
    }
    
    if (settingsForm) {
        settingsForm.classList.remove('hidden');
        // Show the first section by default
        showSettingsSection('wifi');
    }
}

/**
 * Display a message to the user using toast notifications
 * @param {string} message - Message to display
 * @param {string} type - Message type ('success', 'error', 'warning', 'info')
 */
function showMessage(message, type) {
    // Use global toast function if available
    if (window.showToast) {
        window.showToast(message, type);
        return;
    }
    
    // Fallback to simple alert
    if (type === 'error') {
        alert('Error: ' + message);
    } else if (type === 'success') {
        console.log('Success: ' + message);
        // Could show a temporary success div here
    } else {
        console.log(type + ': ' + message);
    }
}

/**
 * Toggle Unbidden Ink settings visibility
 */
function toggleUnbiddenInkSettings() {
    const checkbox = document.getElementById('unbidden-ink-enabled');
    const settings = document.getElementById('unbidden-ink-settings');
    
    if (checkbox && settings) {
        if (checkbox.checked) {
            settings.classList.remove('hidden');
        } else {
            settings.classList.add('hidden');
        }
    }
}

/**
 * Update time range display from sliders
 */
function updateTimeRange() {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    const display = document.getElementById('time-range-display');
    
    if (startSlider && endSlider && display) {
        const startHour = parseInt(startSlider.value);
        const endHour = parseInt(endSlider.value);
        
        const startTime = formatTime(startHour);
        const endTime = formatTime(endHour);
        
        display.textContent = `${startTime} - ${endTime}`;
    }
}

/**
 * Update next scheduled display
 * @param {string} nextScheduled - ISO timestamp of next scheduled run
 */
function updateNextScheduledDisplay(nextScheduled) {
    const display = document.getElementById('next-scheduled-display');
    if (display && nextScheduled) {
        const nextDate = new Date(nextScheduled);
        display.textContent = `Next: ${nextDate.toLocaleString()}`;
    }
}

/**
 * Go back to main page
 */
function goBack() {
    window.location.href = '/';
}

// =============================================================================
// UTILITY HELPER FUNCTIONS
// =============================================================================

/**
 * Safely set element value with fallback
 * @param {string} id - Element ID
 * @param {*} value - Value to set
 */
function setElementValue(id, value) {
    const element = document.getElementById(id);
    if (element) {
        element.value = value || '';
    }
}

/**
 * Safely set element checked state
 * @param {string} id - Element ID
 * @param {boolean} checked - Checked state
 */
function setElementChecked(id, checked) {
    const element = document.getElementById(id);
    if (element) {
        element.checked = !!checked;
    }
}

/**
 * Safely get element value with fallback
 * @param {string} id - Element ID
 * @param {string} fallback - Fallback value
 * @returns {string} Element value or fallback
 */
function getElementValue(id, fallback = '') {
    const element = document.getElementById(id);
    return element ? (element.value || fallback) : fallback;
}

/**
 * Safely get element integer value with fallback
 * @param {string} id - Element ID
 * @param {number} fallback - Fallback value
 * @returns {number} Element integer value or fallback
 */
function getElementIntValue(id, fallback) {
    const element = document.getElementById(id);
    if (!element) return fallback;
    const value = parseInt(element.value);
    return isNaN(value) ? fallback : value;
}

/**
 * Safely get element checked state
 * @param {string} id - Element ID
 * @returns {boolean} Element checked state
 */
function getElementChecked(id) {
    const element = document.getElementById(id);
    return element ? element.checked : false;
}

/**
 * Format hour (0-23) as 12-hour time string
 * @param {number} hour - Hour in 24-hour format
 * @returns {string} Formatted time string
 */
function formatTime(hour) {
    const period = hour >= 12 ? 'PM' : 'AM';
    const displayHour = hour === 0 ? 12 : (hour > 12 ? hour - 12 : hour);
    return `${displayHour}:00 ${period}`;
}

// Export UI module
window.SettingsUI = {
    populateForm,
    collectFormData,
    showSettingsSection,
    hideLoadingState,
    showMessage,
    toggleUnbiddenInkSettings,
    updateTimeRange,
    updateNextScheduledDisplay,
    goBack
};
