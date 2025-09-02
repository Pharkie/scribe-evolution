/**
 * @file settings-buttons.js
 * @brief Entry point for buttons settings page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for button configuration
 */

import { createSettingsButtonsStore } from "../stores/settings-buttons.js";

// Register the store with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register buttons settings store
  const buttonsStore = createSettingsButtonsStore();
  Alpine.store("settingsButtons", buttonsStore);

  // No manual init() - let HTML handle initialization timing with x-init
  console.log("✅ Buttons Settings Store registered with ES6 modules");
});
