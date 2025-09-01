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
        loaded: false,  // Simple loading flag (starts false)
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
        
        // Configuration data (reactive) - Empty object (populated on load)
        config: {},
        
        // Validation state
        validation: {
            errors: {}
        },
        
        // ================== COMPUTED PROPERTIES ==================
        get canSave() {
            // Must have valid configuration if MQTT is enabled
            if (this.config.mqtt.enabled) {
                const isValid = this.validateConfiguration();
                const testPassed = this.mqttTestPassed;
                return isValid && testPassed && this.hasChanges();
            }
            // If MQTT is disabled, just need to have changes
            return this.hasChanges();
        },

        get testButtonLabel() {
            if (this.mqttTesting) {
                return 'Testing...';
            } else if (this.mqttTestPassed) {
                return 'MQTT Connected';
            } else if (this.mqttTestResult && !this.mqttTestResult.success) {
                return 'Connection Failed';
            } else {
                return 'Test Connection';
            }
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

        // ================== VALIDATION ==================
        // Validate MQTT server field
        validateServer(value) {
            if (this.config.mqtt.enabled && (!value || value.trim() === '')) {
                this.validation.errors['mqtt.server'] = 'MQTT server cannot be blank when enabled';
            } else {
                if (this.validation.errors['mqtt.server']) {
                    delete this.validation.errors['mqtt.server'];
                }
            }
        },

        // Validate MQTT port field  
        validatePort(value) {
            if (this.config.mqtt.enabled) {
                const port = parseInt(value);
                if (isNaN(port) || port < 1 || port > 65535) {
                    this.validation.errors['mqtt.port'] = 'Port must be between 1-65535';
                } else {
                    if (this.validation.errors['mqtt.port']) {
                        delete this.validation.errors['mqtt.port'];
                    }
                }
            } else {
                if (this.validation.errors['mqtt.port']) {
                    delete this.validation.errors['mqtt.port'];
                }
            }
        },

        // Validate MQTT username field
        validateUsername(value) {
            if (this.config.mqtt.enabled && (!value || value.trim() === '')) {
                this.validation.errors['mqtt.username'] = 'Username cannot be blank when MQTT enabled';
            } else {
                if (this.validation.errors['mqtt.username']) {
                    delete this.validation.errors['mqtt.username'];
                }
            }
        },

        // Validate MQTT password field
        validatePassword(value) {
            if (this.config.mqtt.enabled && (!value || value.trim() === '')) {
                this.validation.errors['mqtt.password'] = 'Password cannot be blank when MQTT enabled';
            } else {
                if (this.validation.errors['mqtt.password']) {
                    delete this.validation.errors['mqtt.password'];
                }
            }
        },

        // Validate current MQTT configuration
        validateConfiguration() {
            const errors = {};
            
            if (this.config.mqtt.enabled) {
                if (!this.config.mqtt.server || this.config.mqtt.server.trim() === '') {
                    errors['mqtt.server'] = 'MQTT server cannot be blank when enabled';
                }
                
                const port = parseInt(this.config.mqtt.port);
                if (isNaN(port) || port < 1 || port > 65535) {
                    errors['mqtt.port'] = 'Port must be between 1-65535';
                }
                
                if (!this.config.mqtt.username || this.config.mqtt.username.trim() === '') {
                    errors['mqtt.username'] = 'Username cannot be blank when MQTT enabled';
                }
                
                if (!this.config.mqtt.password || this.config.mqtt.password.trim() === '') {
                    errors['mqtt.password'] = 'Password cannot be blank when MQTT enabled';
                }
            }
            
            this.validation.errors = errors;
            return Object.keys(errors).length === 0;
        },

        // ================== PASSWORD HANDLING ==================
        clearPasswordFieldOnFocus() {
            // Clear password field on focus (standard UX pattern)
            if (this.config.mqtt.password) {
                this.config.mqtt.password = '';
                this.mqttPasswordModified = true;
                this.resetMqttTestState();
            }
        },

        trackMqttPasswordChange(newValue) {
            const hasChanged = newValue !== this.originalMaskedPassword;
            this.mqttPasswordModified = hasChanged;
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
                
                // Include password only if modified by user
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
                
                if (response.ok) {
                    this.mqttTestResult = {
                        success: true,
                        message: 'Successfully connected to MQTT broker'
                    };
                    this.mqttTestPassed = true;
                } else {
                    // For error responses, try to parse JSON for error message
                    try {
                        const result = await response.json();
                        this.mqttTestResult = {
                            success: false,
                            message: result.error || 'Connection test failed'
                        };
                    } catch (parseError) {
                        this.mqttTestResult = {
                            success: false,
                            message: 'Connection test failed'
                        };
                    }
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
        async loadConfiguration() {
            // Duplicate initialization guard (failsafe)
            if (this.initialized) {
                return;
            }
            this.initialized = true;
            
            this.loaded = false;
            this.error = null;
            try {
                const response = await fetch('/api/config');
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
                }
                
                const data = await response.json();
                
                // Extract MQTT configuration - Direct assignment
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
                
                this.loaded = true;
                
            } catch (error) {
                this.error = `Failed to load configuration: ${error.message}`;
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
                
                if (response.ok) {
                    // Redirect immediately with success parameter (don't reset saving state)
                    window.location.href = '/settings/?saved=mqtt';
                    return; // Exit without resetting saving state
                } else {
                    // Parse error response for error message
                    const result = await response.json().catch(() => ({}));
                    throw new Error(result.error || 'Unknown error occurred');
                }
            } catch (error) {
                console.error('Error saving MQTT config:', error);
                this.showErrorMessage(`Failed to save MQTT settings: ${error.message}`);
                this.saving = false; // Only reset on error
            }
        },

        // Cancel configuration changes
        cancelConfiguration() {
            // Navigate back to settings
            window.location.href = '/settings/';
        }
    };

    return store;
}

// Auto-register the MQTT store when this script loads
document.addEventListener('alpine:init', () => {
    // Create and register MQTT settings store
    const mqttStore = initializeMqttSettingsStore();
    Alpine.store('settingsMqtt', mqttStore);
    
    // No manual init() - let HTML handle initialization timing with x-init
    
    // Setup Alpine watchers for reactive updates
    Alpine.effect(() => {
        // Clear validation errors and reset test state when MQTT is disabled
        if (mqttStore.config?.mqtt?.enabled === false) {
            mqttStore.validation.errors = {};
            mqttStore.resetMqttTestState();
        }
    });
    
    console.log('✅ MQTT Settings Store registered and initialized');
});