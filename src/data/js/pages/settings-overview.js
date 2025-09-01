/**
 * @file settings-overview.js  
 * @brief Entry point for settings overview page
 * @description Imports and registers the settings overview Alpine.js store
 */

import { createSettingsOverviewStore } from '../stores/settings-overview.js';

// Register Alpine store when Alpine initializes
document.addEventListener('alpine:init', () => {
    const overviewStore = createSettingsOverviewStore();
    Alpine.store('settingsOverview', overviewStore);
});