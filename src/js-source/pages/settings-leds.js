/**
 * @file settings-leds.js
 * @brief Entry point for LEDs settings page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for LED configuration
 */

import { createSettingsLedsStore } from "../stores/settings-leds.js";

// Register the store with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register LEDs settings store
  const ledsStore = createSettingsLedsStore();
  Alpine.store("settingsLeds", ledsStore);

  // No manual init() - let HTML handle initialization timing with x-init
  console.log("✅ LEDs Settings Store registered with ES6 modules");
});
