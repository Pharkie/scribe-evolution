/**
 * @file settings-alpine-store.js
 * @brief Alpine.js data store for settings form state management
 * @description Reactive data store that wraps existing SettingsAPI while maintaining compatibility
 */

/**
 * Initialize Alpine.js settings store with reactive data
 * Preserves existing API layer while adding reactive state management
 */
function initializeSettingsStore() {
    return {
        // Loading states
        loading: false,
        saving: false,
        
        // Configuration data (reactive)
        config: {
            device: {
                owner: '',
                timezone: ''
            },
            wifi: {
                ssid: '',
                password: '',
                connect_timeout: 15000
            },
            mqtt: {
                server: '',
                port: 1883,
                username: '',
                password: ''
            },
            validation: {
                maxCharacters: 1000
            },
            apis: {
                chatgptApiToken: ''
            },
            unbiddenInk: {
                enabled: false,
                startHour: 8,
                endHour: 22,
                frequencyMinutes: 120,
                prompt: ''
            },
            buttons: {
                button1: { shortAction: '', longAction: '', shortMqttTopic: '', longMqttTopic: '' },
                button2: { shortAction: '', longAction: '', shortMqttTopic: '', longMqttTopic: '' },
                button3: { shortAction: '', longAction: '', shortMqttTopic: '', longMqttTopic: '' },
                button4: { shortAction: '', longAction: '', shortMqttTopic: '', longMqttTopic: '' }
            },
            leds: {
                pin: 4,
                count: 60,
                brightness: 128,
                refreshRate: 60
            }
        },
        
        // Form validation states
        validation: {
            errors: {},
            touched: {},
            isValid: true
        },
        
        // UI state management
        ui: {
            activeSection: 'wifi',
            showValidationFeedback: false
        },
        
        // Computed properties for complex UI states
        get timeRangeDisplay() {
            const start = this.config.unbiddenInk.startHour;
            const end = this.config.unbiddenInk.endHour;
            
            if (start === 0 && (end === 0 || end === 24)) {
                return 'All Day';
            }
            
            return `${this.formatHour(start)} - ${this.formatHour(end)}`;
        },
        
        get frequencyDisplay() {
            const minutes = this.config.unbiddenInk.frequencyMinutes;
            const hours = Math.floor(minutes / 60);
            const mins = minutes % 60;
            
            const start = this.config.unbiddenInk.startHour;
            const end = this.config.unbiddenInk.endHour;
            
            let timeText = '';
            if (start === 0 && (end === 0 || end === 24)) {
                timeText = 'all day long';
            } else {
                const startAmPm = this.formatHour12(start);
                const endAmPm = this.formatHour12(end);
                timeText = `from ${startAmPm} to ${endAmPm}`;
            }
            
            if (hours > 0 && mins > 0) {
                return `Every ${hours}h ${mins}m ${timeText}`;
            } else if (hours > 0) {
                return `Every ${hours}h ${timeText}`;
            } else {
                return `Every ${mins}m ${timeText}`;
            }
        },
        
        get timeRangeStyle() {
            const start = this.config.unbiddenInk.startHour;
            const end = this.config.unbiddenInk.endHour;
            
            if (start === 0 && (end === 0 || end === 24)) {
                return { left: '0%', width: '100%' };
            }
            
            const startPercent = (start / 24) * 100;
            const endPercent = (end / 24) * 100;
            
            return {
                left: `${startPercent}%`,
                width: `${endPercent - startPercent}%`
            };
        },
        
        // Initialize store with data from server
        async init() {
            this.loading = true;
            try {
                // Use existing SettingsAPI
                const serverConfig = await window.SettingsAPI.loadConfiguration();
                
                // Deep merge server config into reactive state
                this.mergeConfig(serverConfig);
                
                console.log('Alpine Store: Configuration loaded successfully');
            } catch (error) {
                console.error('Alpine Store: Failed to load configuration:', error);
                this.showMessage('Failed to load configuration: ' + error.message, 'error');
            } finally {
                this.loading = false;
            }
        },
        
        // Save configuration to server
        async saveConfiguration() {
            // Validate form before saving
            if (!this.validateForm()) {
                this.ui.showValidationFeedback = true;
                this.showMessage('Please fix the errors in the form before saving', 'error');
                return;
            }
            
            this.saving = true;
            try {
                // Use existing SettingsAPI with reactive config
                const message = await window.SettingsAPI.saveConfiguration(this.config);
                
                this.showMessage(message, 'success');
                this.ui.showValidationFeedback = false;
                
                // Trigger LED confirmation effect if available
                if (window.SettingsLED && window.SettingsLED.triggerEffect) {
                    window.SettingsLED.triggerEffect('simple_chase', 'green', 3000);
                }
                
                console.log('Alpine Store: Configuration saved successfully');
            } catch (error) {
                console.error('Alpine Store: Failed to save configuration:', error);
                this.showMessage('Failed to save configuration: ' + error.message, 'error');
            } finally {
                this.saving = false;
            }
        },
        
        // Helper methods
        formatHour(hour) {
            if (hour === 0) return '00:00 (All Day)';
            if (hour === 24) return '24:00 (All Day)';
            return hour.toString().padStart(2, '0') + ':00';
        },
        
        formatHour12(hour) {
            if (hour === 0 || hour === 24) return '12 am';
            if (hour === 12) return '12 pm';
            if (hour < 12) return `${hour} am`;
            return `${hour - 12} pm`;
        },
        
        // Deep merge server config into reactive state
        mergeConfig(serverConfig) {
            // Device
            if (serverConfig.device) {
                if (serverConfig.device.owner === undefined) {
                    throw new Error('Device owner configuration is missing from server');
                }
                if (serverConfig.device.timezone === undefined) {
                    throw new Error('Device timezone configuration is missing from server');
                }
                this.config.device.owner = serverConfig.device.owner;
                this.config.device.timezone = serverConfig.device.timezone;
            }
            
            // WiFi
            if (serverConfig.wifi) {
                if (serverConfig.wifi.ssid === undefined) {
                    throw new Error('WiFi SSID configuration is missing from server');
                }
                if (serverConfig.wifi.password === undefined) {
                    throw new Error('WiFi password configuration is missing from server');
                }
                if (serverConfig.wifi.connect_timeout === undefined) {
                    throw new Error('WiFi connect timeout configuration is missing from server');
                }
                this.config.wifi.ssid = serverConfig.wifi.ssid;
                this.config.wifi.password = serverConfig.wifi.password;
                this.config.wifi.connect_timeout = serverConfig.wifi.connect_timeout;
            }
            
            // MQTT
            if (serverConfig.mqtt) {
                if (serverConfig.mqtt.server === undefined) {
                    throw new Error('MQTT server configuration is missing from server');
                }
                if (serverConfig.mqtt.port === undefined) {
                    throw new Error('MQTT port configuration is missing from server');
                }
                if (serverConfig.mqtt.username === undefined) {
                    throw new Error('MQTT username configuration is missing from server');
                }
                if (serverConfig.mqtt.password === undefined) {
                    throw new Error('MQTT password configuration is missing from server');
                }
                this.config.mqtt.server = serverConfig.mqtt.server;
                this.config.mqtt.port = serverConfig.mqtt.port;
                this.config.mqtt.username = serverConfig.mqtt.username;
                this.config.mqtt.password = serverConfig.mqtt.password;
            }
            
            // Validation
            if (serverConfig.validation) {
                if (serverConfig.validation.maxCharacters === undefined) {
                    throw new Error('Validation maxCharacters configuration is missing from server');
                }
                this.config.validation.maxCharacters = serverConfig.validation.maxCharacters;
            }
            
            // APIs
            if (serverConfig.apis) {
                if (serverConfig.apis.chatgptApiToken === undefined) {
                    throw new Error('ChatGPT API token configuration is missing from server');
                }
                this.config.apis.chatgptApiToken = serverConfig.apis.chatgptApiToken;
            }
            
            // Unbidden Ink
            if (serverConfig.unbiddenInk) {
                if (serverConfig.unbiddenInk.enabled === undefined) {
                    throw new Error('Unbidden Ink enabled configuration is missing from server');
                }
                if (serverConfig.unbiddenInk.startHour === undefined) {
                    throw new Error('Unbidden Ink startHour configuration is missing from server');
                }
                if (serverConfig.unbiddenInk.endHour === undefined) {
                    throw new Error('Unbidden Ink endHour configuration is missing from server');
                }
                if (serverConfig.unbiddenInk.frequencyMinutes === undefined) {
                    throw new Error('Unbidden Ink frequencyMinutes configuration is missing from server');
                }
                if (serverConfig.unbiddenInk.prompt === undefined) {
                    throw new Error('Unbidden Ink prompt configuration is missing from server');
                }
                this.config.unbiddenInk.enabled = serverConfig.unbiddenInk.enabled;
                this.config.unbiddenInk.startHour = serverConfig.unbiddenInk.startHour;
                this.config.unbiddenInk.endHour = serverConfig.unbiddenInk.endHour;
                this.config.unbiddenInk.frequencyMinutes = serverConfig.unbiddenInk.frequencyMinutes;
                this.config.unbiddenInk.prompt = serverConfig.unbiddenInk.prompt;
            }
            
            // Buttons
            if (serverConfig.buttons) {
                for (let i = 1; i <= 4; i++) {
                    const buttonKey = `button${i}`;
                    if (serverConfig.buttons[buttonKey]) {
                        if (serverConfig.buttons[buttonKey].shortAction === undefined) {
                            throw new Error(`Button ${i} shortAction configuration is missing from server`);
                        }
                        if (serverConfig.buttons[buttonKey].longAction === undefined) {
                            throw new Error(`Button ${i} longAction configuration is missing from server`);
                        }
                        if (serverConfig.buttons[buttonKey].shortMqttTopic === undefined) {
                            throw new Error(`Button ${i} shortMqttTopic configuration is missing from server`);
                        }
                        if (serverConfig.buttons[buttonKey].longMqttTopic === undefined) {
                            throw new Error(`Button ${i} longMqttTopic configuration is missing from server`);
                        }
                        this.config.buttons[buttonKey] = {
                            shortAction: serverConfig.buttons[buttonKey].shortAction,
                            longAction: serverConfig.buttons[buttonKey].longAction,
                            shortMqttTopic: serverConfig.buttons[buttonKey].shortMqttTopic,
                            longMqttTopic: serverConfig.buttons[buttonKey].longMqttTopic
                        };
                    }
                }
            }
            
            // LEDs
            if (serverConfig.leds) {
                if (serverConfig.leds.pin === undefined) {
                    throw new Error('LED pin configuration is missing from server');
                }
                if (serverConfig.leds.count === undefined) {
                    throw new Error('LED count configuration is missing from server');
                }
                if (serverConfig.leds.brightness === undefined) {
                    throw new Error('LED brightness configuration is missing from server');
                }
                if (serverConfig.leds.refreshRate === undefined) {
                    throw new Error('LED refreshRate configuration is missing from server');
                }
                this.config.leds.pin = serverConfig.leds.pin;
                this.config.leds.count = serverConfig.leds.count;
                this.config.leds.brightness = serverConfig.leds.brightness;
                this.config.leds.refreshRate = serverConfig.leds.refreshRate;
            }
        },
        
        // Message display (delegates to existing system)
        showMessage(message, type) {
            if (window.SettingsUI && window.SettingsUI.showMessage) {
                window.SettingsUI.showMessage(message, type);
            }
        },
        
        // Form validation
        validateField(fieldName, value) {
            this.validation.touched[fieldName] = true;
            
            // Clear previous errors
            delete this.validation.errors[fieldName];
            
            // Required field validation
            const requiredFields = ['device.owner', 'wifi.ssid'];
            if (this.config.unbiddenInk.enabled) {
                requiredFields.push('apis.chatgptApiToken');
            }
            
            if (requiredFields.includes(fieldName) && (!value || value.trim() === '')) {
                this.validation.errors[fieldName] = 'This field is required';
                this.validation.isValid = false;
                return false;
            }
            
            // Field-specific validation
            switch (fieldName) {
                case 'wifi.connect_timeout':
                    const timeout = parseInt(value);
                    if (isNaN(timeout) || timeout < 5 || timeout > 60) {
                        this.validation.errors[fieldName] = 'Timeout must be between 5 and 60 seconds';
                        this.validation.isValid = false;
                        return false;
                    }
                    break;
                    
                case 'mqtt.port':
                    const port = parseInt(value);
                    if (isNaN(port) || port < 1 || port > 65535) {
                        this.validation.errors[fieldName] = 'Port must be between 1 and 65535';
                        this.validation.isValid = false;
                        return false;
                    }
                    break;
                    
                case 'validation.maxCharacters':
                    const maxChars = parseInt(value);
                    if (isNaN(maxChars) || maxChars < 100 || maxChars > 5000) {
                        this.validation.errors[fieldName] = 'Max characters must be between 100 and 5000';
                        this.validation.isValid = false;
                        return false;
                    }
                    break;
            }
            
            // Check overall form validity
            this.validation.isValid = Object.keys(this.validation.errors).length === 0;
            return true;
        },
        
        // Validate all fields
        validateForm() {
            this.validation.errors = {};
            this.validation.isValid = true;
            
            // Validate all required and visible fields
            const fieldsToValidate = [
                'device.owner',
                'wifi.ssid', 
                'wifi.connect_timeout',
                'mqtt.port',
                'validation.maxCharacters'
            ];
            
            if (this.config.unbiddenInk.enabled) {
                fieldsToValidate.push('apis.chatgptApiToken');
            }
            
            fieldsToValidate.forEach(fieldName => {
                const value = this.getNestedValue(fieldName);
                this.validateField(fieldName, value);
            });
            
            return this.validation.isValid;
        },
        
        // Helper to get nested object values
        getNestedValue(fieldName) {
            return fieldName.split('.').reduce((obj, key) => obj && obj[key], this.config);
        },
        
        // LED effect functions
        async testLedEffect(effectName) {
            try {
                const response = await fetch('/api/led-effect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        effect: effectName,
                        duration: 10, // 10 seconds for testing
                        color: 'blue', // Default test color
                    })
                });
                
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}`);
                }
                
                const result = await response.json();
                if (result.success) {
                    window.showMessage(`Testing ${effectName} effect for 10 seconds`, 'info');
                } else {
                    throw new Error(result.message);
                }
            } catch (error) {
                console.error('LED effect test failed:', error);
                window.showMessage(`Failed to test LED effect: ${error.message}`, 'error');
            }
        },
        
        async turnOffLeds() {
            try {
                const response = await fetch('/api/led-effect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        effect: 'turn_off',
                        duration: 0
                    })
                });
                
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}`);
                }
                
                const result = await response.json();
                if (result.success) {
                    window.showMessage('LEDs turned off', 'success');
                } else {
                    throw new Error(result.message);
                }
            } catch (error) {
                console.error('Turn off LEDs failed:', error);
                window.showMessage(`Failed to turn off LEDs: ${error.message}`, 'error');
            }
        },
    };
}

// Export store initializer for use in HTML
window.initializeSettingsStore = initializeSettingsStore;