/**
 * @file diagnostics-microcontroller.js
 * @brief Alpine.js store factory for diagnostics microcontroller page
 * @description Focused Alpine store for microcontroller system information
 */

import { loadDiagnostics, loadConfiguration } from "../api/diagnostics.js";

/**
 * Create Diagnostics Microcontroller Alpine Store
 * Contains system information, memory usage, and connection status
 */
export function createDiagnosticsMicrocontrollerStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false,
    error: null,
    microcontrollerInfo: {},
    memoryUsage: {},
    config: {},

    // ================== INITIALIZATION ==================
    async loadData() {
      this.loaded = false;
      this.error = null;
      try {
        // Load diagnostics and config data in parallel
        const [diagnostics, config] = await Promise.all([
          loadDiagnostics(),
          loadConfiguration(),
        ]);

        this.microcontrollerInfo = diagnostics.microcontrollerInfo || {};
        this.memoryUsage = diagnostics.memoryUsage || {};
        this.config = config;

        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load microcontroller data: ${error.message}`;
        this.loaded = true;
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    getProgressBarClass(percentage) {
      if (percentage < 50) return "bg-green-500";
      if (percentage < 80) return "bg-yellow-500";
      return "bg-red-500";
    },
  };
}
