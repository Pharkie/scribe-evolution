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
    loading: false,
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
          chipModel: mc.chipModel,
          chipRevision: mc.chipRevision,
          cpuFrequency: `${mc.cpuFrequencyMhz} MHz`,
          sdkVersion: mc.sdkVersion,
          resetReason: mc.resetReason,
          temperature: tempCelsius
            ? `${tempCelsius.toFixed(1)}°C ${this.getTemperatureStatus(tempCelsius)}`
            : "Not available",
          temperatureRaw: tempCelsius,
          uptime: this.formatUptime(mc.uptimeMs),
          flashSize: this.formatBytes(mc.flash?.totalChipSize),
          firmwareVersion: mc.sdkVersion,
        };

        // Calculate memory usage percentages and formatted text
        const memory = mc.memory || {};
        const flashUsed =
          (mc.flash?.appPartition?.used || 0) +
          (mc.flash?.filesystem?.used || 0);
        const flashTotal = mc.flash?.totalChipSize || 1;
        const flashUsagePercent = (flashUsed / flashTotal) * 100;

        const heapUsagePercent = memory.totalHeap
          ? ((memory.usedHeap || 0) / memory.totalHeap) * 100
          : 0;

        this.memoryUsage = {
          ...memory,
          flashUsageText: `${this.formatBytes(flashUsed)} / ${this.formatBytes(flashTotal)}`,
          flashUsagePercent,
          heapUsageText: `${this.formatBytes(memory.usedHeap)} / ${this.formatBytes(memory.totalHeap)}`,
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

    // ================== ACTIONS ==================
    async printCharacterTest() {
      this.loading = true;
      try {
        // Fetch character test content directly from file
        const response = await fetch("/resources/character-test.txt");
        if (!response.ok) {
          throw new Error(`Failed to load character test: ${response.status}`);
        }
        const content = await response.text();

        // Print content using local print endpoint
        const printResponse = await fetch("/api/print-local", {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify({
            message: content,
          }),
        });

        if (!printResponse.ok) {
          throw new Error("Failed to print character test");
        }
      } catch (error) {
        console.error("Character test print failed:", error);
        // Could show error to user here if needed
      } finally {
        this.loading = false;
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    formatUptime(uptimeMs) {
      if (!uptimeMs) return "0ms";
      const seconds = Math.floor(uptimeMs / 1000);
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
