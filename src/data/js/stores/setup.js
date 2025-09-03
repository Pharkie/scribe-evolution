/**
 * Alpine.js store factory for setup.html page (AP mode initial configuration)
 * Reuses shared functionality to maintain DRY principle
 */

import {
  loadConfiguration,
  saveConfiguration,
  scanWiFiNetworks,
  testWiFiConnection,
} from "../api/setup.js";
import {
  createWiFiState,
  performWiFiScan,
  getEffectiveSSID,
  validateWiFiConfig,
} from "../device-config-utils/wifi-utils.js";
import {
  createTimezoneState,
  loadTimezones,
  getTimezoneDisplayName,
  getTimezoneOffset,
  filterTimezones,
  createTimezonePickerUI,
} from "../device-config-utils/timezone-utils.js";

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

    // WiFi scanning state using shared utilities
    wifiScan: createWiFiState(),

    // WiFi test gating (setup only)
    wifiTesting: false,
    wifiTestResult: null, // { success, message?, rssi? }
    wifiTestPassed: false,
    dirtySinceLastTest: false,

    // Timezone picker state using shared utilities
    timezonePicker: createTimezoneState(),

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

        // Initialize WiFi state with current SSID
        this.wifiScan.currentSSID = this.config.device.wifi.ssid;
        this.wifiScan.selectedNetwork = this.config.device.wifi.ssid;
      } catch (error) {
        this.error = `Failed to load configuration: ${error.message}`;
      }
    },

    // WiFi scanning using shared utility
    async scanWiFi() {
      if (this.wifiTesting) return;
      await performWiFiScan(
        this.wifiScan,
        (message) => {
          window.showMessage(message, "error");
        },
        scanWiFiNetworks,
      );
      this.markDirtyOnCredentialChange();
    },

    // Validation for setup form
    get canSave() {
      if (!this.loaded || !this.config.device) return false;

      const requiredFields = [
        this.config.device.owner,
        this.config.device.timezone,
        getEffectiveSSID(this.wifiScan),
        this.config.device.wifi.password,
      ];
      const baseValid = requiredFields.every(
        (field) =>
          field && typeof field === "string" && field.trim().length > 0,
      );
      if (!baseValid) return false;
      return this.wifiTestPassed && !this.dirtySinceLastTest;
    },

    // Get the effective SSID using shared utility
    getEffectiveSSID() {
      return getEffectiveSSID(this.wifiScan);
    },

    // Template compatibility - map WiFi scan state to expected properties
    get availableNetworks() {
      return this.wifiScan.networks;
    },

    get scanning() {
      return this.wifiScan.isScanning;
    },

    get hasScanned() {
      return this.wifiScan.hasScanned;
    },

    get manualSsid() {
      return this.wifiScan.manualSSID;
    },

    set manualSsid(value) {
      this.wifiScan.manualSSID = value;
      this.markDirtyOnCredentialChange();
    },

    // ================== TIMEZONE FUNCTIONALITY ==================
    // Timezone functionality using shared utilities
    async loadTimezones() {
      await loadTimezones(this.timezonePicker);
    },

    // Include shared timezone UI methods
    ...createTimezonePickerUI(),

    // Get display name for a timezone ID using shared utility
    getTimezoneDisplayName(timezoneId) {
      return getTimezoneDisplayName(timezoneId, this.timezonePicker.timezones);
    },

    // Get timezone offset for display using shared utility
    getTimezoneOffset(timezoneId) {
      return getTimezoneOffset(timezoneId, this.timezonePicker.timezones);
    },

    // Filter timezones based on search query using shared utility
    get filteredTimezones() {
      return filterTimezones(this.timezonePicker.timezones, this.searchQuery);
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

        // Prepare config with effective SSID using shared utility
        const configToSave = {
          device: {
            owner: this.config.device.owner,
            timezone: this.config.device.timezone,
            wifi: {
              ssid: getEffectiveSSID(this.wifiScan),
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

    // ================== WIFI TEST GATING ==================
    resetWifiTestState() {
      this.wifiTesting = false;
      this.wifiTestResult = null;
      this.wifiTestPassed = false;
      this.dirtySinceLastTest = false;
    },

    markDirtyOnCredentialChange() {
      this.dirtySinceLastTest = true;
      this.wifiTestPassed = false;
      // keep last result visible until user changes something; clear on explicit calls if needed
    },

    async testWifiConnection() {
      if (this.wifiTesting) return;

      const ssid = getEffectiveSSID(this.wifiScan);
      const password = this.config.device.wifi.password || "";

      // Basic base form check
      if (!ssid || !password) {
        window.showMessage("Please fill in SSID and password first", "error");
        return;
      }

      this.wifiTesting = true;
      this.wifiTestResult = null;
      try {
        const result = await testWiFiConnection(ssid, password);
        this.wifiTesting = false;
        this.wifiTestResult = result;
        if (result.success) {
          this.wifiTestPassed = true;
          this.dirtySinceLastTest = false;
        } else {
          this.wifiTestPassed = false;
          this.dirtySinceLastTest = true;
        }
      } catch (err) {
        this.wifiTesting = false;
        this.wifiTestPassed = false;
        this.dirtySinceLastTest = true;
        this.wifiTestResult = { success: false, message: err.message };
      }
    },
  };
}
