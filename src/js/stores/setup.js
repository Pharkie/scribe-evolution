/**
 * Alpine.js store factory for setup.html page (AP mode initial configuration)
 * Reuses shared functionality to maintain DRY principle
 */

import { loadConfiguration, saveConfiguration, scanWiFiNetworks } from '../api/setup.js';

export function createSetupStore() {
    return {
        // Basic state
        loading: true,
        error: null,
        saving: false,
        scanning: false,
        initialized: false,
        
        // Minimal config structure for setup
        config: {
            device: {
                owner: '',  // Start blank
                timezone: '',
                wifi: {
                    ssid: '',
                    password: ''
                }
            }
        },
        
        // Manual SSID entry state
        manualSsid: '',
        
        // WiFi networks (reuse from settings)
        availableNetworks: [],
        
        // Load configuration on initialization
        async init() {
            if (this.initialized) return;
            this.initialized = true;
            
            try {
                await this.loadConfiguration();
            } catch (error) {
                console.error('Setup: Initialization error:', error);
            } finally {
                this.loading = false;
            }
        },
        
        // Load configuration from server
        async loadConfiguration() {
            try {
                const response = await loadConfiguration();
                
                this.config.device.owner = response.device?.owner || '';
                this.config.device.timezone = response.device?.timezone || '';
                this.config.device.wifi.ssid = response.device?.wifi?.ssid || '';
                this.config.device.wifi.password = response.device?.wifi?.password || '';
            } catch (error) {
                console.warn('Setup: Using defaults:', error.message);
                this.config.device.owner = '';
                this.config.device.timezone = '';
                this.config.device.wifi.ssid = '';
                this.config.device.wifi.password = '';
            }
        },
        
        // WiFi scanning
        async scanWiFi() {
            if (this.scanning) return;
            
            this.scanning = true;
            try {
                const networks = await scanWiFiNetworks();
                this.availableNetworks = networks;
            } catch (error) {
                console.error('Setup: WiFi scan failed:', error.message);
                window.showMessage('WiFi scan failed: ' + error.message, 'error');
            } finally {
                this.scanning = false;
            }
        },
        
        // Validation for setup form
        get canSave() {
            // Check if "Select network..." is still selected
            if (this.config.device.wifi.ssid === '') {
                return false;
            }
            
            const requiredFields = [
                this.config.device.owner,
                this.config.device.timezone,
                this.getEffectiveSSID(),
                this.config.device.wifi.password
            ];
            
            return requiredFields.every(field => 
                field && typeof field === 'string' && field.trim().length > 0
            );
        },
        
        // Get the effective SSID (either selected or manual)
        getEffectiveSSID() {
            if (this.config.device.wifi.ssid === '__manual__') {
                return this.manualSsid;
            }
            return this.config.device.wifi.ssid;
        },
        
        // Save configuration and restart (setup-specific behavior)
        async saveAndRestart() {
            if (!this.canSave) {
                window.showMessage('Please fill in all required fields', 'error');
                return;
            }
            
            this.saving = true;
            try {
                // Prepare config with effective SSID
                const configToSave = {
                    device: {
                        owner: this.config.device.owner,
                        timezone: this.config.device.timezone,
                        wifi: {
                            ssid: this.getEffectiveSSID(),
                            password: this.config.device.wifi.password
                        }
                    }
                };
                
                // Save configuration
                await saveConfiguration(configToSave);
                
                // No redirect needed - device will restart and disconnect from AP network
                // User will follow the "After Restart" instructions to reconnect
                // Keep overlay showing for a few seconds, then device will restart anyway
                
            } catch (error) {
                console.error('Setup: Save failed:', error.message);
                window.showMessage('Failed to save configuration: ' + error.message, 'error');
                this.saving = false;
            }
        }
    };
}