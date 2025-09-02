/**
 * @file diagnostics-runtime-config.js
 * @brief Alpine.js store factory for diagnostics runtime configuration page
 * @description Focused Alpine store for runtime configuration data
 */

import { loadConfiguration } from "../api/diagnostics.js";

/**
 * Create Diagnostics Runtime Config Alpine Store
 * Contains active configuration values and validation
 */
export function createDiagnosticsRuntimeConfigStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false,
    error: null,
    config: {},

    // ================== INITIALIZATION ==================
    async loadData() {
      this.loaded = false;
      this.error = null;
      try {
        const config = await loadConfiguration();
        this.config = config;
        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load configuration data: ${error.message}`;
        this.loaded = true;
      }
    },
  };
}
