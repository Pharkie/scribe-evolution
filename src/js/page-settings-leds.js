// LED Settings Alpine.js Store
// Follows established patterns from page-settings-buttons.js

document.addEventListener('alpine:init', () => {
    Alpine.store('settingsLeds', {
        // State
        loading: true,  // Start as loading for fade-in effect
        error: null,
        saving: false,
        initialized: false,
        config: {
            leds: {
                gpio: null,
                count: null,
                brightness: null,
                refreshRate: null
            }
        },
        
        // Effects playground (temporary, not saved to config)
        effect: null,
        speed: null,
        intensity: null,
        cycles: null,
        colors: [],
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

        // LED CONFIGURATION API
        async loadConfiguration() {
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('💡 LED Settings: Already initialized, skipping');
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
                if (!Array.isArray(this.colors) || this.colors.length < 3) {
                    this.colors = ['#0062ff', '#ff0000', '#00ff00'];
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
                   (this.effect === 'chase_single' || 
                    this.effect === 'chase_multi' ||
                    this.effect === 'matrix' ||
                    this.effect === 'twinkle' ||
                    this.effect === 'pulse');
        },

        updateEffectParams() {
            // Reset colors based on C++ config requirements
            if (this.effect === 'chase_single') {
                this.colors = ['#0062ff']; // Blue (DEFAULT_CHASE_SINGLE_COLOR)
            } else if (this.effect === 'chase_multi') {
                this.colors = ['#ff0000', '#00ff00', '#0000ff']; // Red, Green, Blue
            } else if (this.effect === 'matrix') {
                this.colors = ['#00ff00']; // Green (DEFAULT_MATRIX_COLOR)
            } else if (this.effect === 'twinkle') {
                this.colors = ['#ffff00']; // Yellow (DEFAULT_TWINKLE_COLOR)
            } else if (this.effect === 'pulse') {
                this.colors = ['#800080']; // Purple (DEFAULT_PULSE_COLOR)
            } else if (this.effect === 'rainbow') {
                // Rainbow effect uses no colors - procedurally generated
                this.colors = [];
            }
        },

        // LED EFFECT TESTING
        async testLedEffect() {
            if (this.testingEffect) return;
            
            this.testingEffect = true;
            try {
                // Create effect parameters object (exclude system settings like brightness/refreshRate)
                let colors = this.colors || ['#0062ff'];
                
                // Only send relevant colors based on C++ config requirements
                if (this.effect === 'chase_single' || 
                    this.effect === 'matrix' ||
                    this.effect === 'twinkle' ||
                    this.effect === 'pulse') {
                    // Single color effects
                    colors = [colors[0]];
                } else if (this.effect === 'chase_multi') {
                    // Multi color effects need exactly 3 colors
                    colors = colors.slice(0, 3);
                } else if (this.effect === 'rainbow') {
                    // Rainbow effect uses no colors
                    colors = [];
                }
                
                const effectParams = {
                    effect: this.effect,
                    speed: this.speed || 50,
                    intensity: this.intensity || 50,
                    cycles: this.cycles || 3,
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