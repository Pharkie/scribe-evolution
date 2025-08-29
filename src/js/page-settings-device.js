/**
 * @file page-settings-device.js
 * @brief Alpine.js store for device settings page
 * @description Focused Alpine store for device-specific configuration
 * Copies organized API functions from main settings store
 */

/**
 * Initialize Device Settings Alpine Store
 * Contains only device-related functionality and API calls
 */
function initializeDeviceSettingsStore() {
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
        
        // Original configuration for change detection
        originalConfig: null,
        
        // GPIO information from backend
        gpio: {
            availablePins: [],
            safePins: [],
            pinDescriptions: {}
        },
        
        // Configuration data (reactive) - Device section with hardware GPIO
        config: {
            device: {
                owner: null,
                timezone: null,
                printerTxPin: null
            },
            buttons: {
                button1: { gpio: null },
                button2: { gpio: null },
                button3: { gpio: null },
                button4: { gpio: null }
            },
            leds: {
                enabled: false,
                pin: null
            }
        },
        
        // Validation state
        validation: {
            errors: {}
        },

        // ================== DEVICE CONFIGURATION API ==================
        // Initialize store with data from server
        async init() {
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('⚙️ Device Settings: Already initialized, skipping');
                return;
            }
            this.initialized = true;
            
            this.loading = true;
            try {
                // Load configuration from API
                const serverConfig = await window.SettingsAPI.loadConfiguration();
                
                // Store original config for change detection
                this.originalConfig = JSON.parse(JSON.stringify(serverConfig));
                
                // Extract only device-related configuration
                this.mergeDeviceConfig(serverConfig);
                
                console.log('Alpine Device Store: Configuration loaded successfully');
                
            } catch (error) {
                console.error('Alpine Device Store: Failed to load configuration:', error);
                this.error = error.message;
            } finally {
                this.loading = false;
            }
        },

        // Merge server config into reactive state (device section only)
        mergeDeviceConfig(serverConfig) {
            console.log('🔧 Merging device config from server:', serverConfig);
            
            // Device configuration
            if (serverConfig.device) {
                this.config.device.owner = serverConfig.device.owner || '';
                this.config.device.timezone = serverConfig.device.timezone || '';
                this.config.device.printerTxPin = serverConfig.device.printerTxPin;
                
                if (!serverConfig.device.owner) {
                    console.warn('⚠️ Missing device.owner in config');
                }
                if (!serverConfig.device.timezone) {
                    console.warn('⚠️ Missing device.timezone in config');
                }
            } else {
                console.error('❌ Missing device section in config');
            }
            
            // Buttons GPIO configuration
            if (serverConfig.buttons) {
                for (let i = 1; i <= 4; i++) {
                    const buttonKey = `button${i}`;
                    if (serverConfig.buttons[buttonKey]) {
                        this.config.buttons[buttonKey].gpio = serverConfig.buttons[buttonKey].gpio || null;
                    }
                }
            } else {
                console.warn('⚠️ Missing buttons section in config');
            }
            
            // LEDs GPIO configuration
            if (serverConfig.leds) {
                this.config.leds.enabled = serverConfig.leds.enabled || false;
                this.config.leds.pin = Number(serverConfig.leds.pin);
            } else {
                console.warn('⚠️ Missing leds section in config');
            }
            
            // GPIO information
            if (serverConfig.gpio) {
                this.gpio.availablePins = serverConfig.gpio.availablePins || [];
                this.gpio.safePins = serverConfig.gpio.safePins || [];
                this.gpio.pinDescriptions = serverConfig.gpio.pinDescriptions || {};
            } else {
                console.warn('⚠️ Missing gpio section in config');
            }
            
            console.log('✅ Device config merge complete:', this.config);
        },
        
        // Validate device owner field specifically (called from UI)
        validateDeviceOwner(value) {
            if (!value || value.trim() === '') {
                this.validation.errors['device.owner'] = 'Device owner cannot be blank';
            } else {
                // Clear the error if it was previously set
                if (this.validation.errors['device.owner']) {
                    delete this.validation.errors['device.owner'];
                }
            }
        },
        
        // Validate timezone field specifically (called from UI)
        validateTimezone(value) {
            if (!value || value.trim() === '') {
                this.validation.errors['device.timezone'] = 'Timezone cannot be blank';
            } else {
                // Clear the error if it was previously set
                if (this.validation.errors['device.timezone']) {
                    delete this.validation.errors['device.timezone'];
                }
            }
        },

        // Check if configuration has meaningful changes
        hasChanges() {
            if (!this.originalConfig) {
                return false; // Can't determine changes without original config
            }
            
            const original = this.originalConfig;
            
            // Check device owner changes
            if (this.config.device.owner !== (original.device?.owner || '')) {
                return true;
            }
            
            // Check device timezone changes
            if (this.config.device.timezone !== (original.device?.timezone || '')) {
                return true;
            }
            
            // Check printer TX pin changes
            if (this.config.device.printerTxPin !== original.device?.printerTxPin) {
                return true;
            }
            
            // Check button GPIO changes
            for (const buttonKey of ['button1', 'button2', 'button3', 'button4']) {
                const currentGpio = this.config.buttons[buttonKey].gpio;
                const originalGpio = original.buttons?.[buttonKey]?.gpio || null;
                if (currentGpio !== originalGpio) {
                    return true;
                }
            }
            
            // Check LED configuration changes (if LEDs are enabled)
            if (this.config.leds.enabled) {
                if (this.config.leds.pin !== (original.leds?.pin || null)) {
                    return true;
                }
            }
            
            return false;
        },

        // Computed property to check if save should be enabled
        get canSave() {
            // Don't allow save while loading, saving, or with errors
            if (this.loading || this.saving || this.error) {
                return false;
            }
            
            // Required fields must not be blank
            if (!this.config.device.owner || this.config.device.owner.trim() === '') {
                return false;
            }
            
            if (!this.config.device.timezone || this.config.device.timezone.trim() === '') {
                return false;
            }
            
            // Must have changes to save
            return this.hasChanges();
        },

        // Save device configuration via API
        async saveConfiguration() {
            this.saving = true;
            try {
                // Create partial device config for server submission
                const partialConfig = {
                    device: {
                        owner: this.config.device.owner,
                        timezone: this.config.device.timezone,
                        printerTxPin: this.config.device.printerTxPin
                    },
                    buttons: {
                        button1: this.config.buttons.button1,
                        button2: this.config.buttons.button2,
                        button3: this.config.buttons.button3,
                        button4: this.config.buttons.button4
                    }
                };
                
                // Include LED configuration if LEDs are compiled in (card is visible)
                if (this.config.leds.enabled) {
                    partialConfig.leds = {
                        pin: this.config.leds.pin
                    };
                }
                
                console.log('Saving partial device configuration:', partialConfig);
                const message = await window.SettingsAPI.saveConfiguration(partialConfig);
                
                console.log('Alpine Device Store: Configuration saved successfully');
                
                // Redirect immediately with success parameter
                window.location.href = '/settings.html?saved=device';
                
            } catch (error) {
                console.error('Alpine Device Store: Failed to save configuration:', error);
                this.showErrorMessage('Failed to save device settings: ' + error.message);
                this.saving = false; // Only reset on error
            }
        },

        // Cancel configuration changes
        cancelConfiguration() {
            // Navigate back to settings
            window.location.href = '/settings.html';
        },

        // ================== GPIO MANAGEMENT ==================
        // Get what each GPIO pin is assigned to (reactive getter)
        getGpioAssignment(pinNumber) {
            if (pinNumber === -1 || pinNumber === null) return null;
            
            const pin = Number(pinNumber);
            
            // Check printer TX pin (reactive dependency)
            if (this.config.device.printerTxPin === pin) {
                return 'Assigned to printer';
            }
            
            // Check LED strip pin (reactive dependency) 
            if (this.config.leds?.pin === pin) {
                return 'Assigned to LED strip';
            }
            
            // Check button pins (reactive dependency)
            for (let i = 1; i <= 4; i++) {
                if (this.config.buttons[`button${i}`]?.gpio === pin) {
                    return `Assigned to button ${i}`;
                }
            }
            
            return null;
        },

        // Get formatted text for GPIO option (reactive)
        getGpioOptionText(option) {
            if (option.pin === -1) return 'Not connected';
            
            let text = `GPIO ${option.pin} - ${option.description}`;
            if (!option.isSafe) text += ' (Unsafe)';
            
            // Direct config access for Alpine reactivity
            if (this.config.device.printerTxPin === option.pin) text += ' (Assigned to printer)';
            else if (this.config.leds?.pin === option.pin) text += ' (Assigned to LED strip)';
            else if (this.config.buttons?.button1?.gpio === option.pin) text += ' (Assigned to button 1)';
            else if (this.config.buttons?.button2?.gpio === option.pin) text += ' (Assigned to button 2)';
            else if (this.config.buttons?.button3?.gpio === option.pin) text += ' (Assigned to button 3)';
            else if (this.config.buttons?.button4?.gpio === option.pin) text += ' (Assigned to button 4)';
            
            return text;
        },

        // Get used GPIO pins to avoid conflicts
        get usedGpioPins() {
            const used = new Set();
            
            // Add printer TX pin (exclude -1 "Not connected")
            if (this.config.device.printerTxPin !== null && this.config.device.printerTxPin !== -1) {
                used.add(Number(this.config.device.printerTxPin));
            }
            
            // Add LED strip pin (exclude -1 "Not connected")
            if (this.config.leds?.pin !== null && this.config.leds?.pin !== -1) {
                used.add(Number(this.config.leds.pin));
            }
            
            // Add button GPIO pins (exclude -1 "Not connected")
            for (let i = 1; i <= 4; i++) {
                const buttonGpio = this.config.buttons[`button${i}`]?.gpio;
                if (buttonGpio !== null && buttonGpio !== -1) {
                    used.add(Number(buttonGpio));
                }
            }
            
            return used;
        },

        // GPIO options specifically for printer TX (excludes "Not connected" option)
        get printerGpioOptions() {
            if (this.loading || this.gpio.availablePins.length === 0) {
                return [{ 
                    pin: null, 
                    description: 'Loading GPIO options...', 
                    available: false,
                    isSafe: false,
                    inUse: false
                }];
            }

            return this.gpio.availablePins
                .filter(pin => Number(pin) !== -1) // Exclude "Not connected" option
                .map(pin => {
                    const pinNumber = Number(pin);
                    const isSafe = this.gpio.safePins.includes(pin);
                    const description = this.gpio.pinDescriptions[pin] || 'Unknown';
                    // Force reactive dependency on config changes
                    const isUsed = this.usedGpioPins.has(pinNumber);
                    
                    // Calculate assignment by directly checking config (force reactivity)
                    let assignment = null;
                    // Direct config access for reactivity
                    if (this.config.device.printerTxPin === pinNumber) {
                        assignment = 'Assigned to printer';
                    } else if (this.config.leds?.pin === pinNumber) {
                        assignment = 'Assigned to LED strip';
                    } else if (this.config.buttons?.button1?.gpio === pinNumber) {
                        assignment = 'Assigned to button 1';
                    } else if (this.config.buttons?.button2?.gpio === pinNumber) {
                        assignment = 'Assigned to button 2';
                    } else if (this.config.buttons?.button3?.gpio === pinNumber) {
                        assignment = 'Assigned to button 3';
                    } else if (this.config.buttons?.button4?.gpio === pinNumber) {
                        assignment = 'Assigned to button 4';
                    }
                    
                    return {
                        pin: pinNumber,
                        description: description,
                        available: isSafe && !isUsed,
                        isSafe: isSafe,
                        inUse: isUsed,
                        assignment: assignment
                    };
                });
        },

        // Force reactive rebuild of GPIO options array with text updates
        get allGpioOptionsReactive() {
            // Force re-evaluation by accessing ALL config properties that affect text
            const triggerUpdate = this.config.device.printerTxPin + '-' + 
                this.config.leds?.pin + '-' + 
                this.config.buttons?.button1?.gpio + '-' + 
                this.config.buttons?.button2?.gpio + '-' + 
                this.config.buttons?.button3?.gpio + '-' + 
                this.config.buttons?.button4?.gpio;
            
            // Return completely new array with updated text for each option
            return this.gpio.availablePins.map((pin, index) => {
                const pinNumber = Number(pin);
                const isSafe = this.gpio.safePins.includes(pin);
                const description = this.gpio.pinDescriptions[pin] || 'Unknown';
                const isUsed = this.usedGpioPins.has(pinNumber);
                
                // Build text with current assignments
                let text;
                if (pinNumber === -1) {
                    text = 'Not connected';
                } else {
                    text = `GPIO ${pinNumber} - ${description}`;
                    
                    // Add assignment labels
                    if (pinNumber === this.config.device.printerTxPin) {
                        text += ' (Printer)';
                    } else if (pinNumber === this.config.leds?.pin) {
                        text += ' (LED)';
                    } else if (pinNumber === this.config.buttons?.button1?.gpio) {
                        text += ' (Button1)';
                    } else if (pinNumber === this.config.buttons?.button2?.gpio) {
                        text += ' (Button2)';
                    } else if (pinNumber === this.config.buttons?.button3?.gpio) {
                        text += ' (Button3)';
                    } else if (pinNumber === this.config.buttons?.button4?.gpio) {
                        text += ' (Button4)';
                    }
                }
                
                return {
                    pin: pinNumber,
                    description: description,
                    text: text,
                    available: pinNumber === -1 ? true : (isSafe && !isUsed),
                    isSafe: isSafe,
                    inUse: isUsed,
                    // Add unique key to force Alpine re-render
                    key: `${pinNumber}-${triggerUpdate}-${index}`
                };
            });
        },

        // Combined GPIO options that handles loading state properly for Alpine reactivity
        get allGpioOptions() {
            if (this.loading || this.gpio.availablePins.length === 0) {
                return [{ 
                    pin: null, 
                    description: 'Loading GPIO options...', 
                    available: false,
                    isSafe: false,
                    inUse: false
                }];
            }

            return this.gpio.availablePins.map(pin => {
                const pinNumber = Number(pin);
                const isSafe = this.gpio.safePins.includes(pin);
                const description = this.gpio.pinDescriptions[pin] || 'Unknown';
                
                // Force reactive dependency by accessing ALL config properties
                const isUsed = this.usedGpioPins.has(pinNumber);
                
                // Calculate assignment by directly checking config (force reactivity)
                let assignment = null;
                if (pinNumber !== -1 && pinNumber !== null) {
                    // Direct config access for reactivity
                    if (this.config.device.printerTxPin === pinNumber) {
                        assignment = 'Assigned to printer';
                    } else if (this.config.leds?.pin === pinNumber) {
                        assignment = 'Assigned to LED strip';
                    } else if (this.config.buttons?.button1?.gpio === pinNumber) {
                        assignment = 'Assigned to button 1';
                    } else if (this.config.buttons?.button2?.gpio === pinNumber) {
                        assignment = 'Assigned to button 2';
                    } else if (this.config.buttons?.button3?.gpio === pinNumber) {
                        assignment = 'Assigned to button 3';
                    } else if (this.config.buttons?.button4?.gpio === pinNumber) {
                        assignment = 'Assigned to button 4';
                    }
                }
                
                return {
                    pin: pinNumber,
                    description: description,
                    // "Not connected" (-1) is always available, others check safety and usage
                    available: pinNumber === -1 ? true : (isSafe && !isUsed),
                    isSafe: isSafe,
                    inUse: isUsed,
                    assignment: assignment
                };
            });
        }
    };
    
    return store;
}

// Auto-register the device store when this script loads
document.addEventListener('alpine:init', () => {
    // Prevent multiple initializations if alpine:init fires multiple times
    if (window.deviceStoreInstance) {
        console.log('⚙️ Device Settings: Store already exists, skipping alpine:init');
        return;
    }
    
    // Create and register device settings store
    const deviceStore = initializeDeviceSettingsStore();
    Alpine.store('settingsDevice', deviceStore);
    
    // Make store available globally for body x-data
    window.deviceStoreInstance = deviceStore;
    
    // Initialize the store immediately during alpine:init
    deviceStore.init();
    
    console.log('✅ Device Settings Store registered and initialized');
});