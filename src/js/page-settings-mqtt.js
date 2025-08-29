/**
 * @file page-settings-mqtt.js
 * @brief Alpine.js store for MQTT settings page
 * @description Focused Alpine store for MQTT-specific configuration
 * Copies organized API functions from main settings store
 */

/**
 * Initialize MQTT Settings Alpine Store
 * Contains only MQTT-related functionality and API calls
 */
function initializeMqttSettingsStore() {
    const store = {
        // ================== UTILITY FUNCTIONS ==================
        showErrorMessage(message) {
            window.showMessage(message, 'error');
        },

        // ================== STATE MANAGEMENT ==================
        loading: true,
        error: null,
        saving: false,
        initialized: false,
        
        // Original configuration for change detection
        originalConfig: null,
        
        // MQTT test connection state
        mqttTesting: false,
        mqttTestResult: null, // { success: boolean, message: string }
        mqttTestPassed: false, // Track if test passed for save validation
        
        // Track if MQTT password field has been modified by user
        mqttPasswordModified: false,
        originalMaskedPassword: '', // Store original masked value
        
        // Configuration data (reactive) - MQTT section only
        config: {
            mqtt: {
                enabled: false,
                server: null,
                port: null,
                username: null,
                password: null
            }
        },
        
        // ================== COMPUTED PROPERTIES ==================
        get canSave() {
            // Must have all required fields if MQTT is enabled
            if (this.config.mqtt.enabled) {
                const hasServer = this.config.mqtt.server && this.config.mqtt.server.trim();
                const hasPort = this.config.mqtt.port && this.config.mqtt.port > 0;
                const testPassed = this.mqttTestPassed;
                return hasServer && hasPort && testPassed && this.hasChanges();
            }
            // If MQTT is disabled, just need to have changes
            return this.hasChanges();
        },

        // ================== CHANGE DETECTION ==================
        hasChanges() {
            if (!this.originalConfig) return false;
            
            return (
                this.config.mqtt.enabled !== this.originalConfig.mqtt.enabled ||
                this.config.mqtt.server !== this.originalConfig.mqtt.server ||
                this.config.mqtt.port !== this.originalConfig.mqtt.port ||
                this.config.mqtt.username !== this.originalConfig.mqtt.username ||
                this.mqttPasswordModified // Password change detected separately
            );
        },

        // ================== PASSWORD HANDLING ==================
        trackMqttPasswordChange(newValue) {
            const isMasked = newValue && newValue.includes('●');
            const hasChanged = newValue !== this.originalMaskedPassword;
            this.mqttPasswordModified = hasChanged && !isMasked;
            
            // Reset test state when password changes
            if (this.mqttPasswordModified) {
                this.resetMqttTestState();
            }
        },

        // ================== MQTT TEST FUNCTIONALITY ==================
        async testMqttConnection() {
            this.mqttTesting = true;
            this.mqttTestResult = null;
            
            try {
                const testData = {
                    server: this.config.mqtt.server,
                    port: this.config.mqtt.port,
                    username: this.config.mqtt.username
                };
                
                // Include password if modified
                if (this.mqttPasswordModified && this.config.mqtt.password) {
                    testData.password = this.config.mqtt.password;
                }
                
                const response = await fetch('/api/test-mqtt', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(testData)
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    this.mqttTestResult = {
                        success: true,
                        message: result.message || 'Successfully connected to MQTT broker'
                    };
                    this.mqttTestPassed = true;
                } else {
                    this.mqttTestResult = {
                        success: false,
                        message: result.error || 'Connection test failed'
                    };
                    this.mqttTestPassed = false;
                }
            } catch (error) {
                console.error('MQTT test error:', error);
                this.mqttTestResult = {
                    success: false,
                    message: 'Network error during connection test'
                };
                this.mqttTestPassed = false;
            } finally {
                this.mqttTesting = false;
            }
        },

        // Reset MQTT test state when configuration changes
        resetMqttTestState() {
            this.mqttTestPassed = false;
            this.mqttTestResult = null;
        },

        // ================== API CALLS ==================
        async init() {
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('📡 MQTT Settings: Already initialized, skipping');
                return;
            }
            this.initialized = true;
            
            this.loading = true;
            try {
                const response = await fetch('/api/config');
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
                }
                
                const data = await response.json();
                
                // Extract MQTT configuration
                this.config.mqtt = {
                    enabled: data.mqtt?.enabled ?? false,
                    server: data.mqtt?.server ?? '',
                    port: data.mqtt?.port ?? 1883,
                    username: data.mqtt?.username ?? '',
                    password: data.mqtt?.password ?? ''
                };
                
                // Store original for change detection
                this.originalConfig = {
                    mqtt: { ...this.config.mqtt }
                };
                
                // Store original masked password for change detection
                this.originalMaskedPassword = this.config.mqtt.password;
                
                console.log('✅ MQTT Settings: Configuration loaded successfully');
                
            } catch (error) {
                console.error('Error loading MQTT config:', error);
                this.error = `Failed to load configuration: ${error.message}`;
            } finally {
                this.loading = false;
            }
        },

        async saveConfig() {
            if (!this.canSave) return;
            
            try {
                this.saving = true;
                
                // Prepare config update with only MQTT fields that have changed
                const configUpdate = {};
                
                // Always include mqtt object if there are any changes
                if (this.hasChanges()) {
                    configUpdate.mqtt = {
                        enabled: this.config.mqtt.enabled,
                        server: this.config.mqtt.server,
                        port: this.config.mqtt.port,
                        username: this.config.mqtt.username
                    };
                    
                    // Include password only if modified
                    if (this.mqttPasswordModified && this.config.mqtt.password) {
                        configUpdate.mqtt.password = this.config.mqtt.password;
                    }
                }
                
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(configUpdate)
                });
                
                const result = await response.json();
                
                if (response.ok) {
                    window.showMessage('MQTT settings saved successfully!', 'success');
                    
                    // Redirect immediately with success parameter
                    window.location.href = '/settings.html?saved=mqtt';
                    
                } else {
                    throw new Error(result.error || 'Unknown error occurred');
                }
            } catch (error) {
                console.error('Error saving MQTT config:', error);
                this.showErrorMessage(`Failed to save MQTT settings: ${error.message}`);
            } finally {
                this.saving = false;
            }
        },

        // Cancel configuration changes
        cancelConfiguration() {
            // Navigate back to settings
            window.location.href = '/settings.html';
        }
    };

    return store;
}

// Auto-register the MQTT store when this script loads
document.addEventListener('alpine:init', () => {
    // Prevent multiple initializations if alpine:init fires multiple times
    if (window.mqttStoreInstance) {
        console.log('📡 MQTT Settings: Store already exists, skipping alpine:init');
        return;
    }
    
    // Create and register MQTT settings store
    const mqttStore = initializeMqttSettingsStore();
    Alpine.store('settingsMqtt', mqttStore);
    
    // Make store available globally for body x-data
    window.mqttStoreInstance = mqttStore;
    
    // Initialize the store immediately during alpine:init
    mqttStore.init();
    
    // Setup Alpine watchers for reactive updates
    Alpine.effect(() => {
        // Reset test state when server/port/username changes
        if (mqttStore.config?.mqtt?.server || mqttStore.config?.mqtt?.port || mqttStore.config?.mqtt?.username) {
            // Only reset if we have original config to compare against
            if (mqttStore.originalConfig) {
                const hasServerChange = mqttStore.config.mqtt.server !== mqttStore.originalConfig.mqtt.server;
                const hasPortChange = mqttStore.config.mqtt.port !== mqttStore.originalConfig.mqtt.port;
                const hasUsernameChange = mqttStore.config.mqtt.username !== mqttStore.originalConfig.mqtt.username;
                
                if (hasServerChange || hasPortChange || hasUsernameChange) {
                    mqttStore.resetMqttTestState();
                }
            }
        }
        
        // Reset test state when MQTT is disabled
        if (mqttStore.config?.mqtt?.enabled === false) {
            mqttStore.resetMqttTestState();
        }
    });
    
    console.log('✅ MQTT Settings Store registered and initialized');
});