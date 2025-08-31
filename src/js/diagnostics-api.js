/**
 * @file diagnostics-api.js
 * @brief API communication module for diagnostics page - handles all server interactions
 * @description Focused module for loading diagnostics, config, and NVS data
 */

/**
 * Load diagnostics data from server API
 * @returns {Promise<Object>} Diagnostics object from server
 */
async function loadDiagnostics() {
    try {
        console.log('API: Loading diagnostics from server...');
        
        const response = await fetch('/api/diagnostics');
        if (!response.ok) {
            throw new Error(`Diagnostics API returned ${response.status}: ${response.statusText}`);
        }
        
        const diagnostics = await response.json();
        console.log('API: Diagnostics loaded successfully');
        return diagnostics;
        
    } catch (error) {
        console.error('API: Failed to load diagnostics:', error);
        throw error;
    }
}

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
 * Load NVS dump data from server API
 * @returns {Promise<Object>} NVS dump object from server
 */
async function loadNVSDump() {
    try {
        console.log('API: Loading NVS dump from server...');
        
        const response = await fetch('/api/nvs-dump');
        if (!response.ok) {
            throw new Error(`NVS dump API returned ${response.status}: ${response.statusText}`);
        }
        
        const nvs = await response.json();
        console.log('API: NVS dump loaded successfully');
        return nvs;
        
    } catch (error) {
        console.error('API: Failed to load NVS dump:', error);
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
            method: 'GET'
        });

        if (!response.ok) {
            const errorData = await response.text();
            throw new Error(`Quick action '${action}' failed: ${response.status} - ${errorData}`);
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
 * Print content locally via API
 * @param {string} content - Content to print
 * @returns {Promise<void>}
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
            const errorData = await response.json().catch(() => ({}));
            throw new Error(errorData.error || `Print failed: HTTP ${response.status}`);
        }
        
        console.log('API: Content sent to printer successfully');
        
    } catch (error) {
        console.error('API: Failed to print content:', error);
        throw error;
    }
}

/**
 * Load routes data from server API
 * @returns {Promise<Object>} Routes object from server
 */
async function loadRoutes() {
    try {
        console.log('API: Loading routes from server...');
        
        const response = await fetch('/api/routes');
        if (!response.ok) {
            throw new Error(`Routes API returned ${response.status}: ${response.statusText}`);
        }
        
        const routes = await response.json();
        console.log('API: Routes loaded successfully');
        return routes;
        
    } catch (error) {
        console.error('API: Failed to load routes:', error);
        throw error;
    }
}

// Export API module
window.DiagnosticsAPI = {
    loadDiagnostics,
    loadConfiguration,
    loadNVSDump,
    executeQuickAction,
    printLocalContent,
    loadRoutes
};
