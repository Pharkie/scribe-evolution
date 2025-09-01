/**
 * @file settings-overview.js
 * @brief Alpine.js store factory for settings overview page
 * @description Focused Alpine store for settings page navigation and success feedback
 */

/**
 * Create Settings Overview Alpine Store
 * Contains navigation state and success feedback functionality
 */
export function createSettingsOverviewStore() {
    return {
        // ================== STATE MANAGEMENT ==================
        // Success feedback states
        deviceSaved: false,
        wifiSaved: false,
        mqttSaved: false,
        memosSaved: false,
        buttonsSaved: false,
        ledsSaved: false,
        unbiddenInkSaved: false,
        loaded: false,
        error: null,

        // ================== INITIALIZATION ==================
        loadData() {
            // Check for save success first (before setting loaded)
            this.checkSaveSuccess();
            // Set loaded state (no async data to load for overview page)
            this.loaded = true;
        },

        // ================== SUCCESS FEEDBACK ==================
        // Check for save success from URL parameter
        checkSaveSuccess() {
            const urlParams = new URLSearchParams(window.location.search);
            const saved = urlParams.get('saved');
            
            // List of valid saved types
            const validSavedTypes = ['device', 'wifi', 'mqtt', 'memos', 'buttons', 'leds', 'unbiddenInk'];
            
            if (saved && validSavedTypes.includes(saved)) {
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
}