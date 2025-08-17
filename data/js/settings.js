// Settings Page Main Coordinator
// Orchestrates the modular settings components

// Load LED effects configuration HTML partial
async function loadLEDEffectsConfig() {
    try {
        const response = await fetch('/html/partials/led-effects-config.html');
        if (response.ok) {
            const html = await response.text();
            const container = document.getElementById('led-effects-config-container');
            if (container) {
                container.innerHTML = html;
            }
        } else {
            console.error('Failed to load LED effects configuration partial');
        }
    } catch (error) {
        console.error('Error loading LED effects configuration partial:', error);
    }
}

// Main event handlers setup
function setupEventHandlers() {
    // Save button handler
    const saveButton = document.getElementById('save-button');
    if (saveButton) {
        saveButton.addEventListener('click', async () => {
            // Use SettingsCore to handle the save operation
            if (window.SettingsCore && window.SettingsCore.saveSettings) {
                await window.SettingsCore.saveSettings();
            }
        });
    }

    // Load button handler  
    const loadButton = document.getElementById('load-button');
    if (loadButton) {
        loadButton.addEventListener('click', () => {
            // Use SettingsCore to handle the load operation
            if (window.SettingsCore && window.SettingsCore.loadConfiguration) {
                window.SettingsCore.loadConfiguration();
            }
        });
    }
}

// Main DOMContentLoaded handler
document.addEventListener('DOMContentLoaded', () => {
    // Wait for all modules to be available
    if (!window.SettingsCore || !window.LEDConfig || !window.SettingsUtils) {
        console.error('Required settings modules not loaded');
        return;
    }
    
    // Load LED effects configuration HTML partial
    loadLEDEffectsConfig().then(() => {
        // Initialize modules after partial content is loaded
        SettingsCore.initialize();
        SettingsUtils.initialize();
        
        // Initialize LED configuration
        LEDConfig.initialize();
        
        // Set up main event handlers
        setupEventHandlers();
    });
});

// Backward compatibility aliases for legacy code
window.loadConfiguration = () => {
    if (window.SettingsCore && window.SettingsCore.loadConfiguration) {
        return window.SettingsCore.loadConfiguration();
    }
};

window.saveSettings = () => {
    if (window.SettingsCore && window.SettingsCore.saveSettings) {
        return window.SettingsCore.saveSettings();
    }
};

window.populateForm = (config) => {
    if (window.SettingsCore && window.SettingsCore.populateForm) {
        return window.SettingsCore.populateForm(config);
    }
};

window.collectFormData = () => {
    if (window.SettingsCore && window.SettingsCore.collectFormData) {
        return window.SettingsCore.collectFormData();
    }
};

console.log('Settings coordinator loaded');