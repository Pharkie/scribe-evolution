/**
 * @file page-settings-wifi.js
 * @brief Alpine.js store for WiFi settings page
 * @description Focused Alpine store for WiFi-specific configuration
 * Copies organized WiFi API functions from main settings store
 */

/**
 * Initialize WiFi Settings Alpine Store
 * Contains only WiFi-related functionality and API calls
 */
function initializeWiFiSettingsStore() {
    const store = {
        // ================== UTILITY FUNCTIONS ==================
        // Simple utility function extracted from repeated showMessage patterns
        showErrorMessage(message) {
            window.showMessage(message, 'error');
        },

        // ================== STATE MANAGEMENT ==================
        // Core state management
        loading: true,  // Start as loading for fade-in effect
        error: null,
        saving: false,
        initialized: false,
        apPrintStatus: 'normal', // 'normal', 'scribing'
        
        // Computed property to check if save should be enabled
        get canSave() {
            // Don't allow save while loading, saving, or with errors
            if (this.loading || this.saving || this.error) {
                return false;
            }
            
            const selectedSSID = this.wifiScan.mode === 'manual' ? this.wifiScan.manualSSID : this.wifiScan.selectedNetwork;
            const hasValidSSID = selectedSSID && selectedSSID.trim() !== '';
            
            // Must have valid SSID
            let formValid = false;
            if (this.wifiScan.mode === 'scan') {
                formValid = hasValidSSID && this.wifiScan.selectedNetwork;
            } else if (this.wifiScan.mode === 'manual') {
                formValid = hasValidSSID;
            }
            
            if (!formValid) return false;
            
            // If password was modified, it must not be blank
            if (this.passwordsModified.wifiPassword && (!this.config.device.wifi.password || this.config.device.wifi.password.trim() === '')) {
                return false;
            }
            
            // Must have changes to save
            return this.hasChanges();
        },
        
        // Configuration data (reactive) - WiFi section
        config: {
            device: {
                wifi: {
                    ssid: null,
                    password: null,
                    connect_timeout: null,
                    status: null,
                    fallback_ap_ssid: null,
                    fallback_ap_password: null
                },
                mdns: null
            }
        },
        
        // Password modification tracking for secure handling
        passwordsModified: {
            wifiPassword: false
        },
        originalMaskedValues: {
            wifiPassword: ''
        },
        
        // Store original config to detect changes
        originalConfig: {
            connectTimeout: null
        },
        
        // WiFi network scanning state using Alpine reactive patterns
        wifiScan: {
            // Core state
            networks: [],
            currentSSID: null,
            selectedNetwork: null,
            manualSSID: '',
            mode: 'scan', // 'scan' or 'manual'
            isScanning: false,
            error: null,
            hasScanned: false,
            passwordVisible: false,
            
            // Computed properties (Alpine getters)
            get sortedNetworks() {
                if (!this.networks || this.networks.length === 0) return [];
                
                // Convert RSSI to signal strength and add unique keys
                // Networks are already deduped and sorted from the scan function
                const networksWithSignal = this.networks.map((network, index) => ({
                    ...network,
                    signal_strength: this.formatSignalStrength(network.rssi),
                    signal_display: `${this.formatSignalStrength(network.rssi)} (${network.rssi} dBm)`,
                    uniqueKey: `${network.ssid}-${network.rssi}-${index}` // Unique key for Alpine rendering
                }));
                
                return networksWithSignal;
            },
            
            // Format RSSI to signal strength description
            formatSignalStrength(rssi) {
                if (rssi > -30) return 'Excellent';
                if (rssi > -50) return 'Very Good';
                if (rssi > -60) return 'Good';
                if (rssi > -70) return 'Fair';
                return 'Poor';
            },
            
        },
        
        // Validation state
        validation: {
            errors: {}
        },
        
        // Computed properties for complex UI states
        get apPrintButtonText() {
            return this.apPrintStatus === 'scribing' ? 'Scribing' : 'Scribe WiFi Fallback AP';
        },
        
        get saveButtonText() {
            return this.saving ? 'Saving...' : 'Save';
        },

        // ================== WIFI CONFIGURATION API ==================
        // Load configuration data from server
        async loadConfiguration() {
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('📡 WiFi Settings: Already initialized, skipping');
                return;
            }
            this.initialized = true;
            
            this.loading = true;
            try {
                // Load configuration from API
                const serverConfig = await window.SettingsAPI.loadConfiguration();
                
                // Extract only WiFi-related configuration
                this.mergeWiFiConfig(serverConfig);
                
                // Initialize WiFi scanning state
                this.initializeWiFiState();
                
                // Initialize password tracking
                this.initializePasswordTracking();
                
                console.log('Alpine WiFi Store: Configuration loaded successfully');
                
            } catch (error) {
                console.error('Alpine WiFi Store: Failed to load configuration:', error);
                this.error = error.message;
            } finally {
                this.loading = false;
            }
        },

        // Merge server config into reactive state (WiFi section only)
        mergeWiFiConfig(serverConfig) {
            console.log('📡 Merging WiFi config from server:', serverConfig);
            
            // Device WiFi configuration
            if (serverConfig.device && serverConfig.device.wifi) {
                this.config.device.wifi.ssid = serverConfig.device.wifi.ssid || '';
                this.config.device.wifi.password = serverConfig.device.wifi.password || '';
                this.config.device.wifi.connect_timeout = serverConfig.device.wifi.connect_timeout || 15000;
                this.config.device.wifi.status = serverConfig.device.wifi.status || null;
                this.config.device.wifi.fallback_ap_ssid = serverConfig.device.wifi.fallback_ap_ssid || '';
                this.config.device.wifi.fallback_ap_password = serverConfig.device.wifi.fallback_ap_password || '';
                
                if (!serverConfig.device.wifi.ssid) {
                    console.warn('⚠️ Missing device.wifi.ssid in config');
                }
            } else {
                console.error('❌ Missing device.wifi section in config');
            }
            
            // mDNS hostname
            if (serverConfig.device) {
                this.config.device.mdns = serverConfig.device.mdns || '';
            }
            
            console.log('✅ WiFi config merge complete:', this.config);
        },
        
        // Initialize password tracking
        initializePasswordTracking() {
            // Store original masked values for comparison
            this.originalMaskedValues.wifiPassword = this.config.device.wifi.password || '';
            
            // Store original timeout for change detection
            this.originalConfig.connectTimeout = this.config.device.wifi.connect_timeout || 15000;
        },
        
        // Track password modifications (called from templates)
        trackWifiPasswordChange(newValue) {
            const isMasked = newValue && newValue.includes('●');
            const hasChanged = newValue !== this.originalMaskedValues.wifiPassword;
            this.passwordsModified.wifiPassword = hasChanged && !isMasked;
        },
        
        // Initialize WiFi state - simplified
        initializeWiFiState() {
            this.wifiScan.currentSSID = this.config?.device?.wifi?.ssid || null;
            this.wifiScan.selectedNetwork = this.wifiScan.currentSSID;
            this.wifiScan.manualSSID = '';
            this.wifiScan.mode = 'scan';
            this.wifiScan.networks = [];
            this.wifiScan.isScanning = false;
            this.wifiScan.hasScanned = false;
            this.wifiScan.error = null;
            this.wifiScan.passwordVisible = false;
        },

        // ================== WIFI API ==================
        // WiFi scanning - simplified with reactive updates
        async scanWiFiNetworks() {
            this.wifiScan.isScanning = true;
            this.wifiScan.error = null;
            
            try {
                const networks = await window.SettingsAPI.scanWiFiNetworks();
                
                // Filter valid networks and dedupe by SSID (keep strongest signal only)
                const networksBySSID = {};
                
                networks
                    .filter(network => network.ssid && network.ssid.trim())
                    .forEach(network => {
                        const ssid = network.ssid.trim();
                        if (!networksBySSID[ssid] || network.rssi > networksBySSID[ssid].rssi) {
                            networksBySSID[ssid] = network;
                        }
                    });
                
                // Convert to array and sort by signal strength (strongest first), then alphabetically
                const validNetworks = Object.values(networksBySSID)
                    .sort((a, b) => {
                        if (b.rssi !== a.rssi) {
                            return b.rssi - a.rssi;
                        }
                        return a.ssid.localeCompare(b.ssid);
                    });
                
                // Update state - Alpine reactivity handles UI updates
                this.wifiScan.networks = validNetworks;
                this.wifiScan.hasScanned = true;
                
                // Switch to scan mode and auto-select current network if found
                this.wifiScan.mode = 'scan';
                if (this.wifiScan.currentSSID) {
                    const currentNetwork = validNetworks.find(n => n.ssid === this.wifiScan.currentSSID);
                    if (currentNetwork) {
                        this.wifiScan.selectedNetwork = this.wifiScan.currentSSID;
                        console.log('Auto-reselected current network after scan:', this.wifiScan.currentSSID);
                    }
                }
                
                console.log('WiFi scan completed:', validNetworks.length, 'valid networks found from', networks.length, 'total scanned');
                
            } catch (error) {
                console.error('WiFi scan failed:', error);
                this.wifiScan.error = error.message;
                this.showErrorMessage(`WiFi scan failed: ${error.message}`);
            } finally {
                this.wifiScan.isScanning = false;
            }
        },
        
        // Update SSID based on current mode and selection
        updateSSID() {
            const selectedSSID = this.wifiScan.mode === 'manual' ? this.wifiScan.manualSSID : this.wifiScan.selectedNetwork;
            this.config.device.wifi.ssid = selectedSSID || '';
            
            // Clear validation errors when SSID changes
            if (this.validation.errors['wifi.ssid'] && selectedSSID) {
                delete this.validation.errors['wifi.ssid'];
            }
        },

        // Validate password field specifically (called from UI)
        validatePassword(value) {
            // Only validate if password was modified (to avoid showing error on pre-filled masked passwords)
            if (this.passwordsModified.wifiPassword && (!value || value.trim() === '')) {
                this.validation.errors['wifi.password'] = 'Password cannot be blank';
            } else {
                // Clear the error if it was previously set
                if (this.validation.errors['wifi.password']) {
                    delete this.validation.errors['wifi.password'];
                }
            }
        },
        
        // Validate timeout field specifically (called from UI)
        validateTimeout(value) {
            const timeoutSeconds = parseInt(value);
            if (isNaN(timeoutSeconds) || timeoutSeconds < 5 || timeoutSeconds > 60) {
                this.validation.errors['wifi.connect_timeout'] = 'Timeout must be between 5-60 seconds';
            } else {
                // Clear the error if it was previously set
                if (this.validation.errors['wifi.connect_timeout']) {
                    delete this.validation.errors['wifi.connect_timeout'];
                }
            }
        },
        
        // Validate current configuration
        validateConfiguration() {
            const errors = {};
            
            // Get selected SSID based on current mode
            const selectedSSID = this.wifiScan.mode === 'manual' ? this.wifiScan.manualSSID : this.wifiScan.selectedNetwork;
            
            // SSID validation
            if (!selectedSSID || selectedSSID.trim() === '') {
                if (this.wifiScan.mode === 'scan') {
                    errors['wifi.ssid'] = 'Please select a network';
                } else {
                    errors['wifi.ssid'] = 'Network name cannot be empty';
                }
            }
            
            // Password validation (if network requires it)
            // Note: We can't determine if a network requires a password in scan mode
            // without additional API data, so we'll be lenient here
            
            // Timeout validation
            const timeoutSeconds = Math.floor(this.config.device.wifi.connect_timeout / 1000);
            if (timeoutSeconds < 5 || timeoutSeconds > 60) {
                errors['wifi.connect_timeout'] = 'Timeout must be between 5-60 seconds';
            }
            
            this.validation.errors = errors;
            return Object.keys(errors).length === 0;
        },
        
        // Check if configuration has meaningful changes
        hasChanges() {
            const selectedSSID = this.wifiScan.mode === 'manual' ? this.wifiScan.manualSSID : this.wifiScan.selectedNetwork;
            const currentSSID = this.wifiScan.currentSSID;
            
            // SSID changed
            if (selectedSSID !== currentSSID) {
                return true;
            }
            
            // Password modified
            if (this.passwordsModified.wifiPassword) {
                return true;
            }
            
            // Timeout changed (compare current timeout vs original)
            const currentTimeout = this.config.device.wifi.connect_timeout || 15000;
            const originalTimeout = this.originalConfig.connectTimeout || 15000;
            if (currentTimeout !== originalTimeout) {
                // Only consider it a valid change if the timeout is within valid range
                const timeoutSeconds = Math.floor(currentTimeout / 1000);
                if (timeoutSeconds >= 5 && timeoutSeconds <= 60) {
                    return true;
                }
            }
            
            return false;
        },

        // Save WiFi configuration via API
        async saveConfiguration() {
            // Ensure SSID is up to date
            this.updateSSID();
            
            // Validate before saving
            if (!this.validateConfiguration()) {
                this.showErrorMessage('Please fix the errors before saving');
                return;
            }
            
            
            this.saving = true;
            try {
                // Create partial WiFi config for server submission - flat structure
                const partialConfig = {
                    wifi: {
                        ssid: this.config.device.wifi.ssid,
                        connect_timeout: this.config.device.wifi.connect_timeout
                    }
                };
                
                // Only include password if it was modified by user (not masked)
                if (this.passwordsModified.wifiPassword) {
                    partialConfig.wifi.password = this.config.device.wifi.password;
                }
                
                console.log('Saving partial WiFi configuration:', partialConfig);
                const message = await window.SettingsAPI.saveConfiguration(partialConfig);
                
                console.log('Alpine WiFi Store: Configuration saved successfully');
                
                // Redirect immediately with success parameter
                window.location.href = '/settings.html?saved=wifi';
                
            } catch (error) {
                console.error('Alpine WiFi Store: Failed to save configuration:', error);
                this.showErrorMessage('Failed to save WiFi settings: ' + error.message);
                this.saving = false; // Only reset on error
            }
        },

        // Cancel configuration changes
        cancelConfiguration() {
            // Navigate back to settings
            window.location.href = '/settings.html';
        },
        
        // ================== SYSTEM/PRINTING API ==================
        // Print AP details to thermal printer
        async printAPDetails() {
            try {
                // Set scribing state
                this.apPrintStatus = 'scribing';
                
                // Get fallback AP details from configuration - error if not available
                const fallbackSSID = this.config?.device?.wifi?.fallback_ap_ssid;
                const fallbackPassword = this.config?.device?.wifi?.fallback_ap_password;
                
                if (!fallbackSSID || !fallbackPassword) {
                    throw new Error('WiFi fallback AP credentials not configured');
                }
                
                // Create print request with AP details
                const printRequest = {
                    content_type: "memo",
                    content: {
                        title: "WiFi Fallback AP",
                        text: `Network: ${fallbackSSID}
Password: ${fallbackPassword}

Connect to this network if device WiFi fails.

Device will be available at:
http://192.168.4.1

This memo printed from Settings → WiFi`
                    }
                };
                
                // Submit print request using local content
                const content = `WiFi Fallback AP
                
Network: ${fallbackSSID}
Password: ${fallbackPassword}

Connect to this network if device WiFi fails.

Device will be available at:
http://192.168.4.1

This memo printed from Settings → WiFi`;
                
                await window.SettingsAPI.printLocalContent(content);
                
                console.log('AP details print request submitted successfully');
                
                // Show "Scribing" for 2 seconds then revert to normal
                setTimeout(() => {
                    this.apPrintStatus = 'normal';
                }, 2000);
                
            } catch (error) {
                console.error('Failed to print AP details:', error);
                // Reset to normal state and show error to user
                this.apPrintStatus = 'normal';
                
                // Use fallback if showMessage is not available
                if (typeof window.showMessage === 'function') {
                    window.showMessage(`Failed to print AP details: ${error.message}`, 'error');
                } else {
                    alert(`Failed to print AP details: ${error.message}`);
                }
            }
        }
    };
    
    return store;
}

// Auto-register the WiFi store when this script loads
document.addEventListener('alpine:init', () => {
    // Create and register WiFi settings store
    const wifiStore = initializeWiFiSettingsStore();
    Alpine.store('settingsWifi', wifiStore);
    
    // No manual init() - let HTML handle initialization timing with x-init
    
    // Setup Alpine watchers for reactive updates
    Alpine.effect(() => {
        // Update SSID whenever mode or selection changes
        wifiStore.updateSSID();
    });
    
    console.log('✅ WiFi Settings Store registered');
});