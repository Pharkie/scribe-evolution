/**
 * @file 404.js
 * @brief ES6 entry point for 404 error page
 */

import { createErrorStore } from '../stores/404.js';

document.addEventListener('alpine:init', () => {
    // Prevent multiple initializations if alpine:init fires multiple times
    if (window.errorStoreInstance) {
        console.log('🗑️ 404: Store already exists, skipping alpine:init');
        return;
    }
    
    // Create and register error store immediately, initialize it once
    const errorStore = createErrorStore();
    Alpine.store('error', errorStore);
    
    // Make store available globally for body x-data
    window.errorStoreInstance = errorStore;
    
    // Initialize the store immediately during alpine:init (not later in x-init)
    errorStore.init();
    
    console.log('✅ 404 Error Store registered with ES6 modules');
});