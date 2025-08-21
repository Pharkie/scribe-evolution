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
                // Core state management
        loading: true,
        error: null,
        saving: false,
        initialized: false, // Flag to prevent duplicate initialization
        
        // Configuration data (reactive) - matching backend API structure
        config: {
            device: {
                owner: '',
                timezone: '',
                maxCharacters: 1000,
                mqtt_topic: '',
                mdns: '',
                // WiFi nested under device
                wifi: {
                    ssid: '',
                    password: '',
                    connect_timeout: 15000,
                    status: {
                        connected: false,
                        ip_address: '',
                        mac_address: '',
                        gateway: '',
                        dns: '',
                        signal_strength: ''
                    }
                }
            },
            mqtt: {
                server: '',
                port: 1883,
                username: '',
                password: '',
                connected: false
            },
            unbiddenInk: {
                enabled: false,
                startHour: 9,
                endHour: 21,
                frequencyMinutes: 180,
                prompt: 'Generate something creative and interesting',
                chatgptApiToken: ''
            },
            buttons: {
                // Hardware configuration
                count: null,
                debounce_time: null,
                long_press_time: null,
                active_low: null,
                min_interval: null,
                max_per_minute: null,
                // Individual button configurations
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
        activeSection: 'device',
        showValidationFeedback: false,
        
        // LED Effect Parameters (WLED-style unified interface)
        selectedEffect: 'chase_single',
        effectParams: {
            speed: 10,
            intensity: 50,
            cycles: 1,
            color1: '#0062ff',
            color2: '#00ff00',
            color3: '#ff0000',
            custom1: 10,
            custom2: 5,
            custom3: 1
        },
        
        // Color picker instances
        colorPickers: {
            color1: null,
            color2: null,
            color3: null
        },
        
        // Section definitions for navigation
        sections: [
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
                // Don't show message here - let the UI handle the error display
            } finally {
                this.loading = false;
            }
        },
        
        // Create a clean config object without read-only fields for server submission
        createCleanConfig() {
            const cleanConfig = {
                device: {
                    owner: this.config.device.owner,
                    timezone: this.config.device.timezone,
                    // Include WiFi nested under device
                    wifi: {
                        ssid: this.config.device.wifi.ssid,
                        password: this.config.device.wifi.password,
                        connect_timeout: this.config.device.wifi.connect_timeout
                        // Exclude status as it's read-only
                    }
                    // Exclude mqtt_topic as it's read-only
                },
                mqtt: {
                    server: this.config.mqtt.server,
                    port: this.config.mqtt.port,
                    username: this.config.mqtt.username,
                    password: this.config.mqtt.password
                    // Exclude connected as it's read-only
                },
                unbiddenInk: {
                    enabled: this.config.unbiddenInk.enabled,
                    startHour: this.config.unbiddenInk.startHour,
                    endHour: this.config.unbiddenInk.endHour,
                    frequencyMinutes: this.config.unbiddenInk.frequencyMinutes,
                    prompt: this.config.unbiddenInk.prompt,
                    chatgptApiToken: this.config.unbiddenInk.chatgptApiToken
                },
                buttons: {
                    // Include individual button configurations
                    button1: this.config.buttons.button1,
                    button2: this.config.buttons.button2,
                    button3: this.config.buttons.button3,
                    button4: this.config.buttons.button4
                    // Exclude hardware config as it's read-only
                },
                leds: {
                    pin: this.config.leds.pin,
                    count: this.config.leds.count,
                    brightness: this.config.leds.brightness,
                    refreshRate: this.config.leds.refreshRate
                }
            };
            
            console.log('Alpine Store: Created clean config for server:', cleanConfig);
            return cleanConfig;
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
                // Create a clean copy of config without read-only fields
                const cleanConfig = this.createCleanConfig();
                
                // Use existing SettingsAPI with cleaned config
                const message = await window.SettingsAPI.saveConfiguration(cleanConfig);
                
                this.showValidationFeedback = false;
                
                // Redirect to index page with stashed indicator
                window.location.href = '/?settings=stashed';
                
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
        
        // Deep merge server config into reactive state with error logging
        mergeConfig(serverConfig) {
            console.log('🔧 Merging server config:', serverConfig);
            
            // Device - log errors for missing critical values
            if (serverConfig.device) {
                this.config.device.owner = serverConfig.device.owner || '';
                this.config.device.timezone = serverConfig.device.timezone || '';
                this.config.device.mqtt_topic = serverConfig.device.mqtt_topic || '';
                this.config.device.mdns = serverConfig.device.mdns || '';
                this.config.device.maxCharacters = serverConfig.device.maxCharacters || 1000;
                
                if (!serverConfig.device.owner) {
                    console.warn('⚠️ Missing device.owner in config');
                }
                if (!serverConfig.device.timezone) {
                    console.warn('⚠️ Missing device.timezone in config');
                }
            } else {
                console.error('❌ Missing device section in config');
            }
            
            // WiFi - log errors for missing values  
            if (serverConfig.device?.wifi) {
                this.config.device.wifi.ssid = serverConfig.device.wifi.ssid || '';
                this.config.device.wifi.password = serverConfig.device.wifi.password || '';
                this.config.device.wifi.connect_timeout = serverConfig.device.wifi.connect_timeout || 15000;
                
                if (!serverConfig.device.wifi.ssid) {
                    console.warn('⚠️ Missing device.wifi.ssid in config');
                }
                
                // Load WiFi status data if available
                if (serverConfig.device.wifi.status) {
                    this.config.device.wifi.status.connected = serverConfig.device.wifi.status.connected || false;
                    this.config.device.wifi.status.ip_address = serverConfig.device.wifi.status.ip_address || '';
                    this.config.device.wifi.status.mac_address = serverConfig.device.wifi.status.mac_address || '';
                    this.config.device.wifi.status.gateway = serverConfig.device.wifi.status.gateway || '';
                    this.config.device.wifi.status.dns = serverConfig.device.wifi.status.dns || '';
                    this.config.device.wifi.status.signal_strength = serverConfig.device.wifi.status.signal_strength || '';
                } else {
                    console.warn('⚠️ Missing device.wifi.status in config');
                }
            } else {
                console.error('❌ Missing device.wifi section in config');
            }
            
            // MQTT - log errors for missing values
            if (serverConfig.mqtt) {
                this.config.mqtt.server = serverConfig.mqtt.server || '';
                this.config.mqtt.port = serverConfig.mqtt.port || 1883;
                this.config.mqtt.username = serverConfig.mqtt.username || '';
                this.config.mqtt.password = serverConfig.mqtt.password || '';
                this.config.mqtt.connected = serverConfig.mqtt.connected || false;
                
                if (!serverConfig.mqtt.server) {
                    console.warn('⚠️ Missing mqtt.server in config');
                }
            } else {
                console.warn('⚠️ Missing mqtt section in config');
            }
            
            // Unbidden Ink - log errors for missing values
            if (serverConfig.unbiddenInk) {
                this.config.unbiddenInk.enabled = serverConfig.unbiddenInk.enabled || false;
                this.config.unbiddenInk.startHour = serverConfig.unbiddenInk.startHour || 8;
                this.config.unbiddenInk.endHour = serverConfig.unbiddenInk.endHour || 22;
                this.config.unbiddenInk.frequencyMinutes = serverConfig.unbiddenInk.frequencyMinutes || 120;
                this.config.unbiddenInk.prompt = serverConfig.unbiddenInk.prompt || '';
                this.config.unbiddenInk.chatgptApiToken = serverConfig.unbiddenInk.chatgptApiToken || '';
            } else {
                console.warn('⚠️ Missing unbiddenInk section in config');
            }
            
            // Buttons - log errors for missing values
            if (serverConfig.buttons) {
                // Copy hardware configuration properties
                this.config.buttons.count = serverConfig.buttons.count || null;
                this.config.buttons.debounce_time = serverConfig.buttons.debounce_time || null;
                this.config.buttons.long_press_time = serverConfig.buttons.long_press_time || null;
                this.config.buttons.active_low = serverConfig.buttons.active_low || null;
                this.config.buttons.min_interval = serverConfig.buttons.min_interval || null;
                this.config.buttons.max_per_minute = serverConfig.buttons.max_per_minute || null;

                // Copy individual button configurations
                for (let i = 1; i <= 4; i++) {
                    const buttonKey = `button${i}`;
                    if (serverConfig.buttons[buttonKey]) {
                        this.config.buttons[buttonKey] = {
                            shortAction: serverConfig.buttons[buttonKey].shortAction || '',
                            longAction: serverConfig.buttons[buttonKey].longAction || '',
                            shortMqttTopic: serverConfig.buttons[buttonKey].shortMqttTopic || '',
                            longMqttTopic: serverConfig.buttons[buttonKey].longMqttTopic || ''
                        };
                    } else {
                        console.warn(`⚠️ Missing buttons.${buttonKey} config`);
                    }
                }
            } else {
                console.warn('⚠️ Missing buttons section in config');
            }
            
            // LEDs - log errors for missing values
            if (serverConfig.leds) {
                this.config.leds.pin = serverConfig.leds.pin || 4;
                this.config.leds.count = serverConfig.leds.count || 60;
                this.config.leds.brightness = serverConfig.leds.brightness || 128;
                this.config.leds.refreshRate = serverConfig.leds.refreshRate || 60;
            } else {
                console.warn('⚠️ Missing leds section in config');
            }
            
            console.log('✅ Config merge complete, final config:', this.config);
        },
        
        // Message display (delegates to existing system)
        // Form validation
        validateField(fieldName, value) {
            this.validation.touched[fieldName] = true;
            
            // Clear previous errors for this field
            if (this.validation.errors[fieldName]) {
                delete this.validation.errors[fieldName];
            }
            
            // Required field validation
            const requiredFields = ['device.owner', 'device.wifi.ssid'];
            if (this.config.unbiddenInk.enabled) {
                requiredFields.push('unbiddenInk.chatgptApiToken');
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
                case 'device.wifi.connect_timeout':
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
                'device.wifi.ssid', 
                'device.wifi.connect_timeout',
                'mqtt.port'
            ];
            
            if (this.config.unbiddenInk.enabled) {
                fieldsToValidate.push('unbiddenInk.chatgptApiToken');
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
                if (effectName === 'chase_multi') {
                    // Multicolour chase uses 3 colors
                    colors = [
                        this.effectParams.color1,
                        this.effectParams.color2,
                        this.effectParams.color3
                    ];
                } else {
                    // All other effects use single color
                    colors = [this.effectParams.color1];
                }

                // Build unified payload - use cycles instead of duration
                let effectParams = {
                    effect: effectName,
                    cycles: this.effectParams.cycles, // Use cycles from the slider
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
                    const cycleText = this.effectParams.cycles === 1 ? '1 cycle' : `${this.effectParams.cycles} cycles`;
                    window.showMessage(`Testing ${effectName} effect for ${cycleText}`, 'info');
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
                case 'chase_single':
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
                case 'chase_multi':
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
                case 'chase_multi': return 'Dot Size';
                default: return null;
            }
        },
        
        getCustomSlider1Range() {
            switch(this.selectedEffect) {
                case 'rainbow': return { min: 1, max: 20, step: 1 };
                case 'twinkle': return { min: 1, max: 10, step: 1 };
                case 'chase_multi': return { min: 1, max: 10, step: 1 };
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
                'chase_single': { speed: 10, intensity: 50, color1: '#0062ff', color2: '#0062ff', color3: '#0062ff', custom1: 10, custom2: 5, custom3: 1 },
                'rainbow': { speed: 20, intensity: 5, color1: '#ff0000', color2: '#ff0000', color3: '#ff0000', custom1: 5, custom2: 3, custom3: 1 },
                'twinkle': { speed: 5, intensity: 10, color1: '#ffff00', color2: '#ffff00', color3: '#ffff00', custom1: 3, custom2: 2, custom3: 1 },
                'chase_multi': { speed: 15, intensity: 10, color1: '#ff0000', color2: '#00ff00', color3: '#0000ff', custom1: 3, custom2: 5, custom3: 1 },
                'pulse': { speed: 4, intensity: 50, color1: '#800080', color2: '#800080', color3: '#800080', custom1: 5, custom2: 3, custom3: 1 },
                'matrix': { speed: 25, intensity: 20, color1: '#008000', color2: '#008000', color3: '#008000', custom1: 8, custom2: 10, custom3: 1 }
            };
            
            if (defaults[this.selectedEffect]) {
                this.effectParams = { ...defaults[this.selectedEffect] };
            }
        },
        
        // Get effect description
        getEffectDescription() {
            const descriptions = {
                'chase_single': 'Single colour dot chasing around the strip. Speed controls movement rate, Intensity controls brightness.',
                'rainbow': 'Smooth rainbow wave moving across the strip. Speed controls wave movement, Intensity controls density of colors, Wave Length controls how many colors appear simultaneously.',
                'twinkle': 'Random twinkling stars effect. Speed controls twinkle rate, Intensity controls number of active twinkles, Fade Speed controls how quickly stars fade out.',
                'chase_multi': 'Multiple coloured dots chasing with trails using three settable colors. Speed controls movement, Intensity controls trail length, Dot Size controls the size of each colour dot.',
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
        
        get frequencyLabels() {
            return this.frequencyOptions.map(minutes => {
                if (minutes < 60) {
                    return `${minutes}min`;
                } else {
                    const hours = minutes / 60;
                    return `${hours}hr`;
                }
            });
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
        
        // Initialize Pickr color pickers
        initColorPickers() {
            console.log('🎨 Initializing Pickr color pickers...');
            
            // Ensure we have valid color values with fallbacks
            const safeColor1 = this.effectParams.color1 && typeof this.effectParams.color1 === 'string' ? this.effectParams.color1 : '#0062ff';
            const safeColor2 = this.effectParams.color2 && typeof this.effectParams.color2 === 'string' ? this.effectParams.color2 : '#00ff00';
            const safeColor3 = this.effectParams.color3 && typeof this.effectParams.color3 === 'string' ? this.effectParams.color3 : '#ff0000';
            
            console.log('🎨 Color picker values:', { safeColor1, safeColor2, safeColor3 });
            
            // Common Pickr configuration
            const commonConfig = {
                theme: 'nano',
                default: '#0062ff',
                swatches: [
                    '#FF0000', // Red
                    '#1E90FF', // Electric Blue  
                    '#32CD32', // Lime Green
                    '#FFFF00', // Bright Yellow
                    '#FFA500', // Orange
                    '#800080'  // Purple
                ],
                components: {
                    preview: true,
                    hue: true,
                    interaction: {
                        hex: true,
                        input: true,
                        save: true
                    }
                }
            };
            
            // Initialize color1 picker
            if (this.$refs.color1Pickr) {
                this.colorPickers.color1 = Pickr.create({
                    ...commonConfig,
                    el: this.$refs.color1Pickr,
                    default: safeColor1
                });
                
                this.colorPickers.color1.on('save', (color) => {
                    if (color) {
                        this.effectParams.color1 = color.toHEXA().toString();
                    }
                });
            }
            
            // Initialize color2 picker (for multicolor chase)
            if (this.$refs.color2Pickr) {
                this.colorPickers.color2 = Pickr.create({
                    ...commonConfig,
                    el: this.$refs.color2Pickr,
                    default: safeColor2
                });
                
                this.colorPickers.color2.on('save', (color) => {
                    if (color) {
                        this.effectParams.color2 = color.toHEXA().toString();
                    }
                });
            }
            
            // Initialize color3 picker (for multicolor chase)
            if (this.$refs.color3Pickr) {
                this.colorPickers.color3 = Pickr.create({
                    ...commonConfig,
                    el: this.$refs.color3Pickr,
                    default: safeColor3
                });
                
                this.colorPickers.color3.on('save', (color) => {
                    if (color) {
                        this.effectParams.color3 = color.toHEXA().toString();
                    }
                });
            }
        },
    };
}

// Export store initializer for use in HTML
window.initializeSettingsStore = initializeSettingsStore;