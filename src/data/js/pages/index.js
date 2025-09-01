/**
 * @file index.js  
 * @brief Entry point for index page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization
 */

import { createIndexStore } from '../stores/index.js';

// Register the store with Alpine.js on initialization
document.addEventListener('alpine:init', () => {
    // Create and register index store
    const indexStore = createIndexStore();
    Alpine.store('index', indexStore);
    
    console.log('✅ Index Store registered with ES6 modules');
});