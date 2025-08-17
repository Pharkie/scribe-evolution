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
