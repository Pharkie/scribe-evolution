/**
 * @file settings-device.js
 * @brief Entry point for device settings page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for device configuration
 */

import { createSettingsDeviceStore } from "../stores/settings-device.js";

// Register the store with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register device settings store
  const deviceStore = createSettingsDeviceStore();
  Alpine.store("settingsDevice", deviceStore);

  // No manual init() - let HTML handle initialization timing with x-init
  console.log("✅ Device Settings Store registered with ES6 modules");
});
