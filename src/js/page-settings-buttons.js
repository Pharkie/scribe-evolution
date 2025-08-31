// Button Settings Alpine.js Store
// Follows established patterns from page-settings-device.js

window.addEventListener('alpine:init', () => {
    Alpine.store('settingsButtons', {
        // State
        loading: true,
        error: null,
        saving: false,
        config: {},
        originalConfig: {},
        validationErrors: {},

        // Reactive computed properties
        get canSave() {
            return !this.saving && !this.loading && !this.error && this.hasChanges && !this.hasValidationErrors;
        },

        get hasChanges() {
            return JSON.stringify(this.config) !== JSON.stringify(this.originalConfig);
        },

        get hasValidationErrors() {
            return this.validationErrors && Object.keys(this.validationErrors).length > 0;
        },

        // Initialization
        async init() {
            try {
                await this.loadConfiguration();
            } catch (error) {
                console.error('Failed to initialize button settings:', error);
                this.error = 'Failed to initialize button settings. Please refresh the page.';
            }
        },

        // BUTTON CONFIGURATION API
        async loadConfiguration() {
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
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(this.config)
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

                // Success - update original config
                this.originalConfig = JSON.parse(JSON.stringify(this.config));
                
                // Show success feedback and redirect
                this.showSuccessMessage('Button settings saved successfully!');
                
                // Redirect to settings overview with success parameter
                setTimeout(() => {
                    window.location.href = '/settings.html?saved=buttons';
                }, 1000);

            } catch (error) {
                console.error('Error saving configuration:', error);
                this.error = error.message || 'Failed to save configuration';
                this.showErrorMessage(this.error);
            } finally {
                this.saving = false;
            }
        },

        cancelConfiguration() {
            if (this.hasChanges) {
                if (confirm('You have unsaved changes. Are you sure you want to cancel?')) {
                    this.config = JSON.parse(JSON.stringify(this.originalConfig));
                    window.location.href = '/settings.html';
                }
            } else {
                window.location.href = '/settings.html';
            }
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
        showSuccessMessage(message) {
            // Create and show success notification
            const notification = document.createElement('div');
            notification.className = 'fixed top-4 right-4 bg-green-600 text-white px-6 py-3 rounded-lg shadow-lg z-50 transition-all duration-300';
            notification.textContent = message;
            document.body.appendChild(notification);

            // Auto-remove after 3 seconds
            setTimeout(() => {
                notification.style.opacity = '0';
                setTimeout(() => {
                    if (notification.parentNode) {
                        notification.parentNode.removeChild(notification);
                    }
                }, 300);
            }, 3000);
        },

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

// Initialize settings buttons store instance
window.settingsButtonsStoreInstance = Alpine.store('settingsButtons');