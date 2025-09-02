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

        // Map API properties to template-expected properties
        const mc = diagnostics.microcontroller || {};
        this.microcontrollerInfo = {
          chipModel: mc.chip_model,
          chipRevision: mc.chip_revision,
          cpuFrequency: `${mc.cpu_frequency_mhz} MHz`,
          sdkVersion: mc.sdk_version,
          resetReason: mc.reset_reason,
          temperature: `${mc.temperature?.toFixed(1)}°C`,
          uptime: this.formatUptime(mc.uptime_ms),
          flashSize: this.formatBytes(mc.flash?.total_chip_size),
          firmwareVersion: mc.sdk_version,
        };
        this.memoryUsage = mc.memory || {};
        this.config = config;

        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load microcontroller data: ${error.message}`;
        this.loaded = true;
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    formatUptime(uptime_ms) {
      if (!uptime_ms) return "0ms";
      const seconds = Math.floor(uptime_ms / 1000);
      const minutes = Math.floor(seconds / 60);
      const hours = Math.floor(minutes / 60);
      const days = Math.floor(hours / 24);

      if (days > 0) return `${days}d ${hours % 24}h`;
      if (hours > 0) return `${hours}h ${minutes % 60}m`;
      if (minutes > 0) return `${minutes}m ${seconds % 60}s`;
      return `${seconds}s`;
    },

    formatBytes(bytes) {
      if (!bytes) return "0 B";
      const k = 1024;
      const sizes = ["B", "KB", "MB", "GB"];
      const i = Math.floor(Math.log(bytes) / Math.log(k));
      return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + " " + sizes[i];
    },

    getProgressBarClass(percentage) {
      if (percentage < 50) return "bg-green-500";
      if (percentage < 80) return "bg-yellow-500";
      return "bg-red-500";
    },
  };
}
