/**
 * @file settings-wifi.js
 * @brief Entry point for WiFi settings page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for WiFi configuration
 */

import { createSettingsWifiStore } from "../stores/settings-wifi.js";

// Register the store with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register WiFi settings store
  const wifiStore = createSettingsWifiStore();
  Alpine.store("settingsWifi", wifiStore);

  // Setup Alpine watchers for reactive updates
  Alpine.effect(() => {
    // Update SSID whenever mode or selection changes
    wifiStore.updateSSID();
  });

  // No manual init() - let HTML handle initialization timing with x-init
  console.log("📡 WiFi Settings Store registered with ES6 modules");
});
