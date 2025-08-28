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
                pin: null
            }
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

        // Save device configuration via API
        async saveConfiguration() {
            this.saving = true;
            try {
                // Create clean device config for server submission
                const cleanConfig = {
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
                    },
                    leds: {
                        pin: this.config.leds.pin
                    }
                };
                
                console.log('Saving device configuration:', cleanConfig);
                const message = await window.SettingsAPI.saveConfiguration(cleanConfig);
                
                console.log('Alpine Device Store: Configuration saved successfully');
                
                // Redirect to main settings page with success indicator
                window.location.href = '/settings.html?saved=device';
                
            } catch (error) {
                console.error('Alpine Device Store: Failed to save configuration:', error);
                this.showErrorMessage('Failed to save device settings: ' + error.message);
                this.saving = false; // Only reset on error
            }
        },

        // Cancel configuration changes
        cancelConfiguration() {
            // Navigate back to home
            window.location.href = '/';
        },

        // ================== GPIO MANAGEMENT ==================
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
                    const isUsed = this.usedGpioPins.has(pinNumber);
                    
                    return {
                        pin: pinNumber,
                        description: description,
                        available: isSafe && !isUsed,
                        isSafe: isSafe,
                        inUse: isUsed
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
                const isUsed = this.usedGpioPins.has(pinNumber);
                
                return {
                    pin: pinNumber,
                    description: description,
                    // "Not connected" (-1) is always available, others check safety and usage
                    available: pinNumber === -1 ? true : (isSafe && !isUsed),
                    isSafe: isSafe,
                    inUse: isUsed
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