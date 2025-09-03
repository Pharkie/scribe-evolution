/**
 * @file setup.js
 * @brief Entry point for setup page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for AP mode setup
 */

import { createSetupStore } from "../stores/setup.js";

// Register the store with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register setup store
  const setupStore = createSetupStore();
  Alpine.store("setup", setupStore);

  // No Alpine.data wrapper - use direct store access

  console.log("✅ Setup Store registered with ES6 modules");
});
