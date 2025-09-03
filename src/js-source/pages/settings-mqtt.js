/**
 * @file settings-mqtt.js
 * @brief Entry point for MQTT settings page - imports and registers Alpine.js store
 * @description Modern ES6 module-based page initialization for MQTT configuration
 */

import { createSettingsMqttStore } from "../stores/settings-mqtt.js";

// Register the store with Alpine.js on initialization
document.addEventListener("alpine:init", () => {
  // Create and register MQTT settings store
  const mqttStore = createSettingsMqttStore();
  Alpine.store("settingsMqtt", mqttStore);

  // Setup Alpine watchers for reactive updates
  Alpine.effect(() => {
    // Clear validation errors and reset test state when MQTT is disabled
    if (mqttStore.config?.mqtt?.enabled === false) {
      mqttStore.validation.errors = {};
      mqttStore.resetMqttTestState();
    }
  });

  // No manual init() - let HTML handle initialization timing with x-init
  console.log("✅ MQTT Settings Store registered with ES6 modules");
});
