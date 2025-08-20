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
        loading: true, // Start as loading to prevent flicker
        saving: false,
        error: null, // Error state for the UI
        initialized: false, // Flag to prevent duplicate initialization
        
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
        activeSection: 'wifi',
        showValidationFeedback: false,
        
        // LED Effect Parameters (WLED-style unified interface)
        selectedEffect: 'simple_chase',
        effectParams: {
            speed: 10,
            intensity: 50,
            palette: '#0062ff',
            custom1: 10,
            custom2: 5,
            custom3: 1
        },
        
        // Section definitions for navigation
        sections: [
            { id: 'wifi', name: 'WiFi', icon: '📶', color: 'blue' },
            { id: 'device', name: 'Device', icon: '⚙️', color: 'purple' },
            { id: 'mqtt', name: 'MQTT', icon: '📡', color: 'yellow' },
            { id: 'unbidden', name: 'Unbidden Ink', icon: '🎲', color: 'green' },
            { id: 'buttons', name: 'Buttons', icon: '🎛️', color: 'orange' },
            { id: 'leds', name: 'LEDs', icon: '🌈', color: 'purple' }
        ],
        
        // Computed properties for complex UI states
        get timeRangeDisplay() {
            const start = this.config?.unbiddenInk?.startHour ?? 0;
            const end = this.config?.unbiddenInk?.endHour ?? 24;
            
            if (start === 0 && (end === 0 || end === 24)) {
                return 'All Day';
            }
            
            return `${this.formatHour(start)} - ${this.formatHour(end)}`;
        },
        
        get frequencyDisplay() {
            const minutes = this.config?.unbiddenInk?.frequencyMinutes ?? 120;
            const hours = Math.floor(minutes / 60);
            const mins = minutes % 60;
            
            const start = this.config?.unbiddenInk?.startHour ?? 0;
            const end = this.config?.unbiddenInk?.endHour ?? 24;
            
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
            const start = this.config?.unbiddenInk?.startHour ?? 0;
            const end = this.config?.unbiddenInk?.endHour ?? 24;
            
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
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('⚙️ Settings: Already initialized, skipping');
                return;
            }
            this.initialized = true;
            
            // Initialize LED effect parameters
            this.initEffectParams();
            
            this.loading = true;
            try {
                // Use existing SettingsAPI
                const serverConfig = await window.SettingsAPI.loadConfiguration();
                
                // Deep merge server config into reactive state
                this.mergeConfig(serverConfig);
                
                console.log('Alpine Store: Configuration loaded successfully');
            } catch (error) {
                console.error('Alpine Store: Failed to load configuration:', error);
                this.error = error.message;
                window.showMessage('Failed to load configuration: ' + error.message, 'error');
            } finally {
                this.loading = false;
            }
        },
        
        // Save configuration to server
        async saveConfiguration() {
            // Validate form before saving
            if (!this.validateForm()) {
                this.showValidationFeedback = true;
                
                // Show specific validation errors
                const errorFields = Object.keys(this.validation.errors);
                if (errorFields.length > 0) {
                    const firstError = this.validation.errors[errorFields[0]];
                    window.showMessage(`Validation error: ${firstError}`, 'error');
                } else {
                    window.showMessage('Please check all required fields before saving', 'error');
                }
                return;
            }
            
            this.saving = true;
            try {
                // Use existing SettingsAPI with reactive config
                const message = await window.SettingsAPI.saveConfiguration(this.config);
                
                window.showMessage(message, 'success');
                this.showValidationFeedback = false;
                
                // Trigger LED confirmation effect if available
                if (window.SettingsLED && window.SettingsLED.triggerEffect) {
                    window.SettingsLED.triggerEffect('simple_chase', 'green', 3000);
                }
                
                console.log('Alpine Store: Configuration saved successfully');
            } catch (error) {
                console.error('Alpine Store: Failed to save configuration:', error);
                window.showMessage('Failed to save configuration: ' + error.message, 'error');
            } finally {
                this.saving = false;
            }
        },
        
        // Cancel configuration changes and return to index
        cancelConfiguration() {
            window.location.href = '/';
        },
        
        // Helper methods
        formatHour(hour) {
            if (hour === 0) return '00:00';
            if (hour === 24) return '24:00';
            return hour.toString().padStart(2, '0') + ':00';
        },
        
        formatHour12(hour) {
            if (hour === 0 || hour === 24) return '12 am';
            if (hour === 12) return '12 pm';
            if (hour < 12) return `${hour} am`;
            return `${hour - 12} pm`;
        },
        
        // Deep merge server config into reactive state with fallbacks
        mergeConfig(serverConfig) {
            // Device - provide fallbacks for missing values
            if (serverConfig.device) {
                this.config.device.owner = serverConfig.device.owner ?? '';
                this.config.device.timezone = serverConfig.device.timezone ?? 'America/New_York';
            }
            
            // WiFi - provide fallbacks for missing values  
            if (serverConfig.wifi) {
                this.config.wifi.ssid = serverConfig.wifi.ssid ?? '';
                this.config.wifi.password = serverConfig.wifi.password ?? '';
                this.config.wifi.connect_timeout = serverConfig.wifi.connect_timeout ?? 15000;
            }
            
            // MQTT - provide fallbacks for missing values
            if (serverConfig.mqtt) {
                this.config.mqtt.server = serverConfig.mqtt.server ?? '';
                this.config.mqtt.port = serverConfig.mqtt.port ?? 1883;
                this.config.mqtt.username = serverConfig.mqtt.username ?? '';
                this.config.mqtt.password = serverConfig.mqtt.password ?? '';
            }
            
            // Validation - provide fallbacks for missing values
            if (serverConfig.validation) {
                this.config.validation.maxCharacters = serverConfig.validation.maxCharacters ?? 1000;
            }
            
            // APIs - provide fallbacks for missing values
            if (serverConfig.apis) {
                this.config.apis.chatgptApiToken = serverConfig.apis.chatgptApiToken ?? '';
            }
            
            // Unbidden Ink - provide fallbacks for missing values
            if (serverConfig.unbiddenInk) {
                this.config.unbiddenInk.enabled = serverConfig.unbiddenInk.enabled ?? false;
                this.config.unbiddenInk.startHour = serverConfig.unbiddenInk.startHour ?? 8;
                this.config.unbiddenInk.endHour = serverConfig.unbiddenInk.endHour ?? 22;
                this.config.unbiddenInk.frequencyMinutes = serverConfig.unbiddenInk.frequencyMinutes ?? 120;
                this.config.unbiddenInk.prompt = serverConfig.unbiddenInk.prompt ?? '';
            }
            
            // Buttons - provide fallbacks for missing values
            if (serverConfig.buttons) {
                for (let i = 1; i <= 4; i++) {
                    const buttonKey = `button${i}`;
                    if (serverConfig.buttons[buttonKey]) {
                        this.config.buttons[buttonKey] = {
                            shortAction: serverConfig.buttons[buttonKey].shortAction ?? '',
                            longAction: serverConfig.buttons[buttonKey].longAction ?? '',
                            shortMqttTopic: serverConfig.buttons[buttonKey].shortMqttTopic ?? '',
                            longMqttTopic: serverConfig.buttons[buttonKey].longMqttTopic ?? ''
                        };
                    }
                }
            }
            
            // LEDs - provide fallbacks for missing values
            if (serverConfig.leds) {
                this.config.leds.pin = serverConfig.leds.pin ?? 4;
                this.config.leds.count = serverConfig.leds.count ?? 60;
                this.config.leds.brightness = serverConfig.leds.brightness ?? 128;
                this.config.leds.refreshRate = serverConfig.leds.refreshRate ?? 60;
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
            
            // Clear previous errors for this field
            if (this.validation.errors[fieldName]) {
                delete this.validation.errors[fieldName];
            }
            
            // Required field validation
            const requiredFields = ['device.owner', 'wifi.ssid'];
            if (this.config.unbiddenInk.enabled) {
                requiredFields.push('apis.chatgptApiToken');
            }
            
            if (requiredFields.includes(fieldName)) {
                if (!value || (typeof value === 'string' && value.trim() === '')) {
                    this.validation.errors[fieldName] = 'This field is required';
                    this.validation.isValid = false;
                    console.log(`Required field validation failed for ${fieldName}:`, value);
                    return false;
                }
            }
            
            // Field-specific validation
            switch (fieldName) {
                case 'wifi.connect_timeout':
                    const timeout = typeof value === 'number' ? Math.floor(value / 1000) : parseInt(value);
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
                console.log(`Validating ${fieldName}:`, value);
                this.validateField(fieldName, value);
            });
            
            console.log('Validation errors:', this.validation.errors);
            console.log('Form is valid:', this.validation.isValid);
            
            return this.validation.isValid;
        },
        
        // Helper to get nested object values
        getNestedValue(fieldName) {
            return fieldName.split('.').reduce((obj, key) => obj && obj[key], this.config);
        },
        
        // LED effect functions (WLED-style unified interface)
        async testLedEffect(effectName) {
            try {
                // Build colors array based on effect - always use colors array
                let colors = [];
                if (effectName === 'chase') {
                    // Multicolour chase uses 3 colors
                    colors = [
                        this.effectParams.palette,
                        this.effectParams.color2,
                        this.effectParams.color3
                    ];
                } else {
                    // All other effects use single color
                    colors = [this.effectParams.palette];
                }

                // Build unified payload - no more single color fields
                let effectParams = {
                    effect: effectName,
                    duration: 10, // 10 seconds for testing
                    speed: this.effectParams.speed,
                    intensity: this.effectParams.intensity,
                    colors: colors
                };
                
                // Add effect-specific custom parameters
                const customParams = this.getEffectCustomParams(effectName);
                Object.assign(effectParams, customParams);
                
                const response = await fetch('/api/led-effect', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(effectParams)
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
        
        // Map generic parameters to effect-specific ones
        getEffectCustomParams(effectName) {
            const params = {};
            
            switch(effectName) {
                case 'simple_chase':
                    // intensity = unused, custom1-3 = unused
                    break;
                case 'rainbow':
                    // intensity = density, custom1 = wave length
                    params.density = this.effectParams.intensity;
                    params.waveLength = this.effectParams.custom1;
                    break;
                case 'twinkle':
                    // intensity = density, custom1 = fade speed
                    params.density = this.effectParams.intensity;
                    params.fadeSpeed = this.effectParams.custom1;
                    break;
                case 'chase':
                    // intensity = trail_length, custom1 = dot size (color spacing)
                    params.trailLength = this.effectParams.intensity;
                    params.colorSpacing = this.effectParams.custom1;
                    break;
                case 'pulse':
                    // intensity = unused
                    break;
                case 'matrix':
                    // intensity = unused, custom2 = drop rate
                    params.drops = this.effectParams.custom2;
                    break;
            }
            
            return params;
        },
        
        // Get custom slider labels and ranges for current effect
        getCustomSlider1Label() {
            switch(this.selectedEffect) {
                case 'rainbow': return 'Wave Length';
                case 'twinkle': return 'Fade Speed';
                case 'chase': return 'Dot Size';
                default: return null;
            }
        },
        
        getCustomSlider1Range() {
            switch(this.selectedEffect) {
                case 'rainbow': return { min: 1, max: 20, step: 1 };
                case 'twinkle': return { min: 1, max: 10, step: 1 };
                case 'chase': return { min: 1, max: 10, step: 1 };
                default: return { min: 1, max: 100, step: 1 };
            }
        },
        
        getCustomSlider2Label() {
            switch(this.selectedEffect) {
                case 'matrix': return 'Drop Rate';
                default: return null;
            }
        },
        
        getCustomSlider2Range() {
            switch(this.selectedEffect) {
                case 'matrix': return { min: 1, max: 20, step: 1 };
                default: return { min: 1, max: 100, step: 1 };
            }
        },
        
        getCustomSlider3Label() {
            // Currently no effects use 3 custom sliders
            return null;
        },
        
        getCustomSlider3Range() {
            return { min: 1, max: 100, step: 1 };
        },
        
        // Initialize effect parameters based on selected effect  
        initEffectParams() {
            const defaults = {
                'simple_chase': { speed: 10, intensity: 50, palette: '#0062ff', color2: '#0062ff', color3: '#0062ff', custom1: 10, custom2: 5, custom3: 1 },
                'rainbow': { speed: 20, intensity: 5, palette: '#ff0000', color2: '#ff0000', color3: '#ff0000', custom1: 5, custom2: 3, custom3: 1 },
                'twinkle': { speed: 5, intensity: 10, palette: '#ffff00', color2: '#ffff00', color3: '#ffff00', custom1: 3, custom2: 2, custom3: 1 },
                'chase': { speed: 15, intensity: 10, palette: '#ff0000', color2: '#00ff00', color3: '#0000ff', custom1: 3, custom2: 5, custom3: 1 },
                'pulse': { speed: 4, intensity: 50, palette: '#800080', color2: '#800080', color3: '#800080', custom1: 5, custom2: 3, custom3: 1 },
                'matrix': { speed: 25, intensity: 20, palette: '#008000', color2: '#008000', color3: '#008000', custom1: 8, custom2: 10, custom3: 1 }
            };
            
            if (defaults[this.selectedEffect]) {
                this.effectParams = { ...defaults[this.selectedEffect] };
            }
        },
        
        // Get effect description
        getEffectDescription() {
            const descriptions = {
                'simple_chase': 'Single colour dot chasing around the strip. Speed controls movement rate, Intensity controls brightness.',
                'rainbow': 'Smooth rainbow wave moving across the strip. Speed controls wave movement, Intensity controls density of colors, Wave Length controls how many colors appear simultaneously.',
                'twinkle': 'Random twinkling stars effect. Speed controls twinkle rate, Intensity controls number of active twinkles, Fade Speed controls how quickly stars fade out.',
                'chase': 'Multiple coloured dots chasing with trails using three settable colors. Speed controls movement, Intensity controls trail length, Dot Size controls the size of each colour dot.',
                'pulse': 'Entire strip pulsing in selected color. Speed controls pulse rate, Intensity controls brightness variation.',
                'matrix': 'Matrix-style digital rain effect. Speed controls falling rate, Intensity affects brightness variations, Drop Rate controls frequency of new drops.'
            };
            return descriptions[this.selectedEffect] || 'LED effect description not available.';
        },
        
        async turnOffLeds() {
            try {
                const response = await fetch('/api/leds-off', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' }
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
        
        // Section management methods
        showSection(sectionId) {
            this.activeSection = sectionId;
        },
        
        getSectionClass(sectionId) {
            const section = this.sections.find(s => s.id === sectionId);
            const baseClass = 'section-nav-btn';
            const colorClass = `section-nav-btn-${section?.color || 'purple'}`;
            const activeClass = this.activeSection === sectionId ? 'active' : '';
            return `${baseClass} ${colorClass} ${activeClass}`.trim();
        },
        
        // Quick prompt presets
        setQuickPrompt(type) {
            const prompts = {
                creative: "Generate creative, artistic content - poetry, short stories, or imaginative scenarios. Keep it engaging and printable.",
                doctorwho: "Generate content inspired by Doctor Who - time travel adventures, alien encounters, or sci-fi scenarios with a whimsical tone.",
                wisdom: "Share philosophical insights, life wisdom, or thought-provoking reflections. Keep it meaningful and contemplative.",
                humor: "Create funny content - jokes, witty observations, or humorous takes on everyday situations. Keep it light and entertaining."
            };
            
            if (prompts[type]) {
                this.config.unbiddenInk.prompt = prompts[type];
            }
        },
        
        // Check if a prompt preset is currently active
        isPromptActive(type) {
            const prompts = {
                creative: "Generate creative, artistic content - poetry, short stories, or imaginative scenarios. Keep it engaging and printable.",
                doctorwho: "Generate content inspired by Doctor Who - time travel adventures, alien encounters, or sci-fi scenarios with a whimsical tone.",
                wisdom: "Share philosophical insights, life wisdom, or thought-provoking reflections. Keep it meaningful and contemplative.",
                humor: "Create funny content - jokes, witty observations, or humorous takes on everyday situations. Keep it light and entertaining."
            };
            
            return this.config.unbiddenInk.prompt === prompts[type];
        },
        
        // Frequency slider specific values
        get frequencyOptions() {
            return [15, 30, 60, 120, 240, 360, 480]; // 15min, 30min, 1hr, 2hr, 4hr, 6hr, 8hr
        },
        
        get frequencySliderValue() {
            const options = this.frequencyOptions;
            const current = this.config?.unbiddenInk?.frequencyMinutes ?? 120;
            const index = options.indexOf(current);
            return index >= 0 ? index : 3; // Default to 2hr (index 3)
        },
        
        set frequencySliderValue(index) {
            this.config.unbiddenInk.frequencyMinutes = this.frequencyOptions[index] || 120;
        },
        
        // Cancel configuration changes
        cancelConfiguration() {
            // Navigate back to index instead of reloading
            window.location.href = '/';
        },
    };
}

// Export store initializer for use in HTML
window.initializeSettingsStore = initializeSettingsStore;