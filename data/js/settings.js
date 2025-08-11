/**
 * @file settings.js
 * @brief JavaScript for the settings page
 */

let currentConfig = {};

/**
 * Load configuration - wait for global config to be available
 */
async function loadConfiguration() {
    try {
        let config;
        
        // Wait for global config to be loaded (with timeout)
        let attempts = 0;
        const maxAttempts = 50; // 5 seconds total
        
        while ((!window.GLOBAL_CONFIG || Object.keys(window.GLOBAL_CONFIG).length === 0) && attempts < maxAttempts) {
            await new Promise(resolve => setTimeout(resolve, 100));
            attempts++;
        }
        
        if (!window.GLOBAL_CONFIG || Object.keys(window.GLOBAL_CONFIG).length === 0) {
            throw new Error('Global config not loaded within timeout');
        }
        
        config = window.GLOBAL_CONFIG;
        currentConfig = config;
        populateForm(config);
        
        // Hide loading state and show form
        document.getElementById('loading-state').classList.add('hidden');
        document.getElementById('settings-form').classList.remove('hidden');
        
    } catch (error) {
        console.error('Error loading configuration:', error);
        showMessage('Failed to load configuration. Please refresh the page.', 'error');
    }
}

/**
 * Populate form fields with configuration data
 */
function populateForm(config) {
    // WiFi Configuration (new)
    document.getElementById('wifi-ssid').value = config.wifi?.ssid || '';
    document.getElementById('wifi-password').value = config.wifi?.password || '';
    
    // Device Configuration (new)
    document.getElementById('device-owner').value = config.device?.owner || '';
    document.getElementById('timezone').value = config.device?.timezone || '';
    
    // MQTT Configuration
    document.getElementById('mqtt-server').value = config.mqtt?.server || '';
    document.getElementById('mqtt-port').value = config.mqtt?.port || 8883;
    document.getElementById('mqtt-username').value = config.mqtt?.username || '';
    document.getElementById('mqtt-password').value = config.mqtt?.password || '';
    
    // API Configuration - only chatgptApiToken now
    document.getElementById('chatgpt-api-token').value = config.apis?.chatgptApiToken || '';
    
    // Validation Configuration (only maxCharacters)
    document.getElementById('max-characters').value = config.validation?.maxCharacters || 1000;
    
    // Unbidden Ink Configuration - check for the new slider/preset interface
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
    
    // Frequency - set the hidden input and update presets
    const frequencyInput = document.getElementById('frequency-minutes');
    if (frequencyInput) {
        frequencyInput.value = config.unbiddenInk?.frequencyMinutes || 60;
        // Update the visual frequency display
        if (typeof updateFrequencyDisplay === 'function') {
            updateFrequencyDisplay();
        }
    }
    
    // Unbidden Ink prompt
    const unbiddenInkPrompt = document.getElementById('unbidden-ink-prompt');
    if (unbiddenInkPrompt) {
        unbiddenInkPrompt.value = config.unbiddenInk?.prompt || '';
    }
    
    // Update visibility of Unbidden Ink settings based on enabled state
    toggleUnbiddenInkSettings();
    
    // Button Configuration
    document.getElementById('button1-short').value = config.buttons?.button1?.shortAction || '/joke';
    document.getElementById('button1-long').value = config.buttons?.button1?.longAction || '';
    document.getElementById('button2-short').value = config.buttons?.button2?.shortAction || '/riddle';
    document.getElementById('button2-long').value = config.buttons?.button2?.longAction || '';
    document.getElementById('button3-short').value = config.buttons?.button3?.shortAction || '/quote';
    document.getElementById('button3-long').value = config.buttons?.button3?.longAction || '';
    document.getElementById('button4-short').value = config.buttons?.button4?.shortAction || '/quiz';
    document.getElementById('button4-long').value = config.buttons?.button4?.longAction || '';
}

/**
 * Collect form data and build configuration object - all user-configurable settings
 */
function collectFormData() {
    return {
        // Device configuration (overrides config.h)
        device: {
            owner: document.getElementById('device-owner').value,
            timezone: document.getElementById('timezone').value
        },
        // WiFi configuration (overrides config.h)
        wifi: {
            ssid: document.getElementById('wifi-ssid').value,
            password: document.getElementById('wifi-password').value
        },
        // MQTT configuration (overrides config.h)
        mqtt: {
            server: document.getElementById('mqtt-server').value,
            port: parseInt(document.getElementById('mqtt-port').value),
            username: document.getElementById('mqtt-username').value,
            password: document.getElementById('mqtt-password').value
        },
        // API configuration - include all API fields from loaded config, updating only user-editable ones
        apis: {
            ...g_runtimeConfig.apis, // Include all existing API config
            chatgptApiToken: document.getElementById('chatgpt-api-token').value // Only update user-editable field
        },
        // Validation settings
        validation: {
            maxCharacters: parseInt(document.getElementById('max-characters').value)
        },
        // Unbidden Ink configuration
        unbiddenInk: {
            enabled: document.getElementById('unbidden-ink-enabled').checked,
            startHour: parseInt(document.getElementById('time-start')?.value || document.getElementById('start-hour')?.value || 8),
            endHour: parseInt(document.getElementById('time-end')?.value || document.getElementById('end-hour')?.value || 22),
            frequencyMinutes: parseInt(document.getElementById('frequency-minutes').value),
            prompt: document.getElementById('unbidden-ink-prompt').value
        },
        // Button configuration
        buttons: {
            button1: {
                shortAction: document.getElementById('button1-short').value,
                longAction: document.getElementById('button1-long').value
            },
            button2: {
                shortAction: document.getElementById('button2-short').value,
                longAction: document.getElementById('button2-long').value
            },
            button3: {
                shortAction: document.getElementById('button3-short').value,
                longAction: document.getElementById('button3-long').value
            },
            button4: {
                shortAction: document.getElementById('button4-short').value,
                longAction: document.getElementById('button4-long').value
            }
        }
    };
}

/**
 * Save settings to server
 */
async function saveSettings(event) {
    event.preventDefault();
    
    // Show loading spinner
    document.getElementById('save-loading').classList.remove('hidden');
    
    try {
        const configData = collectFormData();
        
        const response = await fetch('/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(configData)
        });
        
        const result = await response.json();
        
        if (response.ok && result.success) {
            showMessage('Settings saved successfully!', 'success');
            currentConfig = configData; // Update current config
        } else {
            throw new Error(result.error || 'Failed to save settings');
        }
        
    } catch (error) {
        console.error('Error saving settings:', error);
        showMessage(`Failed to save settings: ${error.message}`, 'error');
    } finally {
        // Hide loading spinner
        document.getElementById('save-loading').classList.add('hidden');
    }
}

/**
 * Show success or error message
 */
function showMessage(message, type) {
    const container = document.getElementById('message-container');
    
    const messageDiv = document.createElement('div');
    messageDiv.className = `p-4 rounded-lg shadow-lg mb-4 transition-all duration-300 ${
        type === 'success' 
            ? 'bg-green-100 text-green-800 border border-green-200' 
            : 'bg-red-100 text-red-800 border border-red-200'
    }`;
    
    messageDiv.innerHTML = `
        <div class="flex items-center space-x-2">
            <span class="text-xl">${type === 'success' ? '✅' : '❌'}</span>
            <span>${message}</span>
            <button onclick="this.parentElement.parentElement.remove()" class="ml-auto text-lg font-bold opacity-50 hover:opacity-100">×</button>
        </div>
    `;
    
    container.appendChild(messageDiv);
    
    // Auto-remove after 5 seconds
    setTimeout(() => {
        if (messageDiv.parentElement) {
            messageDiv.remove();
        }
    }, 5000);
}

/**
 * Toggle visibility of Unbidden Ink settings based on enable checkbox
 */
function toggleUnbiddenInkSettings() {
    const enabled = document.getElementById('unbidden-ink-enabled').checked;
    const settingsSection = document.getElementById('unbidden-ink-settings');
    
    if (!settingsSection) {
        console.warn('unbidden-ink-settings element not found');
        return;
    }
    
    if (enabled) {
        settingsSection.classList.remove('hidden');
        // Remove disabled attribute from inputs when enabled
        settingsSection.querySelectorAll('input, textarea').forEach(input => {
            input.removeAttribute('disabled');
        });
    } else {
        settingsSection.classList.add('hidden');
        // Disable inputs when hidden to prevent validation issues
        settingsSection.querySelectorAll('input, textarea').forEach(input => {
            input.setAttribute('disabled', 'disabled');
        });
    }
}

/**
 * Go back to homepage
 */
function goBack() {
    window.location.href = '/';
}

/**
 * Initialize the page when DOM is loaded
 */
document.addEventListener('DOMContentLoaded', function() {
    loadConfiguration();
});

// ========================================
// UNBIDDEN INK INTERFACE FUNCTIONS
// ========================================

/**
 * Update time range display and handle dual-slider constraints
 */
function updateTimeRange(slider, type) {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    
    if (!startSlider || !endSlider) return;
    
    let startVal = parseInt(startSlider.value);
    let endVal = parseInt(endSlider.value);
    
    // Handle collision prevention with proper logic
    if (type === 'start') {
        // If user is moving start slider and it would collide with end
        if (startVal >= endVal) {
            // Only adjust if we're at the boundary - try to move end first
            if (endVal < 23) {
                endVal = startVal + 1;
                endSlider.value = endVal;
            } else {
                // If end can't move further, constrain start
                startVal = endVal - 1;
                startSlider.value = startVal;
            }
        }
    } else if (type === 'end') {
        // If user is moving end slider and it would collide with start
        if (endVal <= startVal) {
            // Only adjust if we're at the boundary - try to move start first
            if (startVal > 0) {
                startVal = endVal - 1;
                startSlider.value = startVal;
            } else {
                // If start can't move further, constrain end
                endVal = startVal + 1;
                endSlider.value = endVal;
            }
        }
    }
    
    // Get final values after any adjustments
    startVal = parseInt(startSlider.value);
    endVal = parseInt(endSlider.value);
    
    // Update visual track
    const track = document.getElementById('time-track');
    if (track) {
        const startPercent = (startVal / 23) * 100;
        const endPercent = (endVal / 23) * 100;
        track.style.left = startPercent + '%';
        track.style.width = (endPercent - startPercent) + '%';
    }
    
    // Update time displays
    const startDisplay = document.getElementById('time-display-start');
    const endDisplay = document.getElementById('time-display-end');
    if (startDisplay) startDisplay.textContent = String(startVal).padStart(2, '0') + ':00';
    if (endDisplay) endDisplay.textContent = String(endVal).padStart(2, '0') + ':00';
    
    // Update frequency display to include new time range
    if (typeof updateFrequencyDisplay === 'function') {
        updateFrequencyDisplay();
    }
}

/**
 * Set frequency from preset buttons
 */
function setFrequency(minutes) {
    const input = document.getElementById('frequency-minutes');
    if (input) {
        input.value = minutes;
        updateFrequencyDisplay();
        
        // Update button styling
        document.querySelectorAll('.freq-preset').forEach(btn => {
            btn.classList.remove('bg-purple-100', 'border-purple-400', 'text-purple-800', 'dark:bg-purple-900/30', 'dark:text-purple-300', 'dark:border-purple-600');
            btn.classList.add('border-gray-300', 'dark:border-gray-600');
        });
        
        const activeBtn = document.querySelector(`[data-value="${minutes}"]`);
        if (activeBtn) {
            activeBtn.classList.remove('border-gray-300', 'dark:border-gray-600');
            activeBtn.classList.add('bg-purple-100', 'border-purple-400', 'text-purple-800', 'dark:bg-purple-900/30', 'dark:text-purple-300', 'dark:border-purple-600');
        }
    }
}

/**
 * Update frequency display text
 */
function updateFrequencyDisplay() {
    const input = document.getElementById('frequency-minutes');
    const display = document.getElementById('frequency-display');
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    
    if (!input || !display || !startSlider || !endSlider) return;
    
    const minutes = parseInt(input.value);
    const startHour = parseInt(startSlider.value);
    const endHour = parseInt(endSlider.value);
    
    // Format hours for display
    const formatHour = (hour) => {
        if (hour === 0) return '12 am';
        if (hour < 12) return `${hour} am`;
        if (hour === 12) return '12 pm';
        return `${hour - 12} pm`;
    };
    
    const startTime = formatHour(startHour);
    const endTime = formatHour(endHour);
    
    let text;
    
    if (minutes < 60) {
        text = `Around every ${minutes} minutes from ${startTime} to ${endTime} each day`;
    } else {
        const hours = minutes / 60;
        if (hours === 1) {
            text = `Around once per hour from ${startTime} to ${endTime} each day`;
        } else if (hours % 1 === 0) {
            text = `Around once every ${hours} hours from ${startTime} to ${endTime} each day`;
        } else {
            text = `Around once every ${hours} hours from ${startTime} to ${endTime} each day`;
        }
    }
    
    display.textContent = text;
}

/**
 * Set prompt from preset buttons
 */
function setPrompt(promptText) {
    const textarea = document.getElementById('unbidden-ink-prompt');
    if (textarea) {
        textarea.value = promptText;
        // Trigger any change events
        textarea.dispatchEvent(new Event('input'));
    }
}