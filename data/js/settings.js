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
        
        // Wait for global config to be loaded using event listener (more reliable)
        if (!window.GLOBAL_CONFIG || Object.keys(window.GLOBAL_CONFIG).length === 0) {
            console.log('Waiting for global config to load...');
            
            // Use event-based waiting with fallback timeout
            config = await new Promise((resolve, reject) => {
                const timeout = setTimeout(() => {
                    reject(new Error('Global config not loaded within timeout - please refresh the page'));
                }, 10000); // 10 second timeout
                
                const handleConfigLoaded = (event) => {
                    clearTimeout(timeout);
                    window.removeEventListener('configLoaded', handleConfigLoaded);
                    resolve(event.detail);
                };
                
                // If config is already loaded, resolve immediately
                if (window.GLOBAL_CONFIG && Object.keys(window.GLOBAL_CONFIG).length > 0) {
                    clearTimeout(timeout);
                    resolve(window.GLOBAL_CONFIG);
                    return;
                }
                
                // Otherwise wait for the event
                window.addEventListener('configLoaded', handleConfigLoaded);
            });
        } else {
            config = window.GLOBAL_CONFIG;
        }
        
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
    document.getElementById('wifi-timeout').value = config.wifi?.connect_timeout ? (config.wifi.connect_timeout / 1000) : 15;
    
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
        // Initialize click areas after a short delay to ensure DOM is ready
        setTimeout(() => {
            if (typeof updateClickAreas === 'function') {
                updateClickAreas();
            }
        }, 100);
    }
    
    // Frequency - set the hidden input and update slider
    const frequencyInput = document.getElementById('frequency-minutes');
    if (frequencyInput) {
        const frequencyValue = config.unbiddenInk?.frequencyMinutes || 60;
        frequencyInput.value = frequencyValue;
        
        // Update the slider position
        if (typeof updateSliderFromFrequency === 'function') {
            updateSliderFromFrequency(frequencyValue);
        }
        
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
            password: document.getElementById('wifi-password').value,
            connect_timeout: parseInt(document.getElementById('wifi-timeout').value) * 1000 // Convert to milliseconds
        },
        // MQTT configuration (overrides config.h)
        mqtt: {
            server: document.getElementById('mqtt-server').value,
            port: parseInt(document.getElementById('mqtt-port').value),
            username: document.getElementById('mqtt-username').value,
            password: document.getElementById('mqtt-password').value
        },
        // API configuration - only user-configurable fields
        apis: {
            chatgptApiToken: document.getElementById('chatgpt-api-token').value
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
    setupAPModeUI();
});

/**
 * Detect AP mode and adjust UI accordingly
 */
function setupAPModeUI() {
    // Detect if we're in AP mode by checking the hostname
    const isAPMode = window.location.hostname === '192.168.4.1' || window.location.hostname === 'scribe-setup';
    
    if (isAPMode) {
        console.log('AP mode detected - adjusting UI');
        
        // Change save button text to indicate reboot
        const saveButton = document.getElementById('save-button');
        if (saveButton) {
            saveButton.textContent = 'Save and Reboot';
        }
        
        // Hide close button in AP mode
        const closeButton = document.getElementById('close-button');
        if (closeButton) {
            closeButton.style.display = 'none';
        }
        
        // Make save button full width when close button is hidden
        if (saveButton) {
            saveButton.classList.remove('flex-1');
            saveButton.classList.add('w-full');
        }
    }
}

// ========================================
// UNBIDDEN INK INTERFACE FUNCTIONS
// ========================================

/**
 * Update time range display and handle dual-slider constraints
 */
/**
 * Update click areas around each handle to prevent collisions
 */
function updateClickAreas() {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    const startArea = document.getElementById('click-area-start');
    const endArea = document.getElementById('click-area-end');
    
    if (!startSlider || !endSlider || !startArea || !endArea) return;
    
    const startVal = parseInt(startSlider.value);
    const endVal = parseInt(endSlider.value);
    const containerWidth = startSlider.offsetWidth;
    
    // Calculate positions as percentages for 24-hour scale (0-24)
    const startPercent = (startVal / 24) * 100;
    const endPercent = ((endVal + 1) / 24) * 100; // +1 because hour 23 represents 23:00-24:00
    
    // Define click area size (in percentage of total width)
    const clickAreaSize = 15; // 15% of total width for each click area
    
    // Calculate boundaries to prevent overlap
    const midPoint = (startPercent + endPercent) / 2;
    
    // Position start click area
    const startAreaLeft = Math.max(0, startPercent - clickAreaSize/2);
    const startAreaRight = Math.min(midPoint - 1, startPercent + clickAreaSize/2);
    
    startArea.style.left = startAreaLeft + '%';
    startArea.style.width = (startAreaRight - startAreaLeft) + '%';
    
    // Position end click area  
    const endAreaLeft = Math.max(midPoint + 1, endPercent - clickAreaSize/2);
    const endAreaRight = Math.min(100, endPercent + clickAreaSize/2);
    
    endArea.style.left = endAreaLeft + '%';
    endArea.style.width = (endAreaRight - endAreaLeft) + '%';
}

/**
 * Handle clicks on the dynamic click areas
 */
function handleSliderClick(event, type) {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    
    if (!startSlider || !endSlider) return;
    
    const rect = event.currentTarget.getBoundingClientRect();
    const clickX = event.clientX - rect.left;
    const clickPercent = (clickX / rect.width) * 100;
    
    // Convert click position to hour value
    const clickHour = Math.round((clickPercent / 100) * 24);
    
    if (type === 'start') {
        const newStartVal = Math.max(0, Math.min(24, clickHour));
        const endVal = parseInt(endSlider.value);
        
        // Collision avoidance: move other handle if necessary
        if (newStartVal >= endVal && !(newStartVal === 0 && endVal === 0)) {
            if (endVal < 24) {
                endSlider.value = newStartVal + 1;
            } else {
                startSlider.value = endVal - 1;
                updateTimeRange(startSlider, 'start');
                updateClickAreas();
                return;
            }
        }
        
        startSlider.value = newStartVal;
        updateTimeRange(startSlider, 'start');
    } else if (type === 'end') {
        const newEndVal = Math.max(0, Math.min(24, clickHour));
        const startVal = parseInt(startSlider.value);
        
        // Collision avoidance: move other handle if necessary
        if (newEndVal <= startVal && !(startVal === 0 && newEndVal === 0)) {
            if (startVal > 0) {
                startSlider.value = newEndVal - 1;
            } else {
                endSlider.value = startVal + 1;
                updateTimeRange(endSlider, 'end');
                updateClickAreas();
                return;
            }
        }
        
        endSlider.value = newEndVal;
        updateTimeRange(endSlider, 'end');
    }
    
    updateClickAreas();
}

/**
 * Update the time range display and handle collisions
 */
function updateTimeRange(slider, type) {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    
    if (!startSlider || !endSlider) return;
    
    let startVal = parseInt(startSlider.value);
    let endVal = parseInt(endSlider.value);
    
    // Special case: Allow 0-0 for full day operation (24 hours)
    if (startVal === 0 && endVal === 0) {
        // This is valid - full day operation
    } else {
        // Smart collision handling - move the other handle when possible
        if (type === 'start') {
            if (startVal >= endVal) {
                if (endVal < 24) {
                    // Move end handle forward
                    endVal = startVal + 1;
                    endSlider.value = endVal;
                } else {
                    // End can't move, constrain start
                    startVal = 23;
                    startSlider.value = startVal;
                }
            }
        } else if (type === 'end') {
            if (endVal <= startVal) {
                if (startVal > 0) {
                    // Move start handle backward
                    startVal = endVal - 1;
                    startSlider.value = startVal;
                } else {
                    // Start can't move, constrain end
                    endVal = 1;
                    endSlider.value = endVal;
                }
            }
        }
    }
    
    // Get final values after any adjustments
    startVal = parseInt(startSlider.value);
    endVal = parseInt(endSlider.value);
    
    // Update visual track with accurate positioning
    const track = document.getElementById('time-track');
    if (track) {
        if (startVal === 0 && endVal === 0) {
            // Full day operation - show full width
            track.style.left = '0%';
            track.style.width = '100%';
        } else {
            // Calculate percentages based on where the slider thumbs actually appear
            // Slider thumbs appear at positions based on their value/max ratio
            const startPercent = (startVal / 24) * 100;
            const endPercent = (endVal / 24) * 100;
            
            track.style.left = startPercent + '%';
            track.style.width = (endPercent - startPercent) + '%';
        }
    }
    
    // Update time displays
    const startDisplay = document.getElementById('time-display-start');
    const endDisplay = document.getElementById('time-display-end');
    
    // Format hour 24 as 00:00 (midnight of next day)
    const formatHour = (hour) => {
        if (hour === 24) return '00:00';
        return String(hour).padStart(2, '0') + ':00';
    };
    
    if (startDisplay) startDisplay.textContent = formatHour(startVal);
    if (endDisplay) endDisplay.textContent = formatHour(endVal);
    
    // Update click areas after any changes
    updateClickAreas();
    
    // Update frequency display to include new time range
    if (typeof updateFrequencyDisplay === 'function') {
        updateFrequencyDisplay();
    }
}

/**
 * Update frequency from slider position (0-7 maps to frequency values)
 */
function updateFrequencyFromSlider(sliderValue) {
    // Map slider positions to frequency values
    const frequencyValues = [15, 30, 45, 60, 120, 240, 360, 480];
    const minutes = frequencyValues[parseInt(sliderValue)];
    
    const input = document.getElementById('frequency-minutes');
    if (input) {
        input.value = minutes;
        updateFrequencyDisplay();
    }
}

/**
 * Update slider position from frequency value
 */
function updateSliderFromFrequency(minutes) {
    const frequencyValues = [15, 30, 45, 60, 120, 240, 360, 480];
    const sliderPosition = frequencyValues.indexOf(parseInt(minutes));
    
    const slider = document.getElementById('frequency-slider');
    if (slider && sliderPosition !== -1) {
        slider.value = sliderPosition;
    }
}

/**
 * Set frequency from preset buttons (legacy - now updates slider too)
 */
function setFrequency(minutes) {
    const input = document.getElementById('frequency-minutes');
    if (input) {
        input.value = minutes;
        updateFrequencyDisplay();
        updateSliderFromFrequency(minutes);
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
        if (hour === 0 || hour === 24) return '12 am';  // Midnight
        if (hour < 12) return `${hour} am`;
        if (hour === 12) return '12 pm';  // Noon
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

/**
 * Bring a slider to the front so it can be interacted with
 */
function bringSliderToFront(slider) {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    
    if (!startSlider || !endSlider) return;
    
    // Reset both sliders to lower z-index
    startSlider.style.zIndex = '20';
    endSlider.style.zIndex = '20';
    
    // Bring the active slider to front
    slider.style.zIndex = '30';
}

/**
 * Test Unbidden Ink output
 */
async function testUnbiddenInk() {
    const button = event.target;
    const originalText = button.textContent;
    
    try {
        // Check if ChatGPT API token is configured
        const chatgptToken = document.getElementById('chatgpt-api-token').value.trim();
        if (!chatgptToken) {
            showMessage('Please configure your ChatGPT API Token first. You can get one from https://platform.openai.com/api-keys', 'error');
            return;
        }
        
        // Check if Unbidden Ink is enabled
        const unbiddenInkEnabled = document.getElementById('unbidden-ink-enabled').checked;
        if (!unbiddenInkEnabled) {
            showMessage('Please enable Unbidden Ink first before testing.', 'error');
            return;
        }
        
        // Get the current prompt from the textarea
        const currentPrompt = document.getElementById('unbidden-ink-prompt').value.trim();
        if (!currentPrompt) {
            showMessage('Please enter a prompt in the Custom Prompt field before testing.', 'error');
            return;
        }
        
        // Show loading state
        button.disabled = true;
        button.textContent = 'Generating...';
        
        const response = await fetch('/unbidden-ink', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                prompt: currentPrompt
            })
        });
        
        if (!response.ok) {
            const errorData = await response.json().catch(() => ({}));
            let errorMessage = errorData.error || `HTTP ${response.status}: ${response.statusText}`;
            
            // Provide more helpful error messages
            if (response.status === 500 && errorMessage.includes('Failed to generate')) {
                errorMessage = 'Failed to generate content. Please check that your ChatGPT API Token is valid and you have sufficient API credits. You can check your account at https://platform.openai.com/account';
            }
            
            throw new Error(errorMessage);
        }
        
        const data = await response.json();
        
        if (data.success) {
            showMessage('Unbidden Ink content generated and sent to printer!', 'success');
        } else {
            throw new Error(data.error || 'Unknown error occurred');
        }
        
    } catch (error) {
        console.error('Failed to test Unbidden Ink:', error);
        showMessage(`Failed to test Unbidden Ink: ${error.message}`, 'error');
    } finally {
        // Restore button
        button.disabled = false;
        button.textContent = originalText;
    }
}