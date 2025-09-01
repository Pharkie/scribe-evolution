/**
 * @file settings-unbidden-ink.js
 * @brief Entry point for Unbidden Ink settings page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for Unbidden Ink AI content configuration
 */

import { createSettingsUnbiddenInkStore } from '../stores/settings-unbidden-ink.js';

// Register the store with Alpine.js on initialization
document.addEventListener('alpine:init', () => {
    // Create and register Unbidden Ink settings store
    const unbiddenInkStore = createSettingsUnbiddenInkStore();
    Alpine.store('settingsUnbiddenInk', unbiddenInkStore);
    
    // No manual init() - let HTML handle initialization timing with x-init
    console.log('✅ Unbidden Ink Settings Store registered with ES6 modules');
});