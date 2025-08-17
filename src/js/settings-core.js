/**
 * @file settings-core.js
 * @brief Core settings functionality - loading, saving, and form management
 * @description Main coordination module for settings page functionality
 */

let currentConfig = {};

/**
 * Load configuration - wait for global config to be available
 */
async function loadConfiguration() {
    try {
        let config;
        
        // Wait for global config to be loaded using event listener (more reliable)
        if (!window.GLOBAL_CONFIG || Object.keys(window.GLOBAL_CONFIG).length === 0) {
            console.log('Waiting for global config to load...');
            
            // Use event-based waiting with fallback timeout
            config = await new Promise((resolve, reject) => {
                const timeout = setTimeout(() => {
                    reject(new Error('Global config load timeout'));
                }, 10000); // 10 second timeout
                
                const checkConfig = () => {
                    if (window.GLOBAL_CONFIG && Object.keys(window.GLOBAL_CONFIG).length > 0) {
                        clearTimeout(timeout);
                        document.removeEventListener('configLoaded', checkConfig);
                        resolve(window.GLOBAL_CONFIG);
                    }
                };
                
                document.addEventListener('configLoaded', checkConfig);
                
                // Check immediately in case event already fired
                checkConfig();
            });
        } else {
            config = window.GLOBAL_CONFIG;
        }
        
        console.log('Settings: Configuration loaded successfully');
        currentConfig = config;
        
        // Populate form with loaded configuration
        populateForm(config);
        
    } catch (error) {
        console.error('Settings: Failed to load configuration:', error);
        showMessage('Failed to load configuration: ' + error.message, 'error');
    }
}

/**
 * Populate all form sections with current configuration
 * @param {Object} config - Configuration object from server
 */
function populateForm(config) {
    // Device configuration
    document.getElementById('device-owner').value = config.device?.owner || '';
    document.getElementById('timezone').value = config.device?.timezone || 'UTC';
    
    // WiFi configuration
    document.getElementById('wifi-ssid').value = config.wifi?.ssid || '';
    document.getElementById('wifi-password').value = config.wifi?.password || '';
    document.getElementById('wifi-timeout').value = (config.wifi?.connect_timeout || 30000) / 1000;
    
    // MQTT configuration
    document.getElementById('mqtt-enabled').checked = config.mqtt?.enabled || false;
    document.getElementById('mqtt-server').value = config.mqtt?.server || '';
    document.getElementById('mqtt-port').value = config.mqtt?.port || 1883;
    document.getElementById('mqtt-username').value = config.mqtt?.username || '';
    document.getElementById('mqtt-password').value = config.mqtt?.password || '';
    document.getElementById('mqtt-device-topic').value = config.mqtt?.deviceTopic || '';
    
    // Validation configuration
    document.getElementById('max-characters').value = config.validation?.maxCharacters || 500;
    
    // ChatGPT configuration
    document.getElementById('chatgpt-api-token').value = config.chatgpt?.apiToken || '';
    document.getElementById('chatgpt-api-endpoint').value = config.chatgpt?.apiEndpoint || 'https://api.openai.com/v1/chat/completions';
    
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
    document.getElementById('unbidden-prompt').value = config.unbiddenInk?.prompt || '';
    
    // Button configuration
    for (let i = 0; i < 4; i++) {
        document.getElementById(`button-${i+1}-short-action`).value = config.buttons?.[`button${i+1}`]?.shortAction || '';
        document.getElementById(`button-${i+1}-long-action`).value = config.buttons?.[`button${i+1}`]?.longAction || '';
        document.getElementById(`button-${i+1}-short-mqtt-topic`).value = config.buttons?.[`button${i+1}`]?.shortMqttTopic || '';
        document.getElementById(`button-${i+1}-long-mqtt-topic`).value = config.buttons?.[`button${i+1}`]?.longMqttTopic || '';
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
            password: document.getElementById('mqtt-password').value,
            deviceTopic: document.getElementById('mqtt-device-topic').value
        },
        // Validation configuration (overrides config.h)
        validation: {
            maxCharacters: parseInt(document.getElementById('max-characters').value)
        },
        // ChatGPT configuration (overrides config.h)
        chatgpt: {
            apiToken: document.getElementById('chatgpt-api-token').value,
            apiEndpoint: document.getElementById('chatgpt-api-endpoint').value
        },
        // Unbidden Ink configuration
        unbiddenInk: {
            enabled: document.getElementById('unbidden-ink-enabled').checked,
            startHour: parseInt(document.getElementById('time-start').value),
            endHour: parseInt(document.getElementById('time-end').value),
            frequencyMinutes: parseInt(document.getElementById('frequency-minutes').value),
            prompt: document.getElementById('unbidden-prompt').value
        },
        // Button configuration
        buttons: {}
    };
    
    // Collect button configurations
    for (let i = 0; i < 4; i++) {
        const buttonKey = `button${i+1}`;
        formData.buttons[buttonKey] = {
            shortAction: document.getElementById(`button-${i+1}-short-action`).value,
            longAction: document.getElementById(`button-${i+1}-long-action`).value,
            shortMqttTopic: document.getElementById(`button-${i+1}-short-mqtt-topic`).value,
            longMqttTopic: document.getElementById(`button-${i+1}-long-mqtt-topic`).value
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
