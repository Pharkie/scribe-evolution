/**
 * @file settings-main.js
 * @brief Main coordination module - initialization and module orchestration
 * @description Lightweight coordinator that manages module interactions and page initialization
 */

/**
 * Initialize the settings page and coordinate all modules
 */
async function initializeSettings() {
    try {
        // Wait for all modules to be available
        await waitForModules();
        
        // Load configuration from API
        const config = await window.SettingsAPI.loadConfiguration();
        
        // Populate the form with loaded configuration
        window.SettingsUI.populateForm(config);
        
        // Set up form submission handler
        setupFormHandler();
        
        // Set up UI event handlers (this sets data attributes on preset buttons)
        setupEventHandlers();
        
        // Now that event handlers are set up, match custom prompt to preset
        window.SettingsUI.matchCustomPromptToPreset(config.unbiddenInk?.prompt || '');
        
        // Hide loading state and show settings
        window.SettingsUI.hideLoadingState();
        
        console.log('Settings page initialized successfully');
        
    } catch (error) {
        console.error('Failed to initialize settings page:', error);
        window.SettingsUI.showMessage('Failed to initialize settings page: ' + error.message, 'error');
    }
}

/**
 * Wait for all required modules to be loaded
 */
async function waitForModules() {
    const maxWaitTime = 5000; // 5 seconds
    const checkInterval = 100; // 100ms
    let waitTime = 0;
    
    while (waitTime < maxWaitTime) {
        if (window.SettingsAPI && window.SettingsUI && window.SettingsLED) {
            return; // All modules loaded
        }
        
        await new Promise(resolve => setTimeout(resolve, checkInterval));
        waitTime += checkInterval;
    }
    
    throw new Error('Timeout waiting for settings modules to load');
}

/**
 * Set up form submission handler
 */
function setupFormHandler() {
    const settingsForm = document.getElementById('settings-form');
    if (settingsForm) {
        settingsForm.addEventListener('submit', handleFormSubmit);
    }
}

/**
 * Set up additional UI event handlers
 */
function setupEventHandlers() {
    // Unbidden Ink toggle handler
    const unbiddenInkToggle = document.getElementById('unbidden-ink-enabled');
    if (unbiddenInkToggle) {
        unbiddenInkToggle.addEventListener('change', window.SettingsUI.toggleUnbiddenInkSettings);
        // Remove inline handler
        unbiddenInkToggle.removeAttribute('onchange');
        // Trigger initial state
        window.SettingsUI.toggleUnbiddenInkSettings();
    }
    
    // Time range slider handlers
    const timeStartSlider = document.getElementById('time-start');
    const timeEndSlider = document.getElementById('time-end');
    if (timeStartSlider && timeEndSlider) {
        timeStartSlider.addEventListener('input', function() {
            window.SettingsUI.updateTimeRange(this, 'start');
        });
        timeEndSlider.addEventListener('input', function() {
            window.SettingsUI.updateTimeRange(this, 'end');
        });
        
        // Change handlers for click areas update
        timeStartSlider.addEventListener('change', window.SettingsUI.updateClickAreas);
        timeEndSlider.addEventListener('change', window.SettingsUI.updateClickAreas);
        
        // Remove inline handlers since we're handling these events now
        timeStartSlider.removeAttribute('oninput');
        timeStartSlider.removeAttribute('onchange');
        timeEndSlider.removeAttribute('oninput');
        timeEndSlider.removeAttribute('onchange');
    }
    
    // Custom prompt textarea handler - clear preset highlights when user types
    const customPromptTextarea = document.getElementById('unbidden-ink-prompt');
    if (customPromptTextarea) {
        customPromptTextarea.addEventListener('input', function() {
            // Clear preset highlights when user modifies the text
            setTimeout(() => {
                // Check if current text matches any preset exactly
                const currentText = this.value;
                window.SettingsUI.matchCustomPromptToPreset(currentText);
            }, 100);
        });
    }
    
    // Navigation button handlers
    setupNavigationHandlers();
    
    // Frequency slider handler
    setupFrequencyHandlers();
    
    // Prompt preset button handlers
    setupPromptPresetHandlers();
    
    // Click area handlers for time range sliders
    setupTimeRangeClickHandlers();
    
    // Back button handler
    setupBackButtonHandler();
    
    // Test buttons handlers
    setupTestButtonHandlers();
    
    // LED demo button handlers  
    setupLedDemoHandlers();
}

/**
 * Setup navigation button handlers
 */
function setupNavigationHandlers() {
    // Settings section navigation buttons
    const navButtons = [
        { id: 'nav-wifi', section: 'wifi' },
        { id: 'nav-device', section: 'device' },
        { id: 'nav-mqtt', section: 'mqtt' },
        { id: 'nav-unbidden', section: 'unbidden' },
        { id: 'nav-buttons', section: 'buttons' },
        { id: 'nav-leds', section: 'leds' }
    ];
    
    navButtons.forEach(nav => {
        const button = document.querySelector(`[onclick*="showSettingsSection('${nav.section}')"]`);
        if (button) {
            button.addEventListener('click', function(e) {
                e.preventDefault();
                window.SettingsUI.showSettingsSection(nav.section);
            });
            // Remove inline handler
            button.removeAttribute('onclick');
        }
    });
}

/**
 * Setup frequency slider handler
 */
function setupFrequencyHandlers() {
    const frequencySlider = document.getElementById('frequency-slider');
    if (frequencySlider) {
        frequencySlider.addEventListener('input', function() {
            window.SettingsUI.updateFrequencyFromSlider(this.value);
        });
        // Remove inline handler
        frequencySlider.removeAttribute('oninput');
    }
}

/**
 * Setup prompt preset button handlers
 */
function setupPromptPresetHandlers() {
    const presetPrompts = [
        'Generate a short, inspiring quote about creativity, technology, or daily life. Keep it under 200 characters.',
        'Generate a fun fact under 200 characters about BBC Doctor Who - the characters, episodes, behind-the-scenes trivia, or the show\'s history that is esoteric and only 5% of fans might know.',
        'Write a short, humorous observation about everyday life or a witty one-liner. Keep it light and under 200 characters.',
        'Generate a short creative writing prompt, mini-story, or poetic thought. Be imaginative and keep under 250 characters.'
    ];
    
    // Find all prompt preset buttons and set up their data attributes and event listeners
    const buttons = document.querySelectorAll('.prompt-preset[onclick*="setPrompt"]');
    
    buttons.forEach(button => {
        const onclickAttr = button.getAttribute('onclick');
        if (onclickAttr) {
            // Extract the prompt text from the onclick attribute with robust parsing
            // This regex handles escaped single quotes properly: (?:[^'\\]|\\.)*
            const match = onclickAttr.match(/setPrompt\('((?:[^'\\]|\\.)*)'\)/);
            if (match) {
                // Properly unescape the captured text
                const promptText = match[1]
                    .replace(/\\'/g, "'")    // Unescape single quotes
                    .replace(/\\"/g, '"')    // Unescape double quotes  
                    .replace(/\\\\/g, "\\")  // Unescape backslashes
                    .replace(/\\n/g, "\n")   // Unescape newlines
                    .replace(/\\t/g, "\t");  // Unescape tabs
                
                // Set up event listener
                button.addEventListener('click', function(e) {
                    e.preventDefault();
                    window.SettingsUI.setPrompt(promptText);
                });
                
                // Store the prompt text as a data attribute for later use in highlighting
                button.setAttribute('data-prompt-text', promptText);
                
                // Remove inline handler after storing the data
                button.removeAttribute('onclick');
            } else {
                console.warn('Could not parse onclick attribute:', onclickAttr);
            }
        }
    });
}

/**
 * Setup time range click area handlers
 */
function setupTimeRangeClickHandlers() {
    const startClickArea = document.getElementById('click-area-start');
    const endClickArea = document.getElementById('click-area-end');
    
    if (startClickArea) {
        startClickArea.addEventListener('click', function(e) {
            window.SettingsUI.handleSliderClick(e, 'start');
        });
        // Remove inline handler
        startClickArea.removeAttribute('onclick');
    }
    
    if (endClickArea) {
        endClickArea.addEventListener('click', function(e) {
            window.SettingsUI.handleSliderClick(e, 'end');
        });
        // Remove inline handler
        endClickArea.removeAttribute('onclick');
    }
}

/**
 * Setup back button handler
 */
function setupBackButtonHandler() {
    const backButton = document.querySelector('.back-button');
    if (backButton) {
        backButton.addEventListener('click', function(e) {
            e.preventDefault();
            window.SettingsUI.goBack();
        });
        // Remove inline handler
        backButton.removeAttribute('onclick');
    }
}

/**
 * Setup test button handlers
 */
function setupTestButtonHandlers() {
    const testUnbiddenButton = document.querySelector('[onclick*="testUnbiddenInk()"]');
    if (testUnbiddenButton) {
        testUnbiddenButton.addEventListener('click', function(e) {
            e.preventDefault();
            handleTestUnbiddenInk();
        });
        // Remove inline handler
        testUnbiddenButton.removeAttribute('onclick');
    }
}

/**
 * Setup LED demo button handlers
 */
function setupLedDemoHandlers() {
    // Find all LED demo buttons and add event listeners
    const ledButtons = document.querySelectorAll('[onclick*="demoLedEffect"]');
    ledButtons.forEach(button => {
        // Extract effect name from onclick attribute
        const onclickAttr = button.getAttribute('onclick');
        const effectMatch = onclickAttr.match(/demoLedEffect\('([^']+)'\)/);
        
        if (effectMatch) {
            const effectName = effectMatch[1];
            button.addEventListener('click', function(e) {
                e.preventDefault();
                if (window.SettingsLED && window.SettingsLED.demoLedEffect) {
                    window.SettingsLED.demoLedEffect(effectName);
                }
            });
            // Remove inline handler
            button.removeAttribute('onclick');
        }
    });
    
    // Handle LED off buttons
    const ledOffButtons = document.querySelectorAll('[onclick*="turnOffLeds"]');
    ledOffButtons.forEach(button => {
        button.addEventListener('click', function(e) {
            e.preventDefault();
            handleTurnOffLeds();
        });
        // Remove inline handler
        button.removeAttribute('onclick');
    });
}

/**
 * Handle form submission
 * @param {Event} event - Form submit event
 */
async function handleFormSubmit(event) {
    event.preventDefault();
    
    try {
        // Collect form data
        const formData = window.SettingsUI.collectFormData();
        
        // Validate LED configuration
        if (window.SettingsLED.validateLedConfig) {
            const ledValidation = window.SettingsLED.validateLedConfig(formData.leds);
            if (!ledValidation.isValid) {
                window.SettingsUI.showMessage(ledValidation.error, 'error');
                return;
            }
        }
        
        // Save configuration via API
        await window.SettingsAPI.saveConfiguration(formData);
        
        // Redirect to index page with success message
        window.location.href = '/?message=Settings saved successfully!&type=success';
        
    } catch (error) {
        console.error('Failed to save settings:', error);
        window.SettingsUI.showMessage('Failed to save settings: ' + error.message, 'error');
    }
}

/**
 * Handle LED effect testing
 * @param {string} effectName - Name of the effect to trigger
 */
async function handleLedEffect(effectName) {
    try {
        await window.SettingsAPI.triggerLedEffect(effectName);
        window.SettingsUI.showMessage(`${effectName} effect triggered successfully!`, 'success');
    } catch (error) {
        console.error('Failed to trigger LED effect:', error);
        window.SettingsUI.showMessage(`Failed to trigger ${effectName} effect: ${error.message}`, 'error');
    }
}

/**
 * Handle turning off LEDs
 */
async function handleTurnOffLeds() {
    try {
        await window.SettingsAPI.turnOffLeds();
        window.SettingsUI.showMessage('LEDs turned off successfully!', 'success');
    } catch (error) {
        console.error('Failed to turn off LEDs:', error);
        window.SettingsUI.showMessage(`Failed to turn off LEDs: ${error.message}`, 'error');
    }
}

/**
 * Handle Unbidden Ink testing
 */
async function handleTestUnbiddenInk() {
    const button = event.target;
    const originalText = button.textContent;
    
    try {
        // Validate prerequisites
        const chatgptToken = document.getElementById('chatgpt-api-token').value.trim();
        if (!chatgptToken) {
            window.SettingsUI.showMessage('Please configure your ChatGPT API Token first. You can get one from https://platform.openai.com/api-keys', 'error');
            return;
        }
        
        const unbiddenInkEnabled = document.getElementById('unbidden-ink-enabled').checked;
        if (!unbiddenInkEnabled) {
            window.SettingsUI.showMessage('Please enable Unbidden Ink first before testing.', 'error');
            return;
        }
        
        const currentPrompt = document.getElementById('unbidden-ink-prompt').value.trim();
        if (!currentPrompt) {
            window.SettingsUI.showMessage('Please enter a prompt in the Custom Prompt field before testing.', 'error');
            return;
        }
        
        // Show loading state
        button.disabled = true;
        button.textContent = 'Generating...';
        
        // Generate content via API
        const result = await window.SettingsAPI.testUnbiddenInkGeneration(currentPrompt);
        
        if (result.content) {
            // Print the generated content
            await window.SettingsAPI.printLocalContent(result.content);
            window.SettingsUI.showMessage('Unbidden Ink generated and printed successfully', 'success');
        } else {
            throw new Error(result.error || 'No content generated');
        }
        
    } catch (error) {
        console.error('Failed to test Unbidden Ink:', error);
        window.SettingsUI.showMessage(`Failed to test Unbidden Ink: ${error.message}`, 'error');
    } finally {
        // Restore button state
        button.disabled = false;
        button.textContent = originalText;
    }
}

// =============================================================================
// GLOBAL FUNCTION EXPORTS (minimal - most now handled by event listeners)
// =============================================================================

// Form and save functions (still needed for form submission and external calls)
window.saveSettings = handleFormSubmit;

// LED functions (may be called externally)
window.triggerLedEffect = handleLedEffect;
window.turnOffLeds = handleTurnOffLeds;

// Keep these as fallbacks in case any external code still references them
window.showSettingsSection = window.SettingsUI.showSettingsSection;
window.goBack = window.SettingsUI.goBack;
window.testUnbiddenInk = handleTestUnbiddenInk;
window.toggleUnbiddenInkSettings = window.SettingsUI.toggleUnbiddenInkSettings;
window.setPrompt = window.SettingsUI.setPrompt;
window.setFrequency = window.SettingsUI.setFrequency;

// Initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeSettings);
} else {
    // DOM is already loaded
    initializeSettings();
}
