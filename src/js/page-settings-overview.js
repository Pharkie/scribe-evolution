/**
 * @file page-settings-overview.js
 * @brief Alpine.js store for settings overview page
 * @description Focused Alpine store for settings page navigation and success feedback
 */

/**
 * Initialize Settings Overview Alpine Store
 * Contains navigation state and success feedback functionality
 */
function initializeSettingsOverviewStore() {
    const store = {
        // ================== STATE MANAGEMENT ==================
        // Success feedback states
        deviceSaved: false,
        wifiSaved: false,

        // ================== INITIALIZATION ==================
        init() {
            this.checkSaveSuccess();
        },

        // ================== SUCCESS FEEDBACK ==================
        // Check for save success from URL parameter
        checkSaveSuccess() {
            const urlParams = new URLSearchParams(window.location.search);
            const saved = urlParams.get('saved');
            
            if (saved === 'device') {
                // Clean URL without reload
                const cleanUrl = window.location.pathname;
                window.history.replaceState({}, document.title, cleanUrl);
                
                // Show success feedback
                this.deviceSaved = true;
                
                // Fade back to normal after 2 seconds
                setTimeout(() => {
                    this.deviceSaved = false;
                }, 2000);
            } else if (saved === 'wifi') {
                // Clean URL without reload
                const cleanUrl = window.location.pathname;
                window.history.replaceState({}, document.title, cleanUrl);
                
                // Show success feedback
                this.wifiSaved = true;
                
                // Fade back to normal after 2 seconds
                setTimeout(() => {
                    this.wifiSaved = false;
                }, 2000);
            }
        }
    };

    return store;
}

// Register the Alpine store
if (typeof Alpine !== 'undefined') {
    Alpine.store('settingsOverview', initializeSettingsOverviewStore());
} else {
    // Store initializer for when Alpine loads
    window.initializeSettingsOverviewStore = initializeSettingsOverviewStore;
}