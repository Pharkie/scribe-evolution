/**
 * @file index-api.js
 * @brief API communication module for index page - handles all server interactions
 * @description Focused module for loading config, printing, and quick actions
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
 * Print content locally via API
 * @param {string} content - Content to print
 * @returns {Promise<Object>} Print response from server
 */
async function printLocalContent(content) {
    try {
        console.log('API: Sending content to local printer...');
        
        const response = await fetch('/api/print-local', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ message: content })
        });

        if (!response.ok) {
            const errorData = await response.text();
            throw new Error(`Print failed: ${errorData}`);
        }
        
        const result = await response.json();
        console.log('API: Content sent to local printer successfully');
        return result;
        
    } catch (error) {
        console.error('API: Failed to print local content:', error);
        throw error;
    }
}

/**
 * Print content via MQTT to remote printer
 * @param {string} content - Content to print
 * @param {string} topic - MQTT topic for the target printer
 * @returns {Promise<Object>} Print response from server
 */
async function printMQTTContent(content, topic) {
    try {
        console.log(`API: Sending content to MQTT printer on topic: ${topic}`);
        
        const response = await fetch('/api/print-mqtt', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                message: content,
                topic: topic 
            })
        });

        if (!response.ok) {
            const errorData = await response.text();
            throw new Error(`MQTT print failed: ${errorData}`);
        }
        
        const result = await response.json();
        console.log('API: Content sent to MQTT printer successfully');
        return result;
        
    } catch (error) {
        console.error('API: Failed to print MQTT content:', error);
        throw error;
    }
}

/**
 * Execute a quick action via API
 * @param {string} action - Action name (e.g., 'time', 'joke', 'riddle', 'wisdom')
 * @returns {Promise<Object>} Generated content response
 */
async function executeQuickAction(action) {
    try {
        console.log(`API: Executing quick action: ${action}`);
        
        const response = await fetch(`/api/${action}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' }
        });

        if (!response.ok) {
            const errorData = await response.text();
            throw new Error(`Quick action '${action}' failed: ${errorData}`);
        }

        const result = await response.json();
        console.log(`API: Quick action '${action}' completed successfully`);
        return result;
        
    } catch (error) {
        console.error(`API: Failed to execute quick action '${action}':`, error);
        throw error;
    }
}

/**
 * Generate formatted user message content with MESSAGE header
 * @param {string} message - User's message text
 * @param {string} target - Target printer (local-direct or MQTT topic)
 * @returns {Promise<Object>} Generated content with appropriate MESSAGE header
 */
async function generateUserMessage(message, target = 'local-direct') {
    try {
        console.log('API: Generating user message content...');
        
        // Build payload with target for proper header formatting
        const payload = { message: message };
        if (target !== 'local-direct') {
            payload.target = target;
        }
        
        const response = await fetch('/api/user-message', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        
        const result = await response.json();
        console.log('API: User message content generated successfully');
        return result;
        
    } catch (error) {
        console.error('API: Failed to generate user message content:', error);
        throw error;
    }
}

// Export API module
window.IndexAPI = {
    loadConfiguration,
    printLocalContent,
    printMQTTContent,
    executeQuickAction,
    generateUserMessage
};
