/**
 * @file diagnostics-nvs.js
 * @brief Alpine.js store factory for diagnostics NVS storage page
 * @description Focused Alpine store for NVS storage diagnostics data
 */

import { loadNVSDump } from "../api/diagnostics.js";

/**
 * Create Diagnostics NVS Alpine Store
 * Contains NVS storage statistics, namespaces, and key-value pairs
 */
export function createDiagnosticsNvsStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false,
    error: null,
    nvs: {},

    // ================== INITIALIZATION ==================
    async loadData() {
      this.loaded = false;
      this.error = null;
      try {
        const nvsData = await loadNVSDump();

        // Transform the NVS dump data to match the template expectations
        const entries = [];
        const keys = nvsData.keys || {};

        // Convert keys object to array of entries
        for (const [key, value] of Object.entries(keys)) {
          if (value.exists) {
            entries.push({
              key: key,
              value: value.value,
              type: value.type,
              size: value.length || 0,
              namespace: nvsData.namespace || "scribe-app",
            });
          }
        }

        // Create namespaces info
        const namespaces = [
          {
            name: nvsData.namespace || "scribe-app",
            description: "Main application configuration namespace",
            entryCount: entries.length,
            size: "~2KB", // ESP32 doesn't provide exact namespace sizes
            lastModified: nvsData.timestamp || "Unknown",
          },
        ];

        this.nvs = {
          ...nvsData,
          entries,
          namespaces,
          statistics: {
            totalEntries: nvsData.summary?.totalKeys || entries.length,
            usedSpace: "~2KB", // ESP32 doesn't provide exact space usage
            freeSpace: "~6KB", // NVS is typically 8KB total on ESP32
          },
        };

        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load NVS data: ${error.message}`;
        this.loaded = true;
      }
    },

    // ================== UTILITY METHODS ==================
    getTypeClass(type) {
      const typeClasses = {
        // ESP32 NVS types
        string: "bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200",
        int: "bg-blue-100 text-blue-800 dark:bg-blue-900 dark:text-blue-200",
        bool: "bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200",
        // Legacy low-level types for compatibility
        u8: "bg-blue-100 text-blue-800 dark:bg-blue-900 dark:text-blue-200",
        u16: "bg-green-100 text-green-800 dark:bg-green-900 dark:text-green-200",
        u32: "bg-yellow-100 text-yellow-800 dark:bg-yellow-900 dark:text-yellow-200",
        i8: "bg-purple-100 text-purple-800 dark:bg-purple-900 dark:text-purple-200",
        i16: "bg-pink-100 text-pink-800 dark:bg-pink-900 dark:text-pink-200",
        i32: "bg-indigo-100 text-indigo-800 dark:bg-indigo-900 dark:text-indigo-200",
        str: "bg-red-100 text-red-800 dark:bg-red-900 dark:text-red-200",
        blob: "bg-gray-100 text-gray-800 dark:bg-gray-900 dark:text-gray-200",
      };
      return (
        typeClasses[type] ||
        "bg-gray-100 text-gray-800 dark:bg-gray-900 dark:text-gray-200"
      );
    },

    getDisplayValue(value, type) {
      if (value === null || value === undefined) {
        return "null";
      }

      if (type === "blob") {
        return `[${value.length || 0} bytes]`;
      }

      if (typeof value === "string" && value.length > 100) {
        return value.substring(0, 100) + "...";
      }

      return String(value);
    },
  };
}
