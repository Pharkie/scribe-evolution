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

        // Transform the flat config object into entries for display
        const entries = [];
        const sections = new Set();

        const flattenObject = (obj, prefix = "", section = "root") => {
          for (const [key, value] of Object.entries(obj)) {
            const fullKey = prefix ? `${prefix}.${key}` : key;

            if (
              value &&
              typeof value === "object" &&
              !Array.isArray(value) &&
              value.constructor === Object
            ) {
              // For top-level objects, use the key as the section name
              const newSection = prefix === "" ? key : section;
              sections.add(newSection);
              // Nested object - recurse
              flattenObject(value, fullKey, newSection);
            } else {
              // Leaf value - only add section for non-root
              if (section !== "root") {
                sections.add(section);
              }
              entries.push({
                key: fullKey,
                value: String(value),
                type: typeof value,
                section: section,
              });
            }
          }
        };

        flattenObject(config);

        // Group entries by section for better display
        const entriesBySection = {};
        entries.forEach((entry) => {
          if (!entriesBySection[entry.section]) {
            entriesBySection[entry.section] = [];
          }
          entriesBySection[entry.section].push(entry);
        });

        this.config = {
          ...config,
          entries,
          entriesBySection,
          statistics: {
            totalEntries: entries.length,
            configSections: sections.size,
          },
        };

        this.loaded = true;
      } catch (error) {
        this.error = `Failed to load configuration data: ${error.message}`;
        this.loaded = true;
      }
    },
  };
}
