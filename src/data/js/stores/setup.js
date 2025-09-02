/**
 * Alpine.js store factory for setup.html page (AP mode initial configuration)
 * Reuses shared functionality to maintain DRY principle
 */

import {
  loadConfiguration,
  saveConfiguration,
  scanWiFiNetworks,
} from "../api/setup.js";

export function createSetupStore() {
  return {
    // Basic state
    loaded: false, // Simple loading flag (starts false)
    error: null,
    saving: false,
    scanning: false,
    initialized: false,

    // CRITICAL: Empty object (NO pre-initialized nulls)
    config: {},

    // Manual SSID entry state
    manualSsid: "",

    // WiFi networks
    availableNetworks: [],

    // Timezone picker state (imported from settings-device.js)
    timezonePicker: {
      timezones: [],
      loading: false,
      error: null,
      initialized: false,
    },

    // Search query for timezone picker
    searchQuery: "",

    // Timezone picker UI state
    isOpen: false,
    focusedIndex: -1,

    // Load configuration on initialization
    async init() {
      if (this.initialized) return;
      this.initialized = true;

      try {
        await this.loadConfiguration();
      } catch (error) {
        console.error("Setup: Initialization error:", error);
      } finally {
        this.loaded = true; // Mark as loaded AFTER data assignment
      }
    },

    // Load configuration from server
    async loadConfiguration() {
      this.loaded = false;
      this.error = null;
      try {
        const response = await loadConfiguration();

        // ✅ CRITICAL: Direct assignment to config object
        this.config.device = {
          owner: response.device?.owner || "",
          timezone: response.device?.timezone || "",
          wifi: {
            ssid: response.device?.wifi?.ssid || "",
            password: response.device?.wifi?.password || "",
          },
        };
      } catch (error) {
        this.error = `Failed to load configuration: ${error.message}`;
      }
    },

    // WiFi scanning
    async scanWiFi() {
      if (this.scanning) return;

      this.scanning = true;
      try {
        const networks = await scanWiFiNetworks();
        this.availableNetworks = networks;
      } catch (error) {
        console.error("Setup: WiFi scan failed:", error.message);
        window.showMessage("WiFi scan failed: " + error.message, "error");
      } finally {
        this.scanning = false;
      }
    },

    // Validation for setup form
    get canSave() {
      if (!this.loaded || !this.config.device) return false;

      // Check if "Select network..." is still selected
      if (this.config.device.wifi.ssid === "") {
        return false;
      }

      const requiredFields = [
        this.config.device.owner,
        this.config.device.timezone,
        this.getEffectiveSSID(),
        this.config.device.wifi.password,
      ];

      return requiredFields.every(
        (field) =>
          field && typeof field === "string" && field.trim().length > 0,
      );
    },

    // Get the effective SSID (either selected or manual)
    getEffectiveSSID() {
      if (!this.config.device?.wifi) return "";
      if (this.config.device.wifi.ssid === "__manual__") {
        return this.manualSsid;
      }
      return this.config.device.wifi.ssid;
    },

    // ================== TIMEZONE FUNCTIONALITY ==================
    // Timezone functionality imported from settings-device.js
    async loadTimezones() {
      if (this.timezonePicker.initialized) return;
      this.timezonePicker.loading = true;
      this.timezonePicker.error = null;
      try {
        const response = await fetch("/api/timezones");
        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }
        const data = await response.json();
        if (!data.zones || !Array.isArray(data.zones)) {
          throw new Error("Invalid timezone data format");
        }
        // Transform timezone data for frontend use (same as settings-device.js)
        this.timezonePicker.timezones = data.zones.map((zone) => {
          try {
            // Extract city name from IANA ID (after last '/')
            const parts = zone.id ? zone.id.split("/") : ["Unknown"];
            const city = parts[parts.length - 1].replace(/_/g, " ");

            // Parse the offset (handle different formats)
            let offset = "";
            if (zone.currentOffset) {
              const match = zone.currentOffset.match(/([+-]\d{2})/);
              offset = match ? match[1] : zone.currentOffset;
            }

            return {
              id: zone.id || "Unknown",
              displayName: zone.location?.countryName
                ? `${city} - ${zone.location.countryName}`
                : city,
              countryName: zone.location?.countryName || "",
              comment: zone.location?.comment || "",
              aliases: zone.aliases || [],
              offset,
            };
          } catch (transformError) {
            console.error("Error transforming timezone:", zone, transformError);
            return {
              id: zone.id || "Unknown",
              displayName: zone.id || "Unknown",
              countryName: "",
              comment: "",
              aliases: [],
              offset: "",
            };
          }
        });
        this.timezonePicker.initialized = true;
        console.log(
          `Setup: Loaded ${this.timezonePicker.timezones.length} timezones`,
        );
      } catch (error) {
        this.timezonePicker.error = `Failed to load timezones: ${error.message}`;
        console.error("Setup: Timezone loading failed:", error);
      } finally {
        this.timezonePicker.loading = false;
      }
    },

    // Get display name for a timezone ID (for dropdown)
    getTimezoneDisplayName(timezoneId) {
      if (!timezoneId) return "";
      const timezone = this.timezonePicker.timezones.find(
        (tz) => tz.id === timezoneId,
      );
      if (timezone) {
        return `${timezone.displayName} (${timezone.offset})`;
      }
      return timezoneId; // Fallback to raw ID
    },

    // Get timezone offset for display
    getTimezoneOffset(timezoneId) {
      if (!timezoneId) return "";
      const timezone = this.timezonePicker.timezones.find(
        (tz) => tz.id === timezoneId,
      );
      return timezone ? timezone.offset : "";
    },

    // Filter timezones based on search query
    get filteredTimezones() {
      if (
        !Array.isArray(this.timezonePicker.timezones) ||
        this.timezonePicker.timezones.length === 0
      ) {
        return [];
      }
      const query = (this.searchQuery || "").toLowerCase().trim();
      if (!query) {
        // Show top 5 popular timezones when no search query (same as settings-device.js)
        const popularTimezones = [
          "Europe/London",
          "America/New_York",
          "America/Sao_Paulo",
          "Australia/Sydney",
          "Asia/Tokyo",
        ];
        const popular = [];
        const others = [];
        this.timezonePicker.timezones.forEach((timezone) => {
          if (popularTimezones.includes(timezone.id)) {
            popular.push(timezone);
          } else {
            others.push(timezone);
          }
        });
        // Sort popular by the order in popularTimezones array
        popular.sort((a, b) => {
          const aIndex = popularTimezones.indexOf(a.id);
          const bIndex = popularTimezones.indexOf(b.id);
          return aIndex - bIndex;
        });
        return [...popular, ...others.slice(0, 5 - popular.length)];
      }
      // Search and score results by field priority (same as settings-device.js)
      const results = [];
      this.timezonePicker.timezones.forEach((timezone) => {
        let priority = null;
        // Priority 1: City name (extracted from IANA ID)
        const parts = timezone.id.split("/");
        const city = parts[parts.length - 1].replace(/_/g, " ").toLowerCase();
        if (city.includes(query)) {
          priority = 1;
        }
        // Priority 2: Display name
        else if (timezone.displayName.toLowerCase().includes(query)) {
          priority = 2;
        }
        // Priority 3: Timezone ID
        else if (timezone.id.toLowerCase().includes(query)) {
          priority = 3;
        }
        // Priority 4: Country name
        else if (
          timezone.countryName &&
          timezone.countryName.toLowerCase().includes(query)
        ) {
          priority = 4;
        }
        // Priority 5: Comments
        else if (
          timezone.comment &&
          timezone.comment.toLowerCase().includes(query)
        ) {
          priority = 5;
        }
        if (priority !== null) {
          results.push({ timezone, priority });
        }
      });
      // Sort by priority first, then alphabetically by display name
      return results
        .sort((a, b) => {
          if (a.priority !== b.priority) {
            return a.priority - b.priority;
          }
          return a.timezone.displayName.localeCompare(b.timezone.displayName);
        })
        .map((result) => result.timezone);
    },

    // Select timezone from dropdown
    selectTimezone(timezone) {
      this.config.device.timezone = timezone.id;
      this.searchQuery = ""; // Clear search
      console.log("Setup: Selected timezone:", timezone.id);
    },

    // ================== SAVE FUNCTIONALITY ==================
    // Save configuration and restart (setup-specific behavior)
    async saveAndRestart() {
      if (!this.canSave) {
        window.showMessage("Please fill in all required fields", "error");
        return;
      }

      this.saving = true;
      try {
        if (!this.config.device) {
          throw new Error("Configuration not loaded");
        }

        // Prepare config with effective SSID
        const configToSave = {
          device: {
            owner: this.config.device.owner,
            timezone: this.config.device.timezone,
            wifi: {
              ssid: this.getEffectiveSSID(),
              password: this.config.device.wifi.password,
            },
          },
        };

        // Save configuration
        await saveConfiguration(configToSave);

        // No redirect needed - device will restart and disconnect from AP network
        // User will follow the "After Restart" instructions to reconnect
        // Keep overlay showing for a few seconds, then device will restart anyway
      } catch (error) {
        console.error("Setup: Save failed:", error.message);
        window.showMessage(
          "Failed to save configuration: " + error.message,
          "error",
        );
        this.saving = false;
      }
    },
  };
}
