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
        loading: true,
        error: null,
        saving: false,
        initialized: false,
        apPrintStatus: 'normal', // 'normal', 'scribing'
        
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
        
        // WiFi network scanning state using Alpine reactive patterns
        wifiScan: {
            // Core state
            networks: [],
            currentSSID: null,
            selectedNetwork: null,
            isScanning: false,
            error: null,
            hasScanned: false,
            
            // Computed properties (Alpine getters)
            get dropdownDisabled() {
                return this.isScanning;
            },
            
            get showManualEntry() {
                return this.selectedNetwork === 'manual';
            },
            
            get scanLabel() {
                return this.hasScanned ? 'Rescan networks' : 'Scan networks for more';
            },
            
            // Reactive options
            get options() {
                const options = [];
                
                // Helper function to get signal strength for a network
                const getNetworkSignalStrength = (ssid) => {
                    const network = this.networks.find(n => n.ssid === ssid);
                    return network ? network.signal_strength : null;
                };
                
                // Always show current SSID first (if available)
                if (this.currentSSID) {
                    const currentSignalStrength = getNetworkSignalStrength(this.currentSSID);
                    const currentLabel = currentSignalStrength 
                        ? `${this.currentSSID} (${currentSignalStrength}) ← active`
                        : `${this.currentSSID} ← active`;
                    
                    options.push({
                        value: this.currentSSID,
                        label: currentLabel,
                        disabled: false
                    });
                }
                
                // Add scanned networks (excluding current to avoid duplicates)
                if (this.hasScanned) {
                    // Store reference to formatNetworkDisplay to avoid this context issues
                    const formatNetworkDisplay = (network) => {
                        return `${network.ssid} (${network.signal_strength})`;
                    };
                    
                    this.networks
                        .filter(network => network.ssid !== this.currentSSID)
                        .forEach(network => {
                            options.push({
                                value: network.ssid,
                                label: formatNetworkDisplay(network),
                                disabled: false
                            });
                        });
                }
                
                // Always show manual entry
                options.push({
                    value: 'manual',
                    label: 'Type It Out (Manual entry)',
                    disabled: false
                });
                
                // Show scan/rescan option (or scanning status)
                if (this.isScanning) {
                    options.push({
                        value: '_scanning',
                        label: "Scanning the airwaves...",
                        disabled: true
                    });
                } else {
                    options.push({
                        value: '_scan',
                        label: `${this.scanLabel}`,
                        disabled: false
                    });
                }
                
                return options;
            }
        },
        
        // Validation state
        validation: {
            errors: {}
        },
        
        // Computed properties for complex UI states
        get apPrintButtonText() {
            return this.apPrintStatus === 'scribing' ? 'Scribing' : 'Scribe WiFi Fallback AP';
        },

        // ================== WIFI CONFIGURATION API ==================
        // Initialize store with data from server
        async init() {
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
            this.wifiScan.networks = [];
            this.wifiScan.isScanning = false;
            this.wifiScan.hasScanned = false;
            this.wifiScan.error = null;
        },

        // ================== WIFI API ==================
        // WiFi scanning - simplified with reactive updates
        async scanWiFiNetworks() {
            this.wifiScan.isScanning = true;
            this.wifiScan.error = null;
            
            try {
                const networks = await window.SettingsAPI.scanWiFiNetworks();
                
                // Deduplicate networks by SSID, keeping the strongest signal for each
                const uniqueNetworks = [];
                const seenSSIDs = new Set();
                
                networks
                    .sort((a, b) => b.rssi - a.rssi) // Sort by strongest signal first
                    .forEach(network => {
                        if (!seenSSIDs.has(network.ssid)) {
                            seenSSIDs.add(network.ssid);
                            uniqueNetworks.push(network);
                        }
                    });
                
                // Update state - Alpine reactivity handles UI updates
                this.wifiScan.networks = uniqueNetworks;
                this.wifiScan.hasScanned = true;
                
                // Always reselect current SSID after scan if it's found in the results
                if (this.wifiScan.currentSSID) {
                    const currentNetwork = uniqueNetworks.find(n => n.ssid === this.wifiScan.currentSSID);
                    if (currentNetwork) {
                        this.wifiScan.selectedNetwork = this.wifiScan.currentSSID;
                        console.log('Auto-reselected current network after scan:', this.wifiScan.currentSSID);
                    }
                }
                
                console.log('WiFi scan completed:', uniqueNetworks.length, 'unique networks found from', networks.length, 'total scanned');
                
            } catch (error) {
                console.error('WiFi scan failed:', error);
                this.wifiScan.error = error.message;
                this.showErrorMessage(`WiFi scan failed: ${error.message}`);
            } finally {
                this.wifiScan.isScanning = false;
            }
        },
        
        // Handle network selection from dropdown - State machine transitions
        // Simplified network selection - let Alpine reactivity handle state
        selectNetwork(value) {
            if (value === '_scan') {
                // Trigger scan but don't change selection
                this.scanWiFiNetworks();
                return;
            }
            
            if (value === '_scanning') {
                // Ignore scanning placeholder selection
                return;
            }
            
            // Update selected network - Alpine reactivity handles the rest
            this.wifiScan.selectedNetwork = value;
            
            // Update config based on selection
            if (value === 'manual') {
                this.config.device.wifi.ssid = ''; // Clear for manual input
            } else if (value && value !== '') {
                this.config.device.wifi.ssid = value;
            }
            
            // Clear validation errors
            if (this.validation.errors['wifi.ssid']) {
                delete this.validation.errors['wifi.ssid'];
            }
        },

        // Save WiFi configuration via API
        async saveConfiguration() {
            this.saving = true;
            try {
                // Create partial WiFi config for server submission
                const partialConfig = {
                    device: {
                        wifi: {
                            ssid: this.config.device.wifi.ssid,
                            connect_timeout: this.config.device.wifi.connect_timeout
                        }
                    }
                };
                
                // Only include password if it was modified by user (not masked)
                if (this.passwordsModified.wifiPassword) {
                    partialConfig.device.wifi.password = this.config.device.wifi.password;
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
    // Prevent multiple initializations if alpine:init fires multiple times
    if (window.wifiStoreInstance) {
        console.log('📡 WiFi Settings: Store already exists, skipping alpine:init');
        return;
    }
    
    // Create and register WiFi settings store
    const wifiStore = initializeWiFiSettingsStore();
    Alpine.store('settingsWifi', wifiStore);
    
    // Make store available globally for body x-data
    window.wifiStoreInstance = wifiStore;
    
    // Initialize the store immediately during alpine:init
    wifiStore.init();
    
    console.log('✅ WiFi Settings Store registered and initialized');
});