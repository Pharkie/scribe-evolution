/**
 * @file settings.js
 * @brief Main settings page coordinator
 * @description Lightweight coordinator that orchestrates the modular settings components
 */

// Import note: The actual modules are loaded via HTML script tags
// This file serves as the main entry point and coordinator

/**
 * Initialize the settings page
 */
async function initializeSettings() {
    try {
        // Wait for all modules to be available
        await waitForModules();
        
        // Initialize UI components
        if (window.SettingsUtils && window.SettingsUtils.initializeUI) {
            window.SettingsUtils.initializeUI();
        }
        
        // Load and populate configuration
        if (window.SettingsCore && window.SettingsCore.loadConfiguration) {
            await window.SettingsCore.loadConfiguration();
        }
        
        // Set up form submission handler
        const settingsForm = document.getElementById('settings-form');
        if (settingsForm && window.SettingsCore && window.SettingsCore.saveSettings) {
            settingsForm.addEventListener('submit', window.SettingsCore.saveSettings);
        }
        
        // Hide loading state and show settings
        hideLoadingState();
        
        console.log('Settings page initialized successfully');
        
    } catch (error) {
        console.error('Failed to initialize settings page:', error);
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage('Failed to initialize settings page: ' + error.message, 'error');
        }
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
        if (window.SettingsCore && window.LEDConfig && window.SettingsUtils) {
            return; // All modules loaded
        }
        
        await new Promise(resolve => setTimeout(resolve, checkInterval));
        waitTime += checkInterval;
    }
    
    throw new Error('Timeout waiting for settings modules to load');
}

// Legacy function aliases for backward compatibility
async function loadConfiguration() {
    if (window.SettingsCore && window.SettingsCore.loadConfiguration) {
        return window.SettingsCore.loadConfiguration();
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
 * Show a specific settings section
 * @param {string} sectionName - The name of the section to show
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

// Make functions globally available
window.showSettingsSection = showSettingsSection;
window.hideLoadingState = hideLoadingState;

/**
 * Save settings form (global function for form onsubmit)
 * @param {Event} event - Form submit event
 */
async function saveSettings(event) {
    if (window.SettingsCore && window.SettingsCore.saveSettings) {
        return window.SettingsCore.saveSettings(event);
    }
}

// Make saveSettings globally available for form onsubmit
window.saveSettings = saveSettings;

/**
 * Trigger a LED effect for testing
 * @param {string} effectName - Name of the effect to trigger
 */
async function triggerLedEffect(effectName) {
    try {
        const response = await fetch('/api/led-effect', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                effect: effectName,
                duration: 10000 // 10 seconds
            })
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const result = await response.json();
        
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage(`${effectName} effect triggered successfully!`, 'success');
        }
        
    } catch (error) {
        console.error('Failed to trigger LED effect:', error);
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage(`Failed to trigger ${effectName} effect: ${error.message}`, 'error');
        }
    }
}

// Make triggerLedEffect globally available for button onclick
window.triggerLedEffect = triggerLedEffect;

/**
 * Turn off LEDs
 */
async function turnOffLeds() {
    try {
        const response = await fetch('/api/leds-off', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const result = await response.json();
        
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage('LEDs turned off successfully!', 'success');
        }
        
    } catch (error) {
        console.error('Failed to turn off LEDs:', error);
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage(`Failed to turn off LEDs: ${error.message}`, 'error');
        }
    }
}

// Make turnOffLeds globally available for button onclick  
window.turnOffLeds = turnOffLeds;

// Make other functions globally available for HTML onclick handlers
window.toggleUnbiddenInkSettings = toggleUnbiddenInkSettings;
window.goBack = goBack;

async function saveSettings(event) {
    if (window.SettingsCore && window.SettingsCore.saveSettings) {
        return window.SettingsCore.saveSettings(event);
    }
}

function showMessage(message, type) {
    if (window.SettingsCore && window.SettingsCore.showMessage) {
        return window.SettingsCore.showMessage(message, type);
    }
}

function toggleUnbiddenInkSettings() {
    if (window.SettingsUtils && window.SettingsUtils.toggleUnbiddenInkSettings) {
        return window.SettingsUtils.toggleUnbiddenInkSettings();
    }
}

function goBack() {
    if (window.SettingsUtils && window.SettingsUtils.goBack) {
        return window.SettingsUtils.goBack();
    }
}

function updateTimeRange() {
    if (window.SettingsUtils && window.SettingsUtils.updateTimeRange) {
        return window.SettingsUtils.updateTimeRange();
    }
}

// Initialize when DOM is ready
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeSettings);
} else {
    // DOM is already loaded
    initializeSettings();
}
