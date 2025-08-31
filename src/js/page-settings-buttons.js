// Button Settings Alpine.js Store
// Follows established patterns from page-settings-device.js

window.addEventListener('alpine:init', () => {
    Alpine.store('settingsButtons', {
        // State
        loading: true,  // Start as loading for fade-in effect
        error: null,
        saving: false,
        initialized: false,
        config: {
            buttons: {
                button1: { gpio: null, shortAction: null, longAction: null, shortMqttTopic: null, longMqttTopic: null, shortLedEffect: null, longLedEffect: null },
                button2: { gpio: null, shortAction: null, longAction: null, shortMqttTopic: null, longMqttTopic: null, shortLedEffect: null, longLedEffect: null },
                button3: { gpio: null, shortAction: null, longAction: null, shortMqttTopic: null, longMqttTopic: null, shortLedEffect: null, longLedEffect: null },
                button4: { gpio: null, shortAction: null, longAction: null, shortMqttTopic: null, longMqttTopic: null, shortLedEffect: null, longLedEffect: null }
            }
        },
        originalConfig: {},
        validationErrors: {},

        // Reactive computed properties
        get canSave() {
            return !this.saving && !this.loading && !this.error && this.hasChanges && !this.hasValidationErrors;
        },

        get hasChanges() {
            return JSON.stringify(this.config.buttons) !== JSON.stringify(this.originalConfig.buttons);
        },

        get hasValidationErrors() {
            return this.validationErrors && Object.keys(this.validationErrors).length > 0;
        },

        // BUTTON CONFIGURATION API
        async loadConfiguration() {
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('🔘 Button Settings: Already initialized, skipping');
                return;
            }
            this.initialized = true;

            this.loading = true;
            this.error = null;

            try {
                const response = await fetch('/api/config');
                if (!response.ok) {
                    throw new Error(`Failed to load configuration: ${response.status}`);
                }

                const data = await response.json();
                this.config = data;
                this.originalConfig = JSON.parse(JSON.stringify(data));

                // Ensure button structure exists
                if (!this.config.buttons) {
                    this.config.buttons = {};
                }

                // Initialize button configurations if missing
                for (let i = 1; i <= 4; i++) {
                    if (!this.config.buttons[`button${i}`]) {
                        this.config.buttons[`button${i}`] = {
                            gpio: -1,
                            shortAction: '',
                            longAction: '',
                            shortMqttTopic: '',
                            longMqttTopic: '',
                            shortLedEffect: 'none',
                            longLedEffect: 'none'
                        };
                    }
                }

            } catch (error) {
                console.error('Error loading configuration:', error);
                this.error = error.message || 'Failed to load configuration';
            } finally {
                this.loading = false;
            }
        },

        async saveConfiguration() {
            if (!this.canSave) {
                return;
            }

            this.saving = true;
            this.error = null;

            try {
                // Create partial config with ONLY button-specific fields
                const partialConfig = {
                    buttons: {
                        button1: this.config.buttons.button1,
                        button2: this.config.buttons.button2,
                        button3: this.config.buttons.button3,
                        button4: this.config.buttons.button4
                    }
                };

                console.log('Saving partial button configuration:', partialConfig);
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(partialConfig)
                });

                if (!response.ok) {
                    // Try to parse error response
                    let errorMessage = 'Failed to save configuration';
                    try {
                        const errorData = await response.json();
                        errorMessage = errorData.error || errorMessage;
                    } catch (parseError) {
                        // Ignore parse error, use default message
                    }
                    throw new Error(errorMessage);
                }

                // Success - update original config for buttons section only
                this.originalConfig.buttons = JSON.parse(JSON.stringify(this.config.buttons));
                
                // Redirect immediately to settings overview with success parameter
                window.location.href = '/settings.html?saved=buttons';

            } catch (error) {
                console.error('Error saving configuration:', error);
                this.error = error.message || 'Failed to save configuration';
                this.showErrorMessage(this.error);
                this.saving = false;
            }
        },

        cancelConfiguration() {
            window.location.href = '/settings.html';
        },

        // VALIDATION FUNCTIONS
        isValidMqttTopic(topic) {
            if (!topic) return true; // Empty is valid (means local printing)
            if (topic.length > 65535) return false; // Too long
            if (topic.includes('#') || topic.includes('+')) return false; // No wildcards in publish topics
            if (topic.startsWith('$SYS')) return false; // Reserved prefix
            if (topic.includes('\0')) return false; // No null bytes
            if (topic.trim() !== topic) return false; // No leading/trailing whitespace
            return true;
        },

        validateMqttTopic(topic, buttonNum, pressType) {
            const isValid = this.isValidMqttTopic(topic);
            const errorKey = `mqtt-${buttonNum}-${pressType}`;
            
            if (!isValid && topic) {
                this.validationErrors = this.validationErrors || {};
                this.validationErrors[errorKey] = 'Invalid MQTT topic format';
                return false;
            } else {
                if (this.validationErrors) {
                    delete this.validationErrors[errorKey];
                }
                return true;
            }
        },

        // UTILITY FUNCTIONS
        showErrorMessage(message) {
            // Create and show error notification
            const notification = document.createElement('div');
            notification.className = 'fixed top-4 right-4 bg-red-600 text-white px-6 py-3 rounded-lg shadow-lg z-50 transition-all duration-300';
            notification.textContent = message;
            document.body.appendChild(notification);

            // Auto-remove after 5 seconds
            setTimeout(() => {
                notification.style.opacity = '0';
                setTimeout(() => {
                    if (notification.parentNode) {
                        notification.parentNode.removeChild(notification);
                    }
                }, 300);
            }, 5000);
        }
    });
});

// Settings buttons store registered above with Alpine.store()