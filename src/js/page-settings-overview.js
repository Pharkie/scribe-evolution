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
        mqttSaved: false,
        memosSaved: false,
        buttonsSaved: false,
        ledsSaved: false,
        loading: true,
        error: null,

        // ================== INITIALIZATION ==================
        init() {
            this.checkSaveSuccess();
            // Simulate brief loading then ready state
            this.loading = false;
        },

        // ================== SUCCESS FEEDBACK ==================
        // Check for save success from URL parameter
        checkSaveSuccess() {
            const urlParams = new URLSearchParams(window.location.search);
            const saved = urlParams.get('saved');
            
            if (saved && this.hasOwnProperty(saved + 'Saved')) {
                this.showSuccessFeedback(saved);
            }
        },

        // Show success feedback for a given setting type
        showSuccessFeedback(settingType) {
            // Clean URL without reload
            const cleanUrl = window.location.pathname;
            window.history.replaceState({}, document.title, cleanUrl);
            
            // Show success feedback
            const savedProperty = settingType + 'Saved';
            this[savedProperty] = true;
            
            // Fade back to normal after 2 seconds
            setTimeout(() => {
                this[savedProperty] = false;
            }, 2000);
        }
    };

    return store;
}

// Create store immediately when script loads
const overviewStore = initializeSettingsOverviewStore();
window.settingsOverviewStoreInstance = overviewStore;

// Register with Alpine when ready
document.addEventListener('alpine:init', () => {
    Alpine.store('settingsOverview', overviewStore);
});