/**
 * @file settings-api.js
 * @brief API communication module - handles all server interactions
 * @description Focused module for loading and saving configuration data
 */

/**
 * Load configuration from server API
 * @returns {Promise<Object>} Configuration object from server
 */
async function loadConfiguration() {
    try {
        console.log('API: Loading configuration from server...');
        
        const response = await fetch('/api/config');
        if (!response.ok) {
            throw new Error(`Config API returned ${response.status}: ${response.statusText}`);
        }
        
        const config = await response.json();
        console.log('API: Configuration loaded successfully');
        return config;
        
    } catch (error) {
        console.error('API: Failed to load configuration:', error);
        throw error;
    }
}

/**
 * Save configuration to server API
 * @param {Object} configData - Configuration object to save
 * @returns {Promise<string>} Server response message
 */
async function saveConfiguration(configData) {
    try {
        console.log('API: Sending config to server...');
        
        const response = await fetch('/api/config', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(configData)
        });
        
        if (!response.ok) {
            const errorText = await response.text();
            console.error('API: Server error response:', errorText);
            throw new Error(`Server error: ${response.status} - ${errorText}`);
        }
        
        const result = await response.text();
        console.log('API: Server response:', result);
        return result;
        
    } catch (error) {
        console.error('API: Failed to save configuration:', error);
        throw error;
    }
}

/**
 * Test Unbidden Ink generation via API
 * @param {string} prompt - Prompt text to use for generation
 * @returns {Promise<Object>} Generated content response
 */
async function testUnbiddenInkGeneration(prompt) {
    try {
        const response = await fetch('/api/unbidden-ink', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ prompt })
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
        
        return await response.json();
        
    } catch (error) {
        console.error('API: Failed to test Unbidden Ink:', error);
        throw error;
    }
}

/**
 * Print content locally via API
 * @param {string} content - Content to print
 * @returns {Promise<void>}
 */
async function printLocalContent(content) {
    try {
        const response = await fetch('/api/print-local', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ message: content })
        });
        
        if (!response.ok) {
            const errorData = await response.json().catch(() => ({}));
            throw new Error(errorData.error || `Print failed: HTTP ${response.status}`);
        }
        
    } catch (error) {
        console.error('API: Failed to print content:', error);
        throw error;
    }
}

/**
 * Trigger LED effect via API with optional settings
 * @param {string} effectName - Name of the LED effect
 * @param {number} duration - Duration in milliseconds (default 10000)
 * @param {Object} settings - Effect-specific settings (optional)
 * @returns {Promise<Object>} API response
 */
async function triggerLedEffect(effectName, duration = 10000, settings = null) {
    try {
        const payload = {
            effect: effectName,
            duration: duration
        };
        
        // Add settings to payload if provided
        if (settings && Object.keys(settings).length > 0) {
            payload.settings = settings;
        }
        
        const response = await fetch('/api/led-effect', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(payload)
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        return await response.json();
        
    } catch (error) {
        console.error('API: Failed to trigger LED effect:', error);
        throw error;
    }
}

/**
 * Turn off LEDs via API
 * @returns {Promise<Object>} API response
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

        return await response.json();
        
    } catch (error) {
        console.error('API: Failed to turn off LEDs:', error);
        throw error;
    }
}

// Export API module
window.SettingsAPI = {
    loadConfiguration,
    saveConfiguration,
    testUnbiddenInkGeneration,
    printLocalContent,
    triggerLedEffect,
    turnOffLeds
};
