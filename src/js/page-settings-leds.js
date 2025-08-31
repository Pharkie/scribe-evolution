// LED Settings Alpine.js Store
// Follows established patterns from page-settings-buttons.js

window.addEventListener('alpine:init', () => {
    Alpine.store('settingsLeds', {
        // State
        loading: true,
        error: null,
        saving: false,
        initialized: false,
        config: {},
        originalConfig: {},
        testingEffect: false,

        // Color presets for swatch selection
        colorPresets: [
            '#0062ff', // Blue
            '#ff0000', // Red  
            '#00ff00', // Green
            '#ff00ff', // Magenta
            '#ffff00', // Yellow
            '#00ffff'  // Cyan
        ],

        // Reactive computed properties
        get canSave() {
            return !this.saving && !this.loading && !this.error && this.hasChanges;
        },

        get hasChanges() {
            // Only check system settings that are actually saved (not playground effects)
            const currentSystemSettings = {
                count: this.config.leds?.count,
                brightness: this.config.leds?.brightness,
                refreshRate: this.config.leds?.refreshRate
            };
            const originalSystemSettings = {
                count: this.originalConfig.leds?.count,
                brightness: this.originalConfig.leds?.brightness,
                refreshRate: this.originalConfig.leds?.refreshRate
            };
            return JSON.stringify(currentSystemSettings) !== JSON.stringify(originalSystemSettings);
        },

        // Initialization
        async init() {
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('💡 LED Settings: Already initialized, skipping');
                return;
            }
            this.initialized = true;

            try {
                await this.loadConfiguration();
            } catch (error) {
                console.error('Failed to initialize LED settings:', error);
                this.error = 'Failed to initialize LED settings. Please refresh the page.';
            }
        },

        // LED CONFIGURATION API
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

                // Ensure LED structure exists with defaults
                if (!this.config.leds) {
                    this.config.leds = {};
                }

                // Initialize LED configurations if missing
                const defaults = {
                    gpio: -1,
                    count: 30,
                    effect: 'chase_single',
                    brightness: 128,
                    refreshRate: 60,
                    speed: 50,
                    intensity: 50,
                    cycles: 3,
                    colors: ['#0062ff', '#ff0000', '#00ff00']
                };

                Object.keys(defaults).forEach(key => {
                    if (this.config.leds[key] === undefined) {
                        this.config.leds[key] = defaults[key];
                    }
                });

                // Ensure colors array exists and has at least 3 colors
                if (!Array.isArray(this.config.leds.colors) || this.config.leds.colors.length < 3) {
                    this.config.leds.colors = ['#0062ff', '#ff0000', '#00ff00'];
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
                // Create partial config with ONLY LED system settings (exclude effect playground params)
                const partialConfig = {
                    leds: {
                        count: this.config.leds.count,
                        brightness: this.config.leds.brightness,
                        refreshRate: this.config.leds.refreshRate
                    }
                };

                console.log('Saving partial LED configuration:', partialConfig);
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

                // Success - update original config for system settings only (not playground effects)
                this.originalConfig.leds = this.originalConfig.leds || {};
                this.originalConfig.leds.count = this.config.leds.count;
                this.originalConfig.leds.brightness = this.config.leds.brightness;
                this.originalConfig.leds.refreshRate = this.config.leds.refreshRate;
                
                // Redirect to settings overview with success parameter (no timeout)
                window.location.href = '/settings.html?saved=leds';

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

        // COLOR CONTROL FUNCTIONS
        showColorControls() {
            return this.config.leds && 
                   (this.config.leds.effect === 'chase_single' || 
                    this.config.leds.effect === 'chase_multi' ||
                    this.config.leds.effect === 'matrix' ||
                    this.config.leds.effect === 'twinkle' ||
                    this.config.leds.effect === 'pulse');
        },

        updateEffectParams() {
            // Reset colors based on C++ config requirements
            if (this.config.leds.effect === 'chase_single') {
                this.config.leds.colors = ['#0062ff']; // Blue (DEFAULT_CHASE_SINGLE_COLOR)
            } else if (this.config.leds.effect === 'chase_multi') {
                this.config.leds.colors = ['#ff0000', '#00ff00', '#0000ff']; // Red, Green, Blue
            } else if (this.config.leds.effect === 'matrix') {
                this.config.leds.colors = ['#00ff00']; // Green (DEFAULT_MATRIX_COLOR)
            } else if (this.config.leds.effect === 'twinkle') {
                this.config.leds.colors = ['#ffff00']; // Yellow (DEFAULT_TWINKLE_COLOR)
            } else if (this.config.leds.effect === 'pulse') {
                this.config.leds.colors = ['#800080']; // Purple (DEFAULT_PULSE_COLOR)
            } else if (this.config.leds.effect === 'rainbow') {
                // Rainbow effect uses no colors - procedurally generated
                this.config.leds.colors = [];
            }
        },

        // LED EFFECT TESTING
        async testLedEffect() {
            if (this.testingEffect) return;
            
            this.testingEffect = true;
            try {
                // Create effect parameters object (exclude system settings like brightness/refreshRate)
                let colors = this.config.leds.colors || ['#0062ff'];
                
                // Only send relevant colors based on C++ config requirements
                if (this.config.leds.effect === 'chase_single' || 
                    this.config.leds.effect === 'matrix' ||
                    this.config.leds.effect === 'twinkle' ||
                    this.config.leds.effect === 'pulse') {
                    // Single color effects
                    colors = [colors[0]];
                } else if (this.config.leds.effect === 'chase_multi') {
                    // Multi color effects need exactly 3 colors
                    colors = colors.slice(0, 3);
                } else if (this.config.leds.effect === 'rainbow') {
                    // Rainbow effect uses no colors
                    colors = [];
                }
                
                const effectParams = {
                    effect: this.config.leds.effect,
                    speed: this.config.leds.speed || 50,
                    intensity: this.config.leds.intensity || 50,
                    cycles: this.config.leds.cycles || 3,
                    colors: colors
                };

                console.log('Testing LED effect:', effectParams);
                
                // Use fetch to test LED effect with JSON body
                const response = await fetch('/api/leds/test', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify(effectParams)
                });

                if (!response.ok) {
                    throw new Error(`LED test failed: ${response.status}`);
                }

            } catch (error) {
                console.error('Failed to test LED effect:', error);
                this.showErrorMessage('Failed to test LED effect: ' + error.message);
            } finally {
                this.testingEffect = false;
            }
        },

        async turnOffLeds() {
            try {
                console.log('Turning off LEDs');
                
                // Use fetch to turn off LEDs
                const response = await fetch('/api/leds/off', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    }
                });

                if (!response.ok) {
                    throw new Error(`LED turn off failed: ${response.status}`);
                }

            } catch (error) {
                console.error('Failed to turn off LEDs:', error);
                this.showErrorMessage('Failed to turn off LEDs: ' + error.message);
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

// Settings LEDs store registered above with Alpine.store()