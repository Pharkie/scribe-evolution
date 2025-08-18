/**
 * @file settings-core.js
 * @brief Core settings functionality - loading, saving, and form management
 * @description Main coordination module for settings page functionality
 */

let currentConfig = {};

/**
 * Load configuration directly from API
 */
async function loadConfiguration() {
    try {
        console.log('Settings: Loading configuration from API...');
        
        const response = await fetch('/api/config');
        if (!response.ok) {
            throw new Error(`Config API returned ${response.status}: ${response.statusText}`);
        }
        
        const config = await response.json();
        console.log('Settings: Configuration loaded successfully');
        
        // Populate the form with the loaded configuration
        populateForm(config);
        
        return config;
    } catch (error) {
        console.error('Settings: Failed to load configuration:', error);
        showMessage('Failed to load settings: ' + error.message, 'error');
        throw error;
    }
}

/**
 * Populate all form sections with current configuration
 * @param {Object} config - Configuration object from server
 */
function populateForm(config) {
    // Device configuration
    document.getElementById('device-owner').value = config.device?.owner || '';
    document.getElementById('timezone').value = config.device?.timezone || '';
    
    // WiFi configuration
    document.getElementById('wifi-ssid').value = config.wifi?.ssid || '';
    document.getElementById('wifi-password').value = config.wifi?.password || '';
    document.getElementById('wifi-timeout').value = (config.wifi?.connect_timeout || 15000) / 1000; // Convert from milliseconds
    
    // MQTT configuration
    document.getElementById('mqtt-enabled').checked = config.mqtt?.enabled || false;
    document.getElementById('mqtt-server').value = config.mqtt?.server || '';
    document.getElementById('mqtt-port').value = config.mqtt?.port || 1883;
    document.getElementById('mqtt-username').value = config.mqtt?.username || '';
    document.getElementById('mqtt-password').value = config.mqtt?.password || '';
    
    // Validation configuration
    document.getElementById('max-characters').value = config.validation?.maxCharacters || 500;
    
    // ChatGPT configuration
    document.getElementById('chatgpt-api-token').value = config.chatgpt?.apiToken || '';
    
    // Unbidden Ink configuration
    const unbiddenInkEnabled = document.getElementById('unbidden-ink-enabled');
    if (unbiddenInkEnabled) {
        unbiddenInkEnabled.checked = config.unbiddenInk?.enabled || false;
    }
    
    // Time range sliders
    const timeStart = document.getElementById('time-start');
    const timeEnd = document.getElementById('time-end');
    if (timeStart && timeEnd) {
        timeStart.value = config.unbiddenInk?.startHour || 8;
        timeEnd.value = config.unbiddenInk?.endHour || 22;
        // Update the visual display
        if (typeof updateTimeRange === 'function') {
            updateTimeRange();
        }
    }
    
    // Frequency and prompt
    document.getElementById('frequency-minutes').value = config.unbiddenInk?.frequencyMinutes || 60;
    document.getElementById('unbidden-ink-prompt').value = config.unbiddenInk?.prompt || '';
    
    // Button configuration - with proper error handling
    for (let i = 0; i < 4; i++) {
        const buttonNum = i + 1;
        const shortSelect = document.getElementById(`button${buttonNum}-short`);
        const longSelect = document.getElementById(`button${buttonNum}-long`);
        
        if (shortSelect) {
            shortSelect.value = config.buttons?.[`button${buttonNum}`]?.shortAction || '';
        }
        if (longSelect) {
            longSelect.value = config.buttons?.[`button${buttonNum}`]?.longAction || '';
        }
        
        // MQTT topic fields might not exist in current HTML - check first
        const shortMqttField = document.getElementById(`button${buttonNum}-short-mqtt-topic`);
        const longMqttField = document.getElementById(`button${buttonNum}-long-mqtt-topic`);
        if (shortMqttField) {
            shortMqttField.value = config.buttons?.[`button${buttonNum}`]?.shortMqttTopic || '';
        }
        if (longMqttField) {
            longMqttField.value = config.buttons?.[`button${buttonNum}`]?.longMqttTopic || '';
        }
    }
    
    // LED configuration (handled by LED module)
    if (window.LEDConfig && window.LEDConfig.populateLedForm) {
        window.LEDConfig.populateLedForm(config);
    }
    
    // Update next scheduled time from config data
    updateNextScheduledDisplay(config.status?.unbiddenInk?.nextScheduled);
}

/**
 * Collect form data and build configuration object - all user-configurable settings
 */
function collectFormData() {
    const formData = {
        // Device configuration (overrides config.h)
        device: {
            owner: document.getElementById('device-owner').value,
            timezone: document.getElementById('timezone').value
        },
        // WiFi configuration (overrides config.h)
        wifi: {
            ssid: document.getElementById('wifi-ssid').value,
            password: document.getElementById('wifi-password').value,
            connect_timeout: parseInt(document.getElementById('wifi-timeout').value) * 1000 // Convert to milliseconds
        },
        // MQTT configuration (overrides config.h)
        mqtt: {
            enabled: document.getElementById('mqtt-enabled').checked,
            server: document.getElementById('mqtt-server').value,
            port: parseInt(document.getElementById('mqtt-port').value),
            username: document.getElementById('mqtt-username').value,
            password: document.getElementById('mqtt-password').value
        },
        // Validation configuration (overrides config.h)
        validation: {
            maxCharacters: parseInt(document.getElementById('max-characters').value)
        },
        // ChatGPT configuration (overrides config.h)
        chatgpt: {
            apiToken: document.getElementById('chatgpt-api-token').value
        },
        // Unbidden Ink configuration
        unbiddenInk: {
            enabled: document.getElementById('unbidden-ink-enabled').checked,
            startHour: parseInt(document.getElementById('time-start').value),
            endHour: parseInt(document.getElementById('time-end').value),
            frequencyMinutes: parseInt(document.getElementById('frequency-minutes').value),
            prompt: document.getElementById('unbidden-ink-prompt').value // Fixed ID
        },
        // Button configuration
        buttons: {}
    };
    
    // Collect button configurations
    for (let i = 0; i < 4; i++) {
        const buttonNum = i + 1;
        const buttonKey = `button${buttonNum}`;
        const shortSelect = document.getElementById(`button${buttonNum}-short`);
        const longSelect = document.getElementById(`button${buttonNum}-long`);
        
        formData.buttons[buttonKey] = {
            shortAction: shortSelect ? shortSelect.value : '',
            longAction: longSelect ? longSelect.value : '',
            shortMqttTopic: '', // Not in current HTML form
            longMqttTopic: ''   // Not in current HTML form
        };
    }
    
    // LED configuration (handled by LED module)
    if (window.LEDConfig && window.LEDConfig.collectLedFormData) {
        formData.leds = window.LEDConfig.collectLedFormData();
    }
    
    return formData;
}

/**
 * Save settings to server
 */
async function saveSettings(event) {
    event.preventDefault();
    
    try {
        const configData = collectFormData();
        
        // Client-side validation
        if (window.LEDConfig && window.LEDConfig.validateLedConfig) {
            const ledValidation = window.LEDConfig.validateLedConfig(configData.leds);
            if (!ledValidation.isValid) {
                showMessage(ledValidation.error, 'error');
                return;
            }
        }
        
        const response = await fetch('/api/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(configData)
        });
        
        if (response.ok) {
            const result = await response.text();
            showMessage('Settings saved successfully!', 'success');
            
            // Update current config with saved data
            currentConfig = configData;
        } else {
            const errorText = await response.text();
            throw new Error(`Server error: ${response.status} - ${errorText}`);
        }
    } catch (error) {
        console.error('Settings: Failed to save configuration:', error);
        showMessage('Failed to save settings: ' + error.message, 'error');
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

// Make core functions available globally
window.SettingsCore = {
    loadConfiguration,
    populateForm,
    collectFormData,
    saveSettings,
    showMessage,
    getCurrentConfig: () => currentConfig
};
