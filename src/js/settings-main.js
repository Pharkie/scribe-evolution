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
        
        // Set up UI event handlers
        setupEventHandlers();
        
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
        // Trigger initial state
        window.SettingsUI.toggleUnbiddenInkSettings();
    }
    
    // Time range slider handlers
    const timeStartSlider = document.getElementById('time-start');
    const timeEndSlider = document.getElementById('time-end');
    if (timeStartSlider && timeEndSlider) {
        timeStartSlider.addEventListener('input', window.SettingsUI.updateTimeRange);
        timeEndSlider.addEventListener('input', window.SettingsUI.updateTimeRange);
    }
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
// GLOBAL FUNCTION EXPORTS (for HTML onclick handlers)
// =============================================================================

// Navigation functions
window.showSettingsSection = window.SettingsUI.showSettingsSection;
window.goBack = window.SettingsUI.goBack;

// Form and save functions
window.saveSettings = handleFormSubmit;

// LED functions
window.triggerLedEffect = handleLedEffect;
window.turnOffLeds = handleTurnOffLeds;

// Unbidden Ink functions
window.testUnbiddenInk = handleTestUnbiddenInk;
window.toggleUnbiddenInkSettings = window.SettingsUI.toggleUnbiddenInkSettings;

// Initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeSettings);
} else {
    // DOM is already loaded
    initializeSettings();
}
