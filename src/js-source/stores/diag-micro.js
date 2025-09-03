/**
 * @file diagnostics-microcontroller.js
 * @brief Alpine.js store factory for diagnostics device page (formerly microcontroller)
 * @description Focused Alpine store for device information including system, memory, logging
 */

import { loadDiagnostics, loadConfiguration } from "../api/diagnostics.js";

/**
 * Create Diagnostics Device Alpine Store
 * Contains system information, memory usage, connection status, and logging
 */
export function createDiagnosticsMicrocontrollerStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false,
    error: null,
    microcontrollerInfo: {},
    memoryUsage: {},
    config: {},
    logging: {},

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
        const tempCelsius = mc.temperature;

        this.microcontrollerInfo = {
          chipModel: mc.chip_model,
          chipRevision: mc.chip_revision,
          cpuFrequency: `${mc.cpu_frequency_mhz} MHz`,
          sdkVersion: mc.sdk_version,
          resetReason: mc.reset_reason,
          temperature: tempCelsius
            ? `${tempCelsius.toFixed(1)}°C ${this.getTemperatureStatus(tempCelsius)}`
            : "Not available",
          temperatureRaw: tempCelsius,
          uptime: this.formatUptime(mc.uptime_ms),
          flashSize: this.formatBytes(mc.flash?.total_chip_size),
          firmwareVersion: mc.sdk_version,
        };

        // Calculate memory usage percentages and formatted text
        const memory = mc.memory || {};
        const flashUsed =
          (mc.flash?.app_partition?.used || 0) +
          (mc.flash?.filesystem?.used || 0);
        const flashTotal = mc.flash?.total_chip_size || 1;
        const flashUsagePercent = (flashUsed / flashTotal) * 100;

        const heapUsagePercent = memory.total_heap
          ? ((memory.used_heap || 0) / memory.total_heap) * 100
          : 0;

        this.memoryUsage = {
          ...memory,
          flashUsageText: `${this.formatBytes(flashUsed)} / ${this.formatBytes(flashTotal)}`,
          flashUsagePercent,
          heapUsageText: `${this.formatBytes(memory.used_heap)} / ${this.formatBytes(memory.total_heap)}`,
          heapUsagePercent,
        };
        this.config = config;
        this.logging = diagnostics.logging || {};

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
      if (percentage < 50) return "bg-green-500 dark:bg-green-400";
      if (percentage < 80) return "bg-yellow-500 dark:bg-yellow-400";
      return "bg-red-500 dark:bg-red-400";
    },

    getTemperatureStatus(tempCelsius) {
      if (tempCelsius < 40) return "(Low)";
      if (tempCelsius > 50) return "(High)";
      return "(Normal)";
    },

    getTemperatureClass(tempCelsius) {
      if (tempCelsius < 40) return "text-blue-600 dark:text-blue-400";
      if (tempCelsius > 50) return "text-red-600 dark:text-red-400";
      return "text-green-600 dark:text-green-400";
    },

    // ================== LOGGING UTILITY FUNCTIONS ==================
    getLogLevelClass(level) {
      switch (level?.toLowerCase()) {
        case "error":
          return "border-red-500 dark:border-red-400";
        case "warning":
        case "warn":
          return "border-yellow-500 dark:border-yellow-400";
        case "info":
          return "border-blue-500 dark:border-blue-400";
        case "debug":
          return "border-gray-500 dark:border-gray-400";
        default:
          return "border-gray-300 dark:border-gray-600";
      }
    },

    getLogLevelDot(level) {
      switch (level?.toLowerCase()) {
        case "error":
          return "bg-red-500 dark:bg-red-400";
        case "warning":
        case "warn":
          return "bg-yellow-500 dark:bg-yellow-400";
        case "info":
          return "bg-blue-500 dark:bg-blue-400";
        case "debug":
          return "bg-gray-500 dark:bg-gray-400";
        default:
          return "bg-gray-300 dark:bg-gray-600";
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
