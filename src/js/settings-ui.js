/**
 * @file settings-ui.js
 * @brief UI manipulation module - handles all DOM interactions and form management
 * @description Focused module for user interface operations, form population, and message display
 */

/**
 * Populate all form s/**
 * Update the visual time range display only - no collision detection
 * Used when loading configuration to preserve exact values
 */
function updateTimeRangeDisplay() {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    
    if (!startSlider || !endSlider) return;
    
    const startVal = parseInt(startSlider.value);
    const endVal = parseInt(endSlider.value);
    
    // Update the visual track
    const track = document.getElementById('time-track');
    if (track) {
        if (startVal === 0 && endVal === 0) {
            // Full day operation - show full width
            track.style.left = '0%';
            track.style.width = '100%';
        } else {
            // Calculate percentages based on slider positioning - track should end at handle position
            const startPercent = (startVal / 23) * 100;
            const endPercent = (endVal / 23) * 100; // End exactly at the handle position
            
            track.style.left = startPercent + '%';
            track.style.width = (endPercent - startPercent) + '%';
        }
    }
    
    // Update time display labels  
    const startDisplay = document.getElementById('time-display-start');
    const endDisplay = document.getElementById('time-display-end');
    
    // Format hour display
    const formatHour = (hour) => {
        if (hour === 24) return '00:00';
        return String(hour).padStart(2, '0') + ':00';
    };
    
    if (startDisplay) startDisplay.textContent = formatHour(startVal);
    if (endDisplay) endDisplay.textContent = formatHour(endVal);
    
    // Update frequency display
    updateFrequencyDisplay();
}

/**
 * Populate form with configuration data
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
    setElementValue('time-start', config.unbiddenInk?.startHour !== undefined ? config.unbiddenInk.startHour : 8);
    setElementValue('time-end', config.unbiddenInk?.endHour !== undefined ? config.unbiddenInk.endHour : 22);
    
    // Initialize time range display and click areas after a short delay to ensure DOM is ready
    setTimeout(() => {
        updateTimeRangeDisplay(); 
        updateClickAreas();
    }, 100);
    
    // Frequency and prompt
    setElementValue('frequency-minutes', config.unbiddenInk?.frequencyMinutes || 60);
    updateSliderFromFrequency(config.unbiddenInk?.frequencyMinutes || 60);
    
    // Update frequency display to reflect loaded values
    updateFrequencyDisplay();
    
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
    
    // LED configuration (delegate to LED module)
    if (window.SettingsLED && window.SettingsLED.populateLedForm) {
        window.SettingsLED.populateLedForm(config);
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
 * Update time range with collision detection for user interactions
 * @param {HTMLElement} slider - The slider element that triggered the change
 * @param {string} type - Either 'start' or 'end' to indicate which slider moved
 */
function updateTimeRange(slider, type) {
    const startSlider = document.getElementById('time-start');
    const endSlider = document.getElementById('time-end');
    
    if (!startSlider || !endSlider) return;
    
    let startVal = parseInt(startSlider.value);
    let endVal = parseInt(endSlider.value);
    
    // Debug: Log the values
    console.log('updateTimeRange - startVal:', startVal, 'endVal:', endVal, 'type:', type);
    
    // Special case: Allow 0-0 for full day operation (24 hours)  
    if (startVal === 0 && endVal === 0) {
        // This is valid - full day operation
    } else {
        // Smart collision handling - move the other handle when possible
        if (type === 'start') {
            if (startVal >= endVal) {
                if (endVal < 23) {
                    // Move end handle forward
                    endVal = startVal + 1;
                    endSlider.value = endVal;
                } else {
                    // End can't move, constrain start
                    startVal = 22;
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
        } else {
            // No type specified - use original logic for when called from config loading
            if (startVal >= endVal) {
                if (endVal < 23) {
                    // Move end handle forward
                    endVal = startVal + 1;
                    endSlider.value = endVal;
                } else {
                    // End can't move, constrain start
                    startVal = 22;
                    startSlider.value = startVal;
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
            // Calculate percentages based on slider positioning - track should end at handle position
            const startPercent = (startVal / 23) * 100;
            const endPercent = (endVal / 23) * 100; // End exactly at the handle position
            
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
    updateFrequencyDisplay();
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

/**
 * Handle clicks on the time range slider areas
 * @param {Event} event - Click event
 * @param {string} type - 'start' or 'end'
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
                updateTimeRange();
                updateClickAreas();
                return;
            }
        }
        
        startSlider.value = newStartVal;
        updateTimeRange();
    } else if (type === 'end') {
        const newEndVal = Math.max(0, Math.min(24, clickHour));
        const startVal = parseInt(startSlider.value);
        
        // Collision avoidance: move other handle if necessary
        if (newEndVal <= startVal && !(startVal === 0 && newEndVal === 0)) {
            if (startVal > 0) {
                startSlider.value = newEndVal - 1;
            } else {
                endSlider.value = startVal + 1;
                updateTimeRange();
                updateClickAreas();
                return;
            }
        }
        
        endSlider.value = newEndVal;
        updateTimeRange();
    }
    
    updateClickAreas();
}

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
    
    // Calculate positions as percentages for 24-hour scale (0-24) - matching old behavior
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
 * Update frequency from slider position (0-7 maps to frequency values)
 * @param {number} sliderValue - Slider position value
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
 * @param {number} minutes - Frequency in minutes
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
    
    // Format hours for display using 24-hour format
    const formatHour = (hour) => {
        return String(hour).padStart(2, '0') + ':00';
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
 * Set frequency from preset buttons
 * @param {number} minutes - Frequency in minutes
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
 * Set prompt from preset buttons
 * @param {string} promptText - The prompt text to set
 */
function setPrompt(promptText) {
    const textarea = document.getElementById('unbidden-ink-prompt');
    if (textarea) {
        textarea.value = promptText;
        // Trigger any change events
        textarea.dispatchEvent(new Event('input'));
        
        // Highlight the matching preset button immediately
        highlightMatchingPreset(promptText);
    }
}

/**
 * Match custom prompt to preset and highlight if exact match
 * @param {string} customPrompt - The custom prompt text to match
 */
function matchCustomPromptToPreset(customPrompt) {
    if (!customPrompt) {
        // Clear highlights if no prompt
        document.querySelectorAll('.prompt-preset').forEach(button => {
            button.classList.remove('bg-purple-100', 'dark:bg-purple-900/40', 'ring-2', 'ring-purple-400');
        });
        return;
    }
    
    // Define the preset prompts (must match exactly with HTML onclick values)
    const presetPrompts = [
        'Generate a short, inspiring quote about creativity, technology, or daily life. Keep it under 200 characters.',
        'Generate a fun fact under 200 characters about BBC Doctor Who - the characters, episodes, behind-the-scenes trivia, or the show\'s history that is esoteric and only 5% of fans might know.',
        'Write a short, humorous observation about everyday life or a witty one-liner. Keep it light and under 200 characters.',
        'Generate a short creative writing prompt, mini-story, or poetic thought. Be imaginative and keep under 250 characters.'
    ];
    
    // Check for exact match and highlight immediately
    const matchingIndex = presetPrompts.findIndex(preset => preset === customPrompt);
    
    if (matchingIndex !== -1) {
        highlightMatchingPreset(customPrompt);
    } else {
        // Clear highlights if no match
        document.querySelectorAll('.prompt-preset').forEach(button => {
            button.classList.remove('bg-purple-100', 'dark:bg-purple-900/40', 'ring-2', 'ring-purple-400');
        });
    }
}

/**
 * Highlight the preset button that matches the given prompt text
 * @param {string} promptText - The prompt text to find and highlight
 */
function highlightMatchingPreset(promptText) {
    console.log('Highlighting preset for prompt:', promptText);
    
    // Remove existing highlights
    document.querySelectorAll('.prompt-preset').forEach(button => {
        button.classList.remove('bg-purple-100', 'dark:bg-purple-900/40', 'ring-2', 'ring-purple-400');
    });
    
    // Find and highlight the matching preset by checking the data attribute or onclick
    document.querySelectorAll('.prompt-preset').forEach(button => {
        let buttonPromptText = button.getAttribute('data-prompt-text');
        
        // Fallback to checking onclick if data attribute is not set yet
        if (!buttonPromptText) {
            const onclick = button.getAttribute('onclick');
            if (onclick) {
                const match = onclick.match(/setPrompt\('([^']*)'\)/);
                if (match) {
                    buttonPromptText = match[1];
                }
            }
        }
        
        console.log('Checking button prompt:', buttonPromptText);
        
        // Compare with the provided prompt text (exact match)
        if (buttonPromptText && buttonPromptText.trim() === promptText.trim()) {
            console.log('Found matching preset button, highlighting');
            button.classList.add('bg-purple-100', 'dark:bg-purple-900/40', 'ring-2', 'ring-purple-400');
        }
    });
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
    updateTimeRangeDisplay,
    updateTimeRange,
    updateNextScheduledDisplay,
    goBack,
    handleSliderClick,
    updateClickAreas,
    updateFrequencyFromSlider,
    updateSliderFromFrequency,
    updateFrequencyDisplay,
    setFrequency,
    setPrompt,
    matchCustomPromptToPreset,
    highlightMatchingPreset
};
