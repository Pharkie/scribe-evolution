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
            }
        },
        
        // Form validation states
        validation: {
            errors: {},
            touched: {}
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
        async loadConfiguration() {
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
            this.saving = true;
            try {
                // Use existing SettingsAPI with reactive config
                const message = await window.SettingsAPI.saveConfiguration(this.config);
                
                this.showMessage(message, 'success');
                
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
                this.config.device.owner = serverConfig.device.owner || '';
                this.config.device.timezone = serverConfig.device.timezone || '';
            }
            
            // WiFi  
            if (serverConfig.wifi) {
                this.config.wifi.ssid = serverConfig.wifi.ssid || '';
                this.config.wifi.password = serverConfig.wifi.password || '';
                this.config.wifi.connect_timeout = serverConfig.wifi.connect_timeout || 15000;
            }
            
            // MQTT
            if (serverConfig.mqtt) {
                this.config.mqtt.server = serverConfig.mqtt.server || '';
                this.config.mqtt.port = serverConfig.mqtt.port || 1883;
                this.config.mqtt.username = serverConfig.mqtt.username || '';
                this.config.mqtt.password = serverConfig.mqtt.password || '';
            }
            
            // Validation
            if (serverConfig.validation) {
                this.config.validation.maxCharacters = serverConfig.validation.maxCharacters || 1000;
            }
            
            // APIs
            if (serverConfig.apis) {
                this.config.apis.chatgptApiToken = serverConfig.apis.chatgptApiToken || '';
            }
            
            // Unbidden Ink
            if (serverConfig.unbiddenInk) {
                this.config.unbiddenInk.enabled = serverConfig.unbiddenInk.enabled || false;
                this.config.unbiddenInk.startHour = serverConfig.unbiddenInk.startHour || 8;
                this.config.unbiddenInk.endHour = serverConfig.unbiddenInk.endHour || 22;
                this.config.unbiddenInk.frequencyMinutes = serverConfig.unbiddenInk.frequencyMinutes || 120;
                this.config.unbiddenInk.prompt = serverConfig.unbiddenInk.prompt || '';
            }
            
            // Buttons
            if (serverConfig.buttons) {
                for (let i = 1; i <= 4; i++) {
                    const buttonKey = `button${i}`;
                    if (serverConfig.buttons[buttonKey]) {
                        this.config.buttons[buttonKey] = {
                            shortAction: serverConfig.buttons[buttonKey].shortAction || '',
                            longAction: serverConfig.buttons[buttonKey].longAction || '',
                            shortMqttTopic: serverConfig.buttons[buttonKey].shortMqttTopic || '',
                            longMqttTopic: serverConfig.buttons[buttonKey].longMqttTopic || ''
                        };
                    }
                }
            }
        },
        
        // Message display (delegates to existing system)
        showMessage(message, type) {
            if (window.SettingsUI && window.SettingsUI.showMessage) {
                window.SettingsUI.showMessage(message, type);
            }
        }
    };
}

// Export store initializer for use in HTML
window.initializeSettingsStore = initializeSettingsStore;