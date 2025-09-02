/**
 * @file diagnostics.js
 * @brief Alpine.js reactive store for diagnostics page functionality
 */

import {
  loadDiagnostics,
  loadConfiguration,
  loadNVSDump,
  executeQuickAction,
  printLocalContent,
  loadRoutes,
} from "../api/diagnostics.js";

// Export factory function for creating the diagnostics store
export function createDiagnosticsStore() {
  const store = {
    // Core state
    error: null,
    initialized: false, // Flag to prevent duplicate initialization
    loaded: false, // Flag to indicate data has been loaded
    data: {},

    // UI state
    currentSection: "microcontroller-section",
    progressReady: false,

    // Section definitions
    sections: [
      {
        id: "microcontroller-section",
        name: "Microcontroller",
        icon: "cpuChip",
        color: "orange",
      },
      {
        id: "logging-section",
        name: "Logging",
        icon: "clipboardDocumentList",
        color: "indigo",
      },
      {
        id: "pages-endpoints-section",
        name: "Routes",
        icon: "link",
        color: "teal",
      },
      {
        id: "config-file-section",
        name: "Runtime Configuration",
        icon: "cog6Tooth",
        color: "green",
      },
      {
        id: "nvs-storage-section",
        name: "NVS",
        icon: "saveFloppyDisk",
        color: "cyan",
      },
    ],

    // Load diagnostics data (main initialization method)
    async loadDiagnostics() {
      // Prevent duplicate initialization
      if (this.initialized) {
        console.log("🛠️ Diagnostics: Already initialized, skipping");
        return;
      }
      this.initialized = true;
      this.loaded = false;
      this.error = null;

      try {
        // Load diagnostics, config, NVS, and routes data in parallel with individual error handling
        const [
          diagnosticsResponse,
          configResponse,
          nvsResponse,
          routesResponse,
        ] = await Promise.allSettled([
          loadDiagnostics(),
          loadConfiguration(),
          loadNVSDump(),
          loadRoutes(),
        ]);

        // Check if at least one API succeeded
        const anyApiSuccess =
          diagnosticsResponse.status === "fulfilled" ||
          configResponse.status === "fulfilled" ||
          nvsResponse.status === "fulfilled" ||
          routesResponse.status === "fulfilled";

        if (!anyApiSuccess) {
          // All APIs failed - this is an error state
          this.error =
            "All diagnostic APIs are unavailable. Please check the system.";
          return;
        }

        // Parse responses with error logging for failed APIs
        this.data.diagnostics = {};
        this.data.config = {};
        this.data.nvs = {};
        this.data.routes = {};

        if (diagnosticsResponse.status === "fulfilled") {
          this.data.diagnostics = diagnosticsResponse.value;
        } else {
          console.error(
            "❌ Diagnostics API failed - diagnostics data will be incomplete:",
            diagnosticsResponse.reason,
          );
        }

        if (configResponse.status === "fulfilled") {
          this.data.config = configResponse.value;
        } else {
          console.error(
            "❌ Config API failed - configuration data will be incomplete:",
            configResponse.reason,
          );
        }

        if (nvsResponse.status === "fulfilled") {
          this.data.nvs = nvsResponse.value;
        } else {
          console.error(
            "❌ NVS API failed - NVS storage data will be incomplete:",
            nvsResponse.reason,
          );
        }

        if (routesResponse.status === "fulfilled") {
          this.data.routes = routesResponse.value;
        } else {
          console.error(
            "❌ Routes API failed - routes data will be incomplete:",
            routesResponse.reason,
          );
        }

        this.error = null;
        this.loaded = true;
      } catch (error) {
        console.error(
          "🛠️ Diagnostics: Unexpected error loading diagnostics:",
          error,
        );
        // Unexpected error - show error state
        this.error = `Unexpected error loading diagnostics: ${error.message}`;
      }
    },

    // Section management
    showSection(sectionId) {
      this.currentSection = sectionId;
    },

    // Trigger progress bar animations when content is ready
    triggerProgressAnimations() {
      // Use requestAnimationFrame for smooth timing
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          this.progressReady = true;
        });
      });
    },

    getSectionClass(sectionId) {
      const section = this.sections.find((s) => s.id === sectionId);
      const baseClass = "section-nav-btn";
      const colorClass = `section-nav-btn-${section?.color || "purple"}`;
      const activeClass = this.currentSection === sectionId ? "active" : "";
      return `${baseClass} ${colorClass} ${activeClass}`.trim();
    },

    // Microcontroller computed properties - show errors instead of silent fallbacks
    get microcontrollerInfo() {
      if (!this.loaded) {
        return {
          chipModel: null,
          cpuFrequency: null,
          flashSize: null,
          firmwareVersion: null,
          uptime: null,
          temperature: null,
        };
      }

      const microcontroller = this.data.diagnostics?.microcontroller;

      if (!microcontroller) {
        console.error("❌ Missing microcontroller data from diagnostics API");
        return {
          chipModel: "ERROR: Missing Data",
          cpuFrequency: "ERROR: Missing Data",
          flashSize: "ERROR: Missing Data",
          firmwareVersion: "ERROR: Missing Data",
          uptime: "ERROR: Missing Data",
          temperature: "ERROR: Missing Data",
        };
      }

      return {
        chipModel: microcontroller.chip_model || "ERROR: Missing chip_model",
        cpuFrequency: microcontroller.cpu_frequency_mhz
          ? `${microcontroller.cpu_frequency_mhz} MHz`
          : "ERROR: Missing frequency",
        flashSize:
          this.formatBytes(microcontroller.flash?.total_chip_size) ||
          "ERROR: Missing flash size",
        firmwareVersion:
          microcontroller.sdk_version || "ERROR: Missing SDK version",
        uptime: microcontroller.uptime_ms
          ? this.formatUptime(microcontroller.uptime_ms / 1000)
          : "ERROR: Missing uptime",
        temperature:
          this.formatTemperature(microcontroller.temperature) ||
          "ERROR: Missing temperature",
      };
    },

    // Memory usage computed properties - show errors instead of silent fallbacks
    get memoryUsage() {
      if (!this.loaded) {
        return {
          flashUsageText: null,
          heapUsageText: null,
          flashUsagePercent: 0,
          heapUsagePercent: 0,
        };
      }

      const microcontroller = this.data.diagnostics?.microcontroller;

      if (!microcontroller) {
        console.error("❌ Missing microcontroller data for memory usage");
        return {
          flashUsageText: "ERROR: Missing Data",
          heapUsageText: "ERROR: Missing Data",
          flashUsagePercent: 0,
          heapUsagePercent: 0,
        };
      }

      const flash = microcontroller.flash?.app_partition;
      const memory = microcontroller.memory;

      if (!flash) {
        console.error("❌ Missing flash data in microcontroller diagnostics");
      }
      if (!memory) {
        console.error("❌ Missing memory data in microcontroller diagnostics");
      }

      const flashUsed = flash?.total
        ? ((flash.used || 0) / flash.total) * 100
        : 0;
      const heapUsed = memory?.total_heap
        ? ((memory.used_heap || 0) / memory.total_heap) * 100
        : 0;

      return {
        flashUsageText: flash
          ? `${this.formatBytes(flash.used || 0)} / ${this.formatBytes(flash.total || 0)} (${flashUsed.toFixed(0)}%)`
          : "ERROR: Missing flash data",
        heapUsageText: memory
          ? `${this.formatBytes(memory.used_heap || 0)} / ${this.formatBytes(memory.total_heap || 0)} (${heapUsed.toFixed(0)}%)`
          : "ERROR: Missing memory data",
        flashUsagePercent: flashUsed,
        heapUsagePercent: heapUsed,
      };
    },

    // Logging computed properties - show errors instead of silent fallbacks
    get loggingInfo() {
      if (!this.loaded) {
        return {
          level: null,
          serialLogging: null,
          webLogging: null,
          fileLogging: null,
          mqttLogging: null,
        };
      }

      const logging = this.data.diagnostics?.logging;

      if (!logging) {
        console.error("❌ Missing logging data from diagnostics API");
        return {
          level: "ERROR: Missing Data",
          serialLogging: "ERROR: Missing Data",
          webLogging: "ERROR: Missing Data",
          fileLogging: "ERROR: Missing Data",
          mqttLogging: "ERROR: Missing Data",
        };
      }

      return {
        level: logging.level_name || "ERROR: Missing level_name",
        serialLogging:
          logging.serial_enabled !== undefined
            ? logging.serial_enabled
              ? "Enabled"
              : "Disabled"
            : "ERROR: Missing serial_enabled",
        webLogging:
          logging.betterstack_enabled !== undefined
            ? logging.betterstack_enabled
              ? "Enabled"
              : "Disabled"
            : "ERROR: Missing betterstack_enabled",
        fileLogging:
          logging.file_enabled !== undefined
            ? logging.file_enabled
              ? "Enabled"
              : "Disabled"
            : "ERROR: Missing file_enabled",
        mqttLogging:
          logging.mqtt_enabled !== undefined
            ? logging.mqtt_enabled
              ? "Enabled"
              : "Disabled"
            : "ERROR: Missing mqtt_enabled",
      };
    },

    // Web pages computed properties - show errors instead of silent fallbacks
    get sortedRoutes() {
      if (!this.loaded) {
        return [];
      }

      const routes = this.data.routes?.web_pages;

      if (!routes) {
        console.error("❌ Missing web_pages data from routes API");
        return [
          {
            path: "ERROR: Missing Data",
            description: "Web pages data not available from routes API",
            isError: true,
          },
        ];
      }

      // Separate HTML pages from other routes
      const htmlPages = [];
      const otherRoutes = [];

      routes.forEach((route) => {
        if (route.path.endsWith(".html") || route.path === "/") {
          htmlPages.push({
            ...route,
            isHtmlPage: true,
            linkPath: route.path === "/" ? "/" : route.path,
          });
        } else if (route.path === "(unmatched routes)") {
          otherRoutes.push({
            ...route,
            isUnmatched: true,
            linkPath: "/404",
            path: "*",
            description: "404 handler",
          });
        } else {
          otherRoutes.push({
            ...route,
            isHtmlPage: false,
          });
        }
      });

      // Sort HTML pages alphabetically by path
      htmlPages.sort((a, b) => {
        // Put '/' first
        if (a.path === "/") return -1;
        if (b.path === "/") return 1;
        return a.path.localeCompare(b.path);
      });

      // Sort other routes alphabetically by path
      otherRoutes.sort((a, b) => {
        // Put unmatched route (*) last
        if (a.isUnmatched) return 1;
        if (b.isUnmatched) return -1;
        return a.path.localeCompare(b.path);
      });

      // Combine: HTML pages first, then other routes
      return [...htmlPages, ...otherRoutes];
    },

    // API endpoints computed properties - show errors instead of silent fallbacks
    get apiEndpoints() {
      if (!this.loaded) {
        return {};
      }

      const endpoints = this.data.routes?.api_endpoints;

      if (!endpoints) {
        console.error("❌ Missing api_endpoints data from routes API");
        return {
          ERROR: [
            {
              path: "ERROR: Missing Data",
              description: "API endpoints data not available from routes API",
            },
          ],
        };
      }

      const grouped = {};

      endpoints.forEach((endpoint) => {
        if (!grouped[endpoint.method]) {
          grouped[endpoint.method] = [];
        }
        grouped[endpoint.method].push(endpoint);
      });

      // Sort endpoints alphabetically within each method group
      Object.keys(grouped).forEach((method) => {
        grouped[method].sort((a, b) => a.path.localeCompare(b.path));
      });

      return grouped;
    },

    // Config file formatted
    get configFileFormatted() {
      if (!this.loaded) {
        return null;
      }

      if (!this.data.config || Object.keys(this.data.config).length === 0) {
        return "Configuration not available";
      }

      const redacted = this.redactSecrets(this.data.config);
      return JSON.stringify(redacted, null, 2);
    },

    // NVS data formatted
    get nvsDataFormatted() {
      if (!this.loaded) {
        return null;
      }

      if (!this.data.nvs || Object.keys(this.data.nvs).length === 0) {
        console.error("❌ Missing NVS data from nvs-dump API");
        return "ERROR: NVS data not available - API failed or returned empty data";
      }

      const formattedData = {
        namespace: this.data.nvs.namespace || "ERROR: Missing namespace",
        timestamp: this.data.nvs.timestamp || "ERROR: Missing timestamp",
        summary: {
          totalKeys:
            this.data.nvs.summary?.totalKeys ?? "ERROR: Missing totalKeys",
          validKeys:
            this.data.nvs.summary?.validKeys ?? "ERROR: Missing validKeys",
          correctedKeys:
            this.data.nvs.summary?.correctedKeys ??
            "ERROR: Missing correctedKeys",
          invalidKeys:
            this.data.nvs.summary?.invalidKeys ?? "ERROR: Missing invalidKeys",
        },
        keys: {},
      };

      if (this.data.nvs.keys) {
        const sortedKeys = Object.keys(this.data.nvs.keys).sort();
        sortedKeys.forEach((key) => {
          const keyData = this.data.nvs.keys[key];
          formattedData.keys[key] = {
            type: keyData.type,
            description: keyData.description,
            exists: keyData.exists,
            value: keyData.value,
            validation: keyData.validation,
            status: keyData.status,
          };

          if (keyData.originalValue !== undefined) {
            formattedData.keys[key].originalValue = keyData.originalValue;
          }
          if (keyData.note) {
            formattedData.keys[key].note = keyData.note;
          }
          if (keyData.length !== undefined) {
            formattedData.keys[key].length = keyData.length;
          }
        });
      }

      return JSON.stringify(formattedData, null, 2);
    },

    // Quick actions
    async handleQuickAction(action) {
      try {
        // Use API layer for executing quick action
        const contentResult = await executeQuickAction(action);

        if (!contentResult.content) {
          console.error("No content received from server");
          return;
        }

        // Use API layer for printing content
        await printLocalContent(contentResult.content);
        console.log(`${action} sent to printer successfully!`);
      } catch (error) {
        console.error("Error sending quick action:", error);
      }
    },

    // Utility functions
    formatBytes(bytes) {
      if (bytes === 0) return "0 B";
      const k = 1024;
      const sizes = ["B", "KB", "MB", "GB"];
      const i = Math.floor(Math.log(bytes) / Math.log(k));
      return parseFloat((bytes / Math.pow(k, i)).toFixed(0)) + " " + sizes[i];
    },

    formatUptime(seconds) {
      const days = Math.floor(seconds / 86400);
      const hours = Math.floor((seconds % 86400) / 3600);
      const minutes = Math.floor((seconds % 3600) / 60);
      if (days > 0) return `${days}d ${hours}h ${minutes}m`;
      if (hours > 0) return `${hours}h ${minutes}m`;
      return `${minutes}m`;
    },

    formatTemperature(tempC) {
      if (!tempC || isNaN(tempC)) return "-";

      let status, color;

      if (tempC < 35) {
        status = "Cool";
        color = "#3b82f6";
      } else if (tempC < 50) {
        status = "Normal";
        color = "#10b981";
      } else if (tempC < 65) {
        status = "Warm";
        color = "#f59e0b";
      } else if (tempC < 80) {
        status = "Hot";
        color = "#ef4444";
      } else {
        status = "Critical";
        color = "#dc2626";
      }

      return `${tempC.toFixed(1)}°C (${status})`;
    },

    getProgressBarClass(percentage) {
      if (percentage > 90) return "bg-red-500";
      if (percentage > 75) return "bg-orange-600";
      return "bg-orange-500";
    },

    redactSecrets(configData) {
      const redacted = JSON.parse(JSON.stringify(configData));
      const secretKeys = [
        "password",
        "pass",
        "secret",
        "token",
        "key",
        "apikey",
        "api_key",
        "auth",
        "credential",
        "cert",
        "private",
        "bearer",
        "oauth",
      ];

      function redactObject(obj) {
        if (typeof obj !== "object" || obj === null) return obj;
        for (const [key, value] of Object.entries(obj)) {
          const lowerKey = key.toLowerCase();
          const shouldRedact = secretKeys.some((secretKey) =>
            lowerKey.includes(secretKey),
          );
          if (shouldRedact && typeof value === "string" && value.length > 0) {
            obj[key] =
              value.length > 8
                ? value.substring(0, 2) +
                  "●●●●●●●●" +
                  value.substring(value.length - 2)
                : "●●●●●●●●";
          } else if (typeof value === "object" && value !== null) {
            redactObject(value);
          }
        }
      }

      redactObject(redacted);
      return redacted;
    },

    // JSON syntax highlighting function
    highlightJSON(jsonString) {
      if (
        !jsonString ||
        jsonString === "Configuration not available" ||
        jsonString.startsWith("ERROR:")
      ) {
        return `<span class="text-gray-400">${jsonString}</span>`;
      }

      // Simple regex-based JSON highlighting
      return (
        jsonString
          // Keys (property names in quotes)
          .replace(
            /"([^"]+)"(\s*:)/g,
            '<span class="json-key">"$1"</span><span class="json-punctuation">$2</span>',
          )
          // String values (not keys)
          .replace(/:\s*"([^"]*)"/g, ': <span class="json-string">"$1"</span>')
          // Numbers
          .replace(
            /:\s*(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)/g,
            ': <span class="json-number">$1</span>',
          )
          // Booleans
          .replace(
            /:\s*(true|false)/g,
            ': <span class="json-boolean">$1</span>',
          )
          // Null
          .replace(/:\s*(null)/g, ': <span class="json-null">$1</span>')
          // Punctuation (brackets, braces, commas)
          .replace(/([{}[\],])/g, '<span class="json-punctuation">$1</span>')
          // Fix any double-wrapped punctuation
          .replace(
            /<span class="json-punctuation">(<span class="json-punctuation">)/g,
            "$1",
          )
          .replace(/(<\/span>)<\/span>/g, "$1")
      );
    },

    // Navigation
    goBack() {
      window.goBack();
    },
  };

  return store;
}
