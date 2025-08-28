/**
 * Setup API - Dedicated API for the setup.html page (AP mode initial configuration)
 * 
 * This is a simplified version of the settings API focused only on initial device setup
 */

console.log('Loading Setup API...');

// Setup API namespace
window.SetupAPI = {
    
    /**
     * Load initial setup configuration from server
     * Uses the dedicated /api/setup endpoint that returns minimal config for AP mode
     * @returns {Promise<Object>} Minimal configuration for setup
     */
    async loadConfiguration() {
        console.log('Setup API: Loading setup configuration...');
        const response = await fetch('/api/setup', {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json'
            }
        });

        if (!response.ok) {
            const errorData = await response.json().catch(() => ({}));
            throw new Error(errorData.message || `Failed to load setup configuration: ${response.status} - ${response.statusText}`);
        }

        const config = await response.json();
        console.log('Setup API: Configuration loaded:', config);
        return config;
    },

    /**
     * Save setup configuration using the minimal validation endpoint
     * @param {Object} config - Configuration object with device settings
     * @returns {Promise<Object>} Response from server
     */
    async saveConfiguration(config) {
        console.log('Setup API: Saving setup configuration...', config);
        const response = await fetch('/api/setup', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(config)
        });

        if (!response.ok) {
            const errorData = await response.json().catch(() => ({}));
            throw new Error(errorData.message || `Failed to save setup configuration: ${response.status} - ${response.statusText}`);
        }

        const result = await response.json();
        console.log('Setup API: Configuration saved:', result);
        return result;
    },

    /**
     * Scan for available WiFi networks (reuse from main settings API)
     * @returns {Promise<Array>} Array of WiFi network objects
     */
    async scanWiFiNetworks() {
        console.log('Setup API: Scanning WiFi networks...');
        const response = await fetch('/api/wifi-scan', {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json'
            }
        });

        if (!response.ok) {
            const errorData = await response.json().catch(() => ({}));
            throw new Error(errorData.message || `WiFi scan failed: ${response.status} - ${response.statusText}`);
        }

        const result = await response.json();
        console.log('Setup API: Found', result.networks?.length || 0, 'networks');
        return result.networks || [];
    }
};

console.log('Setup API loaded');