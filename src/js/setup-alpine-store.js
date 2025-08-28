/**
 * Alpine.js store for the setup.html page (AP mode initial configuration)
 * Reuses shared functionality from settings store to maintain DRY principle
 */

// Lightweight setup store that focuses only on essential AP mode setup
document.addEventListener('alpine:init', () => {
    // Create setup store instance
    const setupStore = {
        // Basic state
        loading: true,
        error: null,
        saving: false,
        scanning: false,
        
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
            console.log('Setup Store: Initializing...');
            try {
                await this.loadConfiguration();
            } catch (error) {
                console.error('Setup Store: Failed to initialize:', error);
                this.error = error.message;
            } finally {
                this.loading = false;
            }
        },
        
        // Load configuration from server (reuse SettingsAPI)
        async loadConfiguration() {
            console.log('Setup Store: Loading configuration...');
            const response = await window.SettingsAPI.loadConfiguration();
            
            // Extract only the fields we need for setup - keep owner and ssid blank for new setup
            this.config.device.owner = '';  // Always start blank
            this.config.device.timezone = response.device?.timezone || '';
            this.config.device.wifi.ssid = '';  // Always start blank in AP mode
            this.config.device.wifi.password = '';
            
            console.log('Setup Store: Configuration loaded:', this.config);
        },
        
        // WiFi scanning (reuse SettingsAPI)
        async scanWiFi() {
            if (this.scanning) return;
            
            this.scanning = true;
            try {
                console.log('Setup Store: Scanning for WiFi networks...');
                const networks = await window.SettingsAPI.scanWiFiNetworks();
                this.availableNetworks = networks;
                console.log('Setup Store: Found', networks.length, 'networks');
            } catch (error) {
                console.error('Setup Store: WiFi scan failed:', error);
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
                
                console.log('Setup Store: Saving configuration...', configToSave);
                
                // Save configuration (reuse SettingsAPI)
                await window.SettingsAPI.saveConfiguration(configToSave);
                console.log('Setup Store: Configuration saved successfully');
                
                // No redirect needed - device will restart and disconnect from AP network
                // User will follow the "After Restart" instructions to reconnect
                // Keep overlay showing for a few seconds, then device will restart anyway
                
            } catch (error) {
                console.error('Setup Store: Save failed:', error);
                window.showMessage('Failed to save configuration: ' + error.message, 'error');
                this.saving = false;
            }
        }
    };
    
    // Register the store
    Alpine.store('setup', setupStore);
    
    // Initialize when Alpine is ready
    Alpine.data('setupPage', () => ({
        // Expose all store properties as computed getters
        get loading() { return this.$store.setup.loading; },
        get error() { return this.$store.setup.error; },
        get saving() { return this.$store.setup.saving; },
        get scanning() { return this.$store.setup.scanning; },
        get config() { return this.$store.setup.config; },
        get manualSsid() { return this.$store.setup.manualSsid; },
        set manualSsid(value) { this.$store.setup.manualSsid = value; },
        get availableNetworks() { return this.$store.setup.availableNetworks; },
        get canSave() { return this.$store.setup.canSave; },
        
        // Expose store methods
        scanWiFi() { return this.$store.setup.scanWiFi(); },
        saveAndRestart() { return this.$store.setup.saveAndRestart(); },
        
        init() {
            // Initialize the store
            this.$store.setup.init();
        }
    }));
    
    // Make setup store instance available globally for the HTML
    window.setupStoreInstance = {
        get loading() { return Alpine.store('setup').loading; },
        get error() { return Alpine.store('setup').error; },
        get saving() { return Alpine.store('setup').saving; },
        get scanning() { return Alpine.store('setup').scanning; },
        get config() { return Alpine.store('setup').config; },
        get manualSsid() { return Alpine.store('setup').manualSsid; },
        set manualSsid(value) { Alpine.store('setup').manualSsid = value; },
        get availableNetworks() { return Alpine.store('setup').availableNetworks; },
        get canSave() { return Alpine.store('setup').canSave; },
        scanWiFi() { return Alpine.store('setup').scanWiFi(); },
        saveAndRestart() { return Alpine.store('setup').saveAndRestart(); },
        init() { return Alpine.store('setup').init(); }
    };
});

console.log('Setup Alpine Store loaded');