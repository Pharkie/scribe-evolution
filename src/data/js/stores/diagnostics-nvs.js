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
        const data = await loadNVSDump();
        // Transform NVS dump data into the expected structure
        this.nvs = {
          statistics: {
            totalEntries: Object.keys(data.keys || {}).length,
            usedSpace: this.calculateUsedSpace(data.keys || {}),
            freeSpace: "Unknown KB",
          },
          namespaces: [
            {
              name: data.namespace || "scribe-app",
              entryCount: Object.keys(data.keys || {}).length,
              description: "Main application settings and configuration",
              size: this.calculateUsedSpace(data.keys || {}),
              lastModified: data.timestamp || "Unknown",
            },
          ],
          entries: this.transformKeys(data.keys || {}),
        };
        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load NVS data: ${error.message}`;
        this.loaded = true;
      }
    },

    // ================== DATA TRANSFORMATION ==================
    calculateUsedSpace(keys) {
      const totalBytes = Object.values(keys).reduce(
        (sum, key) => sum + (key.length || 0),
        0,
      );
      return totalBytes < 1024
        ? `${totalBytes} B`
        : `${(totalBytes / 1024).toFixed(1)} KB`;
    },

    transformKeys(keys) {
      return Object.entries(keys).map(([keyName, keyData]) => ({
        key: keyName,
        type: keyData.type,
        value: keyData.value,
        size: keyData.length,
        namespace: "scribe-app",
      }));
    },

    // ================== UTILITY METHODS ==================
    getTypeClass(type) {
      const typeClasses = {
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
