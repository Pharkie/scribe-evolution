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
import { formatSignalStrength } from "../device-config-utils/wifi-utils.js";
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

    // WiFi test gating (simple: any credential change clears pass state)
    wifiTesting: false,
    wifiTestResult: null, // { success, message?, rssi?, ssid }
    wifiTestPassed: false,
    _confettiShown: false,

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
          // Fail-fast: set reactive error state for inline banners
          this.wifiScan.error = message;
          this.error = message;
        },
        scanWiFiNetworks,
      );
      this.onWiFiCredentialsChanged();
    },

    // Validation for setup form
    get canSave() {
      if (!this.loaded || !this.config.device) return false;
      const owner = this.config.device.owner;
      const tz = this.config.device.timezone;
      const ssid = getEffectiveSSID(this.wifiScan);
      const pwd = this.config.device.wifi.password;
      if (!owner || !owner.trim() || !tz || !ssid || !pwd || !pwd.trim())
        return false;
      return this.wifiTestPassed; // credentials change auto-clears pass
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
      this.onWiFiCredentialsChanged();
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
        this.error = "Please fill in all required fields";
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
        this.error = "Failed to save configuration: " + error.message;
        this.saving = false;
      }
    },

    // ================== WIFI TEST GATING ==================
    get testWifiButtonLabel() {
      if (this.wifiTesting) return "Testing...";
      if (this.wifiTestResult) {
        if (this.wifiTestPassed) {
          const rssi = this.wifiTestResult.rssi;
          const signal =
            typeof rssi === "number" ? formatSignalStrength(rssi) : "OK";
          return `WiFi connected (Signal: ${signal})`;
        }
        return "WiFi connection failed";
      }
      return "Test WiFi";
    },
    resetWifiTestState() {
      this.wifiTesting = false;
      this.wifiTestResult = null;
      this.wifiTestPassed = false;
      this._confettiShown = false;
    },

    onWiFiCredentialsChanged() {
      // Any change invalidates previous test
      this.wifiTestResult = null;
      this.wifiTestPassed = false;
      this._confettiShown = false; // allow celebration again
    },

    async testWifiConnection() {
      if (this.wifiTesting) return;

      const ssid = getEffectiveSSID(this.wifiScan);
      const password = this.config.device.wifi.password || "";

      // Basic base form check
      if (!ssid || !password) {
        this.error = "Please fill in SSID and password first";
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
          if (
            !this._confettiShown &&
            typeof window !== "undefined" &&
            typeof window.confetti === "function"
          ) {
            // Inline SVG (Heroicons-style WiFi arcs) as image shape.
            // Decide color first, then bake it directly into the SVG stroke (no runtime recolor needed).
            const isDark =
              document.documentElement.classList.contains("dark") ||
              (window.matchMedia &&
                window.matchMedia("(prefers-color-scheme: dark)").matches);
            // Bright pink in dark mode, dark green in light mode
            const wifiColor = isDark ? "#ff2fae" : "#064e3b";
            const rawWifiSvg =
              "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>" +
              `<path stroke-linecap='round' stroke-linejoin='round' stroke-width='1.5' stroke='${wifiColor}' fill='none' d='M8.288 15.038a5.25 5.25 0 0 1 7.424 0M5.106 11.856c3.807-3.808 9.98-3.808 13.788 0M1.924 8.674c5.565-5.565 14.587-5.565 20.152 0M12.53 18.22l-.53.53-.53-.53a.75.75 0 0 1 1.06 0Z'/>` +
              "</svg>";
            const wifiSvgDataUrl =
              "data:image/svg+xml;utf8," + encodeURIComponent(rawWifiSvg);
            const btn = document.getElementById("test-wifi-button");
            let origin = { x: 0.5, y: 0.5 };
            if (btn) {
              const rect = btn.getBoundingClientRect();
              origin = {
                x: (rect.left + rect.width / 2) / window.innerWidth,
                y: (rect.top + rect.height / 2) / window.innerHeight,
              };
            }
            this._confettiShown = true;
            const base = {
              scalar: 2.4, // Larger scalar (~2x original visual area)
              spread: 70,
              particleCount: 40,
              origin,
              startVelocity: 10,
              gravity: 0.3,
              decay: 0.98,
            };
            // Burst of WiFi icon shapes
            window.confetti({
              ...base,
              shapes: ["image"],
              shapeOptions: {
                image: [
                  {
                    src: wifiSvgDataUrl,
                    replaceColor: false, // already baked color
                    width: 48,
                    height: 48,
                  },
                ],
              },
              colors: [wifiColor], // not used for image now but harmless if future mix added
            });
          }
        } else {
          this.wifiTestPassed = false;
        }
      } catch (err) {
        this.wifiTesting = false;
        this.wifiTestPassed = false;
        this.wifiTestResult = { success: false, message: err.message };
      }
    },
  };
}
