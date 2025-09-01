/**
 * @file settings-memos.js
 * @brief Entry point for memos settings page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for memo configuration
 */

import { createSettingsMemosStore } from '../stores/settings-memos.js';

// Register the store with Alpine.js on initialization
document.addEventListener('alpine:init', () => {
    // Create and register memos settings store
    const memosStore = createSettingsMemosStore();
    Alpine.store('settingsMemos', memosStore);
    
    // No manual init() - let HTML handle initialization timing with x-init
    console.log('✅ Memos Settings Store registered with ES6 modules');
});