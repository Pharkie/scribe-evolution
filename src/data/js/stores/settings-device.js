/**
 * @file settings-device.js
 * @brief Alpine.js store factory for device settings page
 * @description Focused Alpine store for device-specific configuration
 * Converted from legacy concatenated JavaScript to ES6 modules
 */

import { loadConfiguration, saveConfiguration } from '../api/settings.js';

export function createSettingsDeviceStore() {
    return {
        // ================== UTILITY FUNCTIONS ==================
        // Simple utility function extracted from repeated showMessage patterns
        showErrorMessage(message) {
            window.showMessage(message, 'error');
        },

        // ================== STATE MANAGEMENT ==================
        // Core state management
        loaded: false,
        config: {},
        error: null,
        saving: false,
        initialized: false,
        
        // Original configuration for change detection
        originalConfig: null,
        
        // Validation state
        validation: {
            errors: {}
        },
        
        // Timezone picker UI state
        searchQuery: '',

        // ================== DEVICE CONFIGURATION API ==================
        // Load configuration data from server
        async loadConfiguration() {
            // Prevent duplicate initialization
            if (this.initialized) {
                console.log('⚙️ Device Settings: Already initialized, skipping');
                return;
            }
            this.initialized = true;
            
            this.loaded = false;
            try {
                // Load configuration from API
                const serverConfig = await loadConfiguration();
                
                // Store original config for change detection
                this.originalConfig = JSON.parse(JSON.stringify(serverConfig));
                
                // Store all data in this.config
                this.config = serverConfig;
                
                // Preload timezone data for proper display formatting
                if (this.config.device?.timezone) {
                    await this.loadTimezones();
                }
                
                console.log('Alpine Device Store: Configuration loaded successfully');
                
            } catch (error) {
                console.error('Alpine Device Store: Failed to load configuration:', error);
                this.error = error.message;
            } finally {
                this.loaded = true;
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

        // TIMEZONE PICKER FUNCTIONALITY
        timezonePicker: {
            loading: false,
            error: null,
            timezones: [],
            initialized: false
        },

        // Computed property for filtered timezones
        get filteredTimezones() {
            if (!Array.isArray(this.timezonePicker.timezones) || this.timezonePicker.timezones.length === 0) {
                return [];
            }

            const query = (this.searchQuery || '').toLowerCase().trim();
            if (!query) {
                // Show top 5 popular timezones when no search query (alphabetical by city)
                const popularTimezones = [
                    'Europe/London', 'America/New_York', 'America/Sao_Paulo', 'Australia/Sydney', 'Asia/Tokyo'
                ];
                
                const popular = [];
                const others = [];
                
                this.timezonePicker.timezones.forEach(timezone => {
                    if (popularTimezones.includes(timezone.id)) {
                        popular.push(timezone);
                    } else {
                        others.push(timezone);
                    }
                });
                
                // Sort popular by the order in popularTimezones array
                popular.sort((a, b) => {
                    const aIndex = popularTimezones.indexOf(a.id);
                    const bIndex = popularTimezones.indexOf(b.id);
                    return aIndex - bIndex;
                });
                
                // Return popular first, then fill with others if needed
                return [...popular, ...others.slice(0, 5 - popular.length)].slice(0, 5);
            }

            // Search and score results by field priority
            const results = [];
            
            this.timezonePicker.timezones.forEach(timezone => {
                let priority = null;
                
                // Priority 1: City name (extracted from IANA ID)
                const parts = timezone.id.split('/');
                const city = parts[parts.length - 1].replace(/_/g, ' ').toLowerCase();
                if (city.includes(query)) {
                    priority = 1;
                } 
                // Priority 2: Display name
                else if (timezone.displayName.toLowerCase().includes(query)) {
                    priority = 2;
                } 
                // Priority 3: Timezone ID
                else if (timezone.id.toLowerCase().includes(query)) {
                    priority = 3;
                } 
                // Priority 4: Country name
                else if (timezone.countryName && timezone.countryName.toLowerCase().includes(query)) {
                    priority = 4;
                } 
                // Priority 5: Comments
                else if (timezone.comment && timezone.comment.toLowerCase().includes(query)) {
                    priority = 5;
                }
                
                if (priority !== null) {
                    results.push({ timezone, priority });
                }
            });

            // Sort by priority first, then alphabetically by display name
            return results
                .sort((a, b) => {
                    if (a.priority !== b.priority) {
                        return a.priority - b.priority;
                    }
                    return a.timezone.displayName.localeCompare(b.timezone.displayName);
                })
                .map(result => result.timezone);
        },

        // Load timezone data from API
        async loadTimezones() {
            if (this.timezonePicker.initialized) return;

            this.timezonePicker.loading = true;
            this.timezonePicker.error = null;

            try {
                const response = await fetch('/api/timezones');
                
                if (!response.ok) {
                    throw new Error(`HTTP ${response.status}: ${response.statusText}`);
                }

                const data = await response.json();
                
                if (!data.zones || !Array.isArray(data.zones)) {
                    throw new Error('Invalid timezone data format');
                }

                // Transform timezone data for frontend use
                this.timezonePicker.timezones = data.zones.map(zone => {
                    try {
                        // Extract city name from IANA ID (after last '/')
                        const parts = zone.id ? zone.id.split('/') : ['Unknown'];
                        const city = parts[parts.length - 1].replace(/_/g, ' ');
                        
                        // Create enhanced display name with comment and DST info
                        const countryName = zone.location && zone.location.countryName ? zone.location.countryName : null;
                        const comment = zone.location && zone.location.comment ? zone.location.comment.trim() : '';
                        
                        let displayName;
                        let offset = '';
                        
                        if (zone.offsets && Array.isArray(zone.offsets) && zone.offsets.length > 0) {
                            // Format offsets with :00 suffix for clarity
                            const formatOffset = (o) => {
                                const cleaned = o.replace(/^\+/, '+').replace(/^-/, '-').replace(/^00/, '+00');
                                return cleaned + ':00';
                            };
                            
                            if (zone.offsets.length === 1) {
                                // Single offset (no DST)
                                offset = 'UTC' + formatOffset(zone.offsets[0]);
                                displayName = countryName ? `${city}, ${countryName}` : (zone.id || 'Unknown');
                                if (comment) {
                                    displayName += ` — ${comment}`;
                                }
                            } else {
                                // Multiple offsets (DST zone) - first is standard, second is DST
                                const standardOffset = formatOffset(zone.offsets[0]);
                                const dstOffset = formatOffset(zone.offsets[1]);
                                offset = `UTC${standardOffset} / ${dstOffset} DST`;
                                
                                displayName = countryName ? `${city}, ${countryName}` : (zone.id || 'Unknown');
                                if (comment) {
                                    displayName += ` — ${comment}`;
                                }
                            }
                        } else {
                            // Fallback if no offsets available
                            displayName = countryName ? `${city}, ${countryName}` : (zone.id || 'Unknown');
                        }

                        return {
                            id: zone.id || 'Unknown',
                            displayName,
                            countryName: countryName,
                            comment: zone.location && zone.location.comment ? zone.location.comment : '',
                            aliases: zone.aliases || [],
                            offset
                        };
                    } catch (transformError) {
                        console.error('Error transforming timezone:', zone, transformError);
                        return {
                            id: zone.id || 'Unknown',
                            displayName: zone.id || 'Unknown',
                            countryName: '',
                            comment: '',
                            aliases: [],
                            offset: ''
                        };
                    }
                });

                this.timezonePicker.initialized = true;
                console.log(`Loaded ${this.timezonePicker.timezones.length} timezones`);

            } catch (error) {
                console.error('Failed to load timezone data:', error);
                console.error('Error details:', error.message, error.stack);
                this.timezonePicker.error = `Failed to load timezone data: ${error.message}`;
            } finally {
                this.timezonePicker.loading = false;
            }
        },

        // Load timezones and open dropdown
        async loadTimezonesAndOpen() {
            if (!this.timezonePicker.initialized && !this.timezonePicker.loading) {
                await this.loadTimezones();
            }
            this.isOpen = true;
        },

        // Open timezone picker (clear search on click/focus)
        async openTimezonePicker() {
            if (!this.timezonePicker.initialized && !this.timezonePicker.loading) {
                await this.loadTimezones();
            }
            this.searchQuery = '';
            this.isOpen = true;
        },

        // Reset focus index
        resetTimezoneFocus() {
            this.focusedIndex = -1;
        },

        // Handle global keydown to capture typing when dropdown is open
        handleGlobalKeydown(event, refs) {
            // Only handle when dropdown is open and not already focused on search input
            if (!this.isOpen || document.activeElement === refs.searchInput) {
                return;
            }

            // Handle printable characters (letters, numbers, space)
            if (event.key.length === 1 && !event.ctrlKey && !event.metaKey && !event.altKey) {
                // Focus search input and add the character
                refs.searchInput.focus();
                // Let the input handle the character naturally
                return;
            }

            // Handle backspace to focus input for editing
            if (event.key === 'Backspace') {
                event.preventDefault();
                refs.searchInput.focus();
                // Clear last character if search query exists
                if (this.searchQuery.length > 0) {
                    this.searchQuery = this.searchQuery.slice(0, -1);
                    this.onSearchInputWithReset();
                }
            }
        },

        // Close timezone picker
        closeTimezonePicker() {
            this.isOpen = false;
            this.focusedIndex = -1;
        },

        // Navigate up in timezone list (Alpine context version)
        navigateTimezoneUp(refs, nextTick) {
            this.focusedIndex = this.focusedIndex > 0 ? this.focusedIndex - 1 : -1;
            if (this.focusedIndex === -1) {
                refs.searchInput.focus();
            } else {
                nextTick(() => {
                    const options = refs.dropdown.querySelectorAll('.timezone-option');
                    options[this.focusedIndex]?.focus();
                });
            }
        },

        // Navigate down in timezone list (Alpine context version)
        navigateTimezoneDown(refs, nextTick) {
            const maxIndex = Math.min(this.filteredTimezones.length - 1, 4);
            this.focusedIndex = this.focusedIndex < maxIndex ? this.focusedIndex + 1 : 0;
            nextTick(() => {
                const options = refs.dropdown.querySelectorAll('.timezone-option');
                options[this.focusedIndex]?.focus();
            });
        },

        // Navigate to first timezone option from input
        navigateToFirstTimezone(refs, nextTick) {
            if (this.isOpen && this.filteredTimezones.length > 0) {
                this.focusedIndex = 0;
                nextTick(() => {
                    const options = refs.dropdown.querySelectorAll('.timezone-option');
                    options[0]?.focus();
                });
            }
        },

        // Navigate to last timezone option from input  
        navigateToLastTimezone(refs, nextTick) {
            if (this.isOpen && this.filteredTimezones.length > 0) {
                this.focusedIndex = Math.min(this.filteredTimezones.length - 1, 4);
                nextTick(() => {
                    const options = refs.dropdown.querySelectorAll('.timezone-option');
                    options[this.focusedIndex]?.focus();
                });
            }
        },

        // Handle search input changes
        onSearchInput() {
            // Ensure dropdown stays open when typing
            if (!this.isOpen && this.timezonePicker.initialized) {
                this.isOpen = true;
            }
        },

        // Handle search input with focus reset (Alpine-native single method)
        onSearchInputWithReset() {
            this.onSearchInput();
            this.resetTimezoneFocus();
        },

        // Select a timezone
        selectTimezone(timezone) {
            this.config.device.timezone = timezone.id;
            this.searchQuery = timezone.displayName;
            this.isOpen = false;
            
            // Clear any validation errors
            if (this.validation.errors['device.timezone']) {
                delete this.validation.errors['device.timezone'];
            }
            
            console.log('Selected timezone:', timezone.id);
        },

        // Get display name for a timezone ID (for dropdown)
        getTimezoneDisplayName(timezoneId) {
            if (!timezoneId) return '';
            
            const timezone = this.timezonePicker.timezones.find(tz => tz.id === timezoneId);
            if (timezone) {
                return `${timezone.displayName} (${timezone.offset})`;
            }
            
            // Fallback: convert IANA ID to readable format
            const parts = timezoneId.split('/');
            const city = parts[parts.length - 1].replace(/_/g, ' ');
            return `${city} (${timezoneId})`;
        },
        
        // Get display name for selected timezone (without offset)
        getSelectedTimezoneDisplayName(timezoneId) {
            const timezone = this.getSelectedTimezone(timezoneId);
            return timezone ? timezone.displayName : '';
        },
        
        // Get offset display for selected timezone  
        getSelectedTimezoneOffset(timezoneId) {
            const timezone = this.getSelectedTimezone(timezoneId);
            return timezone ? `(${timezone.offset})` : '';
        },
        
        // Helper to find selected timezone
        getSelectedTimezone(timezoneId) {
            return timezoneId ? this.timezonePicker.timezones.find(tz => tz.id === timezoneId) : null;
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
                const message = await saveConfiguration(partialConfig);
                
                console.log('Alpine Device Store: Configuration saved successfully');
                
                // Redirect immediately with success parameter
                window.location.href = '/settings/?saved=device';
                
            } catch (error) {
                console.error('Alpine Device Store: Failed to save configuration:', error);
                this.showErrorMessage('Failed to save device settings: ' + error.message);
                this.saving = false; // Only reset on error
            }
        },

        // Cancel configuration changes
        cancelConfiguration() {
            // Navigate back to settings
            window.location.href = '/settings/';
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
}