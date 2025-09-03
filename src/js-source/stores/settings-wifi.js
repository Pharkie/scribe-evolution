/**
 * @file settings-wifi.js
 * @brief Alpine.js store factory for WiFi settings page
 * @description Focused Alpine store for WiFi-specific configuration
 * Converted from legacy JavaScript to ES6 modules following established patterns
 */

import {
  loadConfiguration,
  saveConfiguration,
  scanWiFiNetworks,
  printLocalContent,
} from "../api/settings.js";
import {
  createWiFiState,
  performWiFiScan,
  getEffectiveSSID,
  validateWiFiConfig,
} from "../device-config-utils/wifi-utils.js";

export function createSettingsWifiStore() {
  return {
    // ================== UTILITY FUNCTIONS ==================
    // Error helper: set inline reactive error state
    showErrorMessage(message) {
      this.error = message;
      if (this.wifiScan) this.wifiScan.error = message;
    },

    // ================== STATE MANAGEMENT ==================
    // Simple Loading Flag Pattern
    loaded: false, // Simple loading flag (starts false)
    error: null,
    saving: false,
    initialized: false, // Failsafe guard to prevent multiple inits
    apPrintStatus: "normal", // 'normal', 'scribing'

    // Computed property to check if save should be enabled
    get canSave() {
      // Don't allow save while not loaded, saving, or with errors
      if (!this.loaded || this.saving || this.error) {
        return false;
      }

      const selectedSSID = getEffectiveSSID(this.wifiScan);
      const hasValidSSID = selectedSSID && selectedSSID.trim() !== "";

      // Must have valid SSID
      let formValid = false;
      if (this.wifiScan.mode === "scan") {
        formValid = hasValidSSID && this.wifiScan.selectedNetwork;
      } else if (this.wifiScan.mode === "manual") {
        formValid = hasValidSSID;
      }

      if (!formValid) return false;

      // If password was modified, it must not be blank
      if (
        this.passwordsModified.wifiPassword &&
        (!this.config.device.wifi.password ||
          this.config.device.wifi.password.trim() === "")
      ) {
        return false;
      }

      // Must have changes to save
      return this.hasChanges();
    },

    // Configuration data (empty object populated on load)
    config: {},

    // Password modification tracking for secure handling
    passwordsModified: {
      wifiPassword: false,
    },
    originalMaskedValues: {
      wifiPassword: "",
    },

    // Store original config to detect changes
    originalConfig: {
      connectTimeout: null,
    },

    // WiFi network scanning state using shared utilities
    wifiScan: createWiFiState(),

    // Validation state
    validation: {
      errors: {},
    },

    // Computed properties for complex UI states
    get apPrintButtonText() {
      return this.apPrintStatus === "scribing"
        ? "Scribing"
        : "Scribe WiFi Fallback AP";
    },

    get saveButtonText() {
      return this.saving ? "Saving..." : "Save";
    },

    // ================== WIFI CONFIGURATION API ==================
    // Load data from server
    async loadConfiguration() {
      // Duplicate initialization guard (failsafe)
      if (this.initialized) {
        console.log("📡 WiFi Settings: Already initialized, skipping");
        return;
      }
      this.initialized = true;

      this.loaded = false;
      this.error = null;
      try {
        // Load configuration from API
        const serverConfig = await loadConfiguration();

        // Direct assignment to config object
        this.config = serverConfig;

        // Initialize WiFi scanning state
        this.initializeWiFiState();

        // Initialize password tracking
        this.initializePasswordTracking();

        console.log("📡 WiFi: Configuration loaded");
        this.loaded = true;
      } catch (error) {
        console.error("📡 WiFi: Failed to load configuration:", error);
        this.error = error.message;
      }
    },

    // Initialize password tracking
    initializePasswordTracking() {
      // Store original masked values for comparison
      this.originalMaskedValues.wifiPassword =
        this.config.device.wifi.password || "";

      // Store original timeout for change detection
      this.originalConfig.connectTimeout =
        this.config.device.wifi.connect_timeout || 15000;
    },

    // Clear password field on focus (standard UX pattern)
    clearWifiPasswordFieldOnFocus() {
      if (this.config.device.wifi.password) {
        this.config.device.wifi.password = "";
        this.passwordsModified.wifiPassword = true;
      }
    },

    // Track password modifications (called from templates)
    trackWifiPasswordChange(newValue) {
      const hasChanged = newValue !== this.originalMaskedValues.wifiPassword;
      this.passwordsModified.wifiPassword = hasChanged;
    },

    // Initialize WiFi state - simplified
    initializeWiFiState() {
      const currentSSID = this.config?.device?.wifi?.ssid || null;
      // Replace existing state with fresh state initialized with current SSID
      this.wifiScan = createWiFiState(currentSSID);
    },

    // ================== WIFI API ==================
    // WiFi scanning using shared utility
    async scanWiFiNetworks() {
      await performWiFiScan(
        this.wifiScan,
        this.showErrorMessage,
        scanWiFiNetworks,
      );
    },

    // Update SSID based on current mode and selection using shared utility
    updateSSID() {
      // Only update if config is loaded
      if (!this.config?.device?.wifi) return;

      const selectedSSID = getEffectiveSSID(this.wifiScan);
      this.config.device.wifi.ssid = selectedSSID || "";

      // Clear validation errors when SSID changes
      if (this.validation.errors["wifi.ssid"] && selectedSSID) {
        delete this.validation.errors["wifi.ssid"];
      }
    },

    // Validate password field specifically (called from UI)
    validatePassword(value) {
      // Only validate if password was modified (to avoid showing error on pre-filled masked passwords)
      if (
        this.passwordsModified.wifiPassword &&
        (!value || value.trim() === "")
      ) {
        this.validation.errors["wifi.password"] = "Password cannot be blank";
      } else {
        // Clear the error if it was previously set
        if (this.validation.errors["wifi.password"]) {
          delete this.validation.errors["wifi.password"];
        }
      }
    },

    // Validate timeout field specifically (called from UI)
    validateTimeout(value) {
      const timeoutSeconds = parseInt(value);
      if (isNaN(timeoutSeconds) || timeoutSeconds < 5 || timeoutSeconds > 60) {
        this.validation.errors["wifi.connect_timeout"] =
          "Timeout must be between 5-60 seconds";
      } else {
        // Clear the error if it was previously set
        if (this.validation.errors["wifi.connect_timeout"]) {
          delete this.validation.errors["wifi.connect_timeout"];
        }
      }
    },

    // Validate current configuration using shared utility
    validateConfiguration() {
      const wifiValidation = validateWiFiConfig(
        this.wifiScan,
        this.passwordsModified,
        this.config.device.wifi.password,
      );

      const errors = { ...wifiValidation.errors };

      // Additional timeout validation specific to settings page
      const timeoutSeconds = Math.floor(
        this.config.device.wifi.connect_timeout / 1000,
      );
      if (timeoutSeconds < 5 || timeoutSeconds > 60) {
        errors["wifi.connect_timeout"] = "Timeout must be between 5-60 seconds";
      }

      this.validation.errors = errors;
      return Object.keys(errors).length === 0;
    },

    // Check if configuration has meaningful changes using shared utility
    hasChanges() {
      const selectedSSID = getEffectiveSSID(this.wifiScan);
      const currentSSID = this.wifiScan.currentSSID;

      // SSID changed
      if (selectedSSID !== currentSSID) {
        return true;
      }

      // Password modified
      if (this.passwordsModified.wifiPassword) {
        return true;
      }

      // Timeout changed (compare current timeout vs original)
      const currentTimeout = this.config.device.wifi.connect_timeout || 15000;
      const originalTimeout = this.originalConfig.connectTimeout || 15000;
      if (currentTimeout !== originalTimeout) {
        // Only consider it a valid change if the timeout is within valid range
        const timeoutSeconds = Math.floor(currentTimeout / 1000);
        if (timeoutSeconds >= 5 && timeoutSeconds <= 60) {
          return true;
        }
      }

      return false;
    },

    // Save WiFi configuration via API
    async saveConfiguration() {
      // Ensure SSID is up to date
      this.updateSSID();

      // Validate before saving
      if (!this.validateConfiguration()) {
        this.showErrorMessage("Please fix the errors before saving");
        return;
      }

      this.saving = true;
      try {
        // Create partial WiFi config for server submission - flat structure
        const partialConfig = {
          wifi: {
            ssid: this.config.device.wifi.ssid,
            connect_timeout: this.config.device.wifi.connect_timeout,
          },
        };

        // Only include password if it was modified by user (not masked)
        if (this.passwordsModified.wifiPassword) {
          partialConfig.wifi.password = this.config.device.wifi.password;
        }

        console.log("📡 WiFi: Saving configuration");
        const message = await saveConfiguration(partialConfig);

        console.log("📡 WiFi: Configuration saved successfully");

        // Redirect immediately with success parameter
        window.location.href = "/settings/?saved=wifi";
      } catch (error) {
        console.error("📡 WiFi: Failed to save configuration:", error);
        this.showErrorMessage("Failed to save WiFi settings: " + error.message);
        this.saving = false; // Only reset on error
      }
    },

    // Cancel configuration changes
    cancelConfiguration() {
      // Navigate back to settings
      window.location.href = "/settings/";
    },

    // ================== SYSTEM/PRINTING API ==================
    // Print AP details to thermal printer
    async printAPDetails() {
      try {
        // Set scribing state
        this.apPrintStatus = "scribing";

        // Get fallback AP details from configuration - error if not available
        const fallbackSSID = this.config?.device?.wifi?.fallback_ap_ssid;
        const fallbackPassword =
          this.config?.device?.wifi?.fallback_ap_password;

        if (!fallbackSSID || !fallbackPassword) {
          throw new Error("WiFi fallback AP credentials not configured");
        }

        // Create print request with AP details
        const printRequest = {
          content_type: "memo",
          content: {
            title: "WiFi Fallback AP",
            text: `Network: ${fallbackSSID}
Password: ${fallbackPassword}

Connect to this network if device WiFi fails.

Device will be available at:
http://192.168.4.1

This memo printed from Settings → WiFi`,
          },
        };

        // Submit print request using local content
        const content = `WiFi Fallback AP
                
Network: ${fallbackSSID}
Password: ${fallbackPassword}

Connect to this network if device WiFi fails.

Device will be available at:
http://192.168.4.1

This memo printed from Settings → WiFi`;

        await printLocalContent(content);

        console.log("📡 WiFi: AP details printed");

        // Show "Scribing" for 2 seconds then revert to normal
        setTimeout(() => {
          this.apPrintStatus = "normal";
        }, 2000);
      } catch (error) {
        console.error("📡 WiFi: Failed to print AP details:", error);
        // Reset to normal state and show error to user
        this.apPrintStatus = "normal";

        this.error = `Failed to print AP details: ${error.message}`;
      }
    },
  };
}
