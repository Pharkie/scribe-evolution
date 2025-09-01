/**
 * @file setup.js
 * @brief Entry point for setup page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for AP mode setup
 */

import { createSetupStore } from '../stores/setup.js';

// Register the store with Alpine.js on initialization
document.addEventListener('alpine:init', () => {
    // Create and register setup store
    const setupStore = createSetupStore();
    Alpine.store('setup', setupStore);
    
    // Create Alpine data component for setup page
    Alpine.data('setupPage', () => ({
        // Expose all store properties as computed getters
        get loaded() { return this.$store.setup.loaded; },
        get error() { return this.$store.setup.error; },
        get saving() { return this.$store.setup.saving; },
        get scanning() { return this.$store.setup.scanning; },
        get config() { return this.$store.setup.config; },
        get manualSsid() { return this.$store.setup.manualSsid; },
        set manualSsid(value) { this.$store.setup.manualSsid = value; },
        get availableNetworks() { return this.$store.setup.availableNetworks; },
        get canSave() { return this.$store.setup.canSave; },
        
        // Expose store methods
        scanWiFi() { return this.$store.setup.scanWiFi(); },
        saveAndRestart() { return this.$store.setup.saveAndRestart(); },
        
        init() {
            // Initialize the store
            this.$store.setup.init();
        }
    }));
    
    // Make setup store instance available globally for the HTML (legacy compatibility)
    window.setupStoreInstance = {
        get loaded() { return Alpine.store('setup').loaded; },
        get error() { return Alpine.store('setup').error; },
        get saving() { return Alpine.store('setup').saving; },
        get scanning() { return Alpine.store('setup').scanning; },
        get config() { return Alpine.store('setup').config; },
        get manualSsid() { return Alpine.store('setup').manualSsid; },
        set manualSsid(value) { Alpine.store('setup').manualSsid = value; },
        get availableNetworks() { return Alpine.store('setup').availableNetworks; },
        get canSave() { return Alpine.store('setup').canSave; },
        scanWiFi() { return Alpine.store('setup').scanWiFi(); },
        saveAndRestart() { return Alpine.store('setup').saveAndRestart(); },
        init() { return Alpine.store('setup').init(); }
    };
    
    console.log('✅ Setup Store registered with ES6 modules');
});