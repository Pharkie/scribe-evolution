/**
 * @file diagnostics-logging.js
 * @brief Alpine.js store factory for diagnostics logging page
 * @description Focused Alpine store for system logging information
 */

import { loadDiagnostics } from "../api/diagnostics.js";

/**
 * Create Diagnostics Logging Alpine Store
 * Contains log configuration, recent entries, and debug statistics
 */
export function createDiagnosticsLoggingStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false,
    error: null,
    logging: {},

    // ================== INITIALIZATION ==================
    async loadData() {
      this.loaded = false;
      this.error = null;
      try {
        const diagnostics = await loadDiagnostics();
        this.logging = diagnostics.logging || {};
        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load logging data: ${error.message}`;
        this.loaded = true;
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    getLogLevelClass(level) {
      switch (level?.toLowerCase()) {
        case "error":
          return "border-red-500";
        case "warning":
        case "warn":
          return "border-yellow-500";
        case "info":
          return "border-blue-500";
        case "debug":
          return "border-gray-500";
        default:
          return "border-gray-300";
      }
    },

    getLogLevelDot(level) {
      switch (level?.toLowerCase()) {
        case "error":
          return "bg-red-500";
        case "warning":
        case "warn":
          return "bg-yellow-500";
        case "info":
          return "bg-blue-500";
        case "debug":
          return "bg-gray-500";
        default:
          return "bg-gray-300";
      }
    },

    getLogLevelText(level) {
      switch (level?.toLowerCase()) {
        case "error":
          return "text-red-600 dark:text-red-400";
        case "warning":
        case "warn":
          return "text-yellow-600 dark:text-yellow-400";
        case "info":
          return "text-blue-600 dark:text-blue-400";
        case "debug":
          return "text-gray-600 dark:text-gray-400";
        default:
          return "text-gray-500 dark:text-gray-500";
      }
    },
  };
}
