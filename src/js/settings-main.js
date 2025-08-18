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
        
        // Set up UI event handlers FIRST (before form population)
        setupEventHandlers();
        
        // Populate the form with loaded configuration
        window.SettingsUI.populateForm(config);
        
        // Set up form submission handler
        setupFormHandler();
        
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
    // Settings section navigation handlers using data attributes
    document.querySelectorAll('[data-settings-section]').forEach(button => {
        button.addEventListener('click', function() {
            const section = this.dataset.settingsSection;
            if (typeof showSettingsSection === 'function') {
                showSettingsSection(section);
            }
        });
    });
    
    // Unbidden Ink toggle handler using data attribute
    const unbiddenInkToggle = document.querySelector('[data-toggle="unbidden-ink-settings"]');
    if (unbiddenInkToggle) {
        unbiddenInkToggle.addEventListener('change', window.SettingsUI.toggleUnbiddenInkSettings);
        // Trigger initial state
        window.SettingsUI.toggleUnbiddenInkSettings();
    }
    
    // Slider click handlers using data attributes
    document.querySelectorAll('[data-slider-type]').forEach(element => {
        element.addEventListener('click', function(event) {
            const sliderType = this.dataset.sliderType;
            if (typeof handleSliderClick === 'function') {
                handleSliderClick(event, sliderType);
            }
        });
    });
    
    // Frequency slider handler using data attribute
    const frequencySlider = document.querySelector('[data-frequency-slider="true"]');
    if (frequencySlider) {
        frequencySlider.addEventListener('input', function() {
            if (typeof updateFrequencyFromSlider === 'function') {
                updateFrequencyFromSlider(this.value);
            }
        });
    }
    
    // Test button handlers using data attributes  
    document.querySelectorAll('[data-test-action]').forEach(button => {
        button.addEventListener('click', function() {
            const action = this.dataset.testAction;
            if (action === 'unbidden-ink' && typeof testUnbiddenInk === 'function') {
                testUnbiddenInk();
            }
        });
    });
    
    // LED demo button handlers using data attributes
    document.querySelectorAll('[data-demo-led-effect]').forEach(button => {
        button.addEventListener('click', function() {
            const effect = this.dataset.demoLedEffect;
            if (typeof demoLedEffect === 'function') {
                demoLedEffect(effect);
            }
        });
    });
    
    // LED action handlers
    document.querySelectorAll('[data-led-action]').forEach(button => {
        button.addEventListener('click', function() {
            const action = this.dataset.ledAction;
            if (action === 'turn-off' && typeof turnOffLeds === 'function') {
                turnOffLeds();
            }
        });
    });
    
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
    
    // Setup close button handler
    const closeButton = document.getElementById('close-button');
    if (closeButton) {
        closeButton.addEventListener('click', function() {
            if (typeof goBack === 'function') {
                goBack();
            }
        });
    }
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
    
    // Find all prompt preset buttons by class and set up event listeners
    const buttons = document.querySelectorAll('.prompt-preset');
    
    buttons.forEach((button, index) => {
        if (index < presetPrompts.length) {
            const promptText = presetPrompts[index];
            
            // Set up event listener
            button.addEventListener('click', function(e) {
                e.preventDefault();
                window.SettingsUI.setPrompt(promptText);
            });
            
            // Store the prompt text as a data attribute for later use in highlighting
            button.setAttribute('data-prompt-text', promptText);
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
    }
    
    if (endClickArea) {
        endClickArea.addEventListener('click', function(e) {
            window.SettingsUI.handleSliderClick(e, 'end');
        });
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
    }
}

/**
 * Setup test button handlers
 */
function setupTestButtonHandlers() {
    const testUnbiddenButton = document.querySelector('[data-test-action="unbidden-ink"]');
    if (testUnbiddenButton) {
        testUnbiddenButton.addEventListener('click', function(e) {
            e.preventDefault();
            handleTestUnbiddenInk();
        });
    }
}

/**
 * Setup LED demo button handlers
 */
function setupLedDemoHandlers() {
    // Find all LED demo buttons using data attributes
    const ledButtons = document.querySelectorAll('[data-demo-led-effect]');
    ledButtons.forEach(button => {
        const effectName = button.getAttribute('data-demo-led-effect');
        if (effectName) {
            button.addEventListener('click', function(e) {
                e.preventDefault();
                if (window.SettingsLED && window.SettingsLED.demoLedEffect) {
                    window.SettingsLED.demoLedEffect(effectName);
                }
            });
        }
    });
    
    // Handle LED off buttons using data attributes
    const ledOffButtons = document.querySelectorAll('[data-led-action="turn-off"]');
    ledOffButtons.forEach(button => {
        button.addEventListener('click', function(e) {
            e.preventDefault();
            handleTurnOffLeds();
        });
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
