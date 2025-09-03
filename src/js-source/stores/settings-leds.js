/**
 * @file settings-leds.js
 * @brief Alpine.js store factory for LED settings page
 * @description Focused Alpine store for LED-specific configuration
 * Converted from legacy JavaScript to ES6 modules following established patterns
 */

import {
  loadConfiguration,
  saveConfiguration,
  triggerLedEffect,
  turnOffLeds,
} from "../api/settings.js";

export function createSettingsLedsStore() {
  return {
    // ================== STATE MANAGEMENT ==================
    loaded: false, // Simple loading flag (starts false)
    error: null,
    saving: false,
    initialized: false,
    config: {}, // Empty object (populated on load)

    // Effects playground (temporary, not saved to config)
    effect: null,
    speed: null,
    intensity: null,
    cycles: null,
    colors: [],
    originalConfig: {},
    testingEffect: false,

    // Color presets for swatch selection
    colorPresets: [
      "#0062ff", // Blue
      "#ff0000", // Red
      "#00ff00", // Green
      "#ff00ff", // Magenta
      "#ffff00", // Yellow
      "#00ffff", // Cyan
    ],

    // ================== COMPUTED PROPERTIES ==================
    get canSave() {
      return !this.saving && this.loaded && !this.error && this.hasChanges;
    },

    get hasChanges() {
      // Only check system settings that are actually saved (not playground effects)
      const currentSystemSettings = {
        count: this.config.leds?.count,
        brightness: this.config.leds?.brightness,
        refreshRate: this.config.leds?.refreshRate,
      };
      const originalSystemSettings = {
        count: this.originalConfig.leds?.count,
        brightness: this.originalConfig.leds?.brightness,
        refreshRate: this.originalConfig.leds?.refreshRate,
      };
      return (
        JSON.stringify(currentSystemSettings) !==
        JSON.stringify(originalSystemSettings)
      );
    },

    // ================== LED CONFIGURATION API ==================
    async loadConfiguration() {
      // Duplicate initialization guard (failsafe)
      if (this.initialized) {
        return;
      }
      this.initialized = true;

      this.loaded = false;
      this.error = null;
      try {
        const data = await loadConfiguration();

        // ✅ CRITICAL: Direct assignment to config object
        this.config.leds = {
          gpio: data.leds?.gpio ?? -1,
          count: data.leds?.count ?? 30,
          effect: data.leds?.effect ?? "chase_single",
          brightness: data.leds?.brightness ?? 128,
          refreshRate: data.leds?.refreshRate ?? 60,
          speed: data.leds?.speed ?? 50,
          intensity: data.leds?.intensity ?? 50,
          cycles: data.leds?.cycles ?? 3,
          colors: data.leds?.colors ?? ["#0062ff", "#ff0000", "#00ff00"],
        };

        this.originalConfig = {
          leds: JSON.parse(JSON.stringify(this.config.leds)),
        };

        // Ensure colors array exists and has at least 3 colors
        if (!Array.isArray(this.colors) || this.colors.length < 3) {
          this.colors = ["#0062ff", "#ff0000", "#00ff00"];
        }

        this.loaded = true; // Mark as loaded AFTER data assignment
      } catch (error) {
        this.error = `Failed to load configuration: ${error.message}`;
      }
    },

    async saveConfiguration() {
      if (!this.canSave) {
        return;
      }

      this.saving = true;
      this.error = null;

      try {
        // Create partial config with ONLY LED system settings (exclude effect playground params)
        const partialConfig = {
          leds: {
            count: this.config.leds.count,
            brightness: this.config.leds.brightness,
            refreshRate: this.config.leds.refreshRate,
          },
        };

        console.log("Saving partial LED configuration:", partialConfig);
        const message = await saveConfiguration(partialConfig);

        // Success - update original config for system settings only (not playground effects)
        this.originalConfig.leds = this.originalConfig.leds || {};
        this.originalConfig.leds.count = this.config.leds.count;
        this.originalConfig.leds.brightness = this.config.leds.brightness;
        this.originalConfig.leds.refreshRate = this.config.leds.refreshRate;

        // Redirect to settings overview with success parameter (no timeout)
        window.location.href = "/settings/?saved=leds";
      } catch (error) {
        console.error("Error saving configuration:", error);
        this.error = error.message || "Failed to save configuration";
        this.showErrorMessage(this.error);
        this.saving = false;
      }
    },

    cancelConfiguration() {
      window.location.href = "/settings/";
    },

    // ================== COLOR CONTROL FUNCTIONS ==================
    showColorControls() {
      return (
        this.config.leds &&
        (this.effect === "chase_single" ||
          this.effect === "chase_multi" ||
          this.effect === "matrix" ||
          this.effect === "twinkle" ||
          this.effect === "pulse")
      );
    },

    updateEffectParams() {
      // Reset colors based on C++ config requirements
      if (this.effect === "chase_single") {
        this.colors = ["#0062ff"]; // Blue (DEFAULT_CHASE_SINGLE_COLOR)
      } else if (this.effect === "chase_multi") {
        this.colors = ["#ff0000", "#00ff00", "#0000ff"]; // Red, Green, Blue
      } else if (this.effect === "matrix") {
        this.colors = ["#00ff00"]; // Green (DEFAULT_MATRIX_COLOR)
      } else if (this.effect === "twinkle") {
        this.colors = ["#ffff00"]; // Yellow (DEFAULT_TWINKLE_COLOR)
      } else if (this.effect === "pulse") {
        this.colors = ["#800080"]; // Purple (DEFAULT_PULSE_COLOR)
      } else if (this.effect === "rainbow") {
        // Rainbow effect uses no colors - procedurally generated
        this.colors = [];
      }
    },

    // ================== LED EFFECT TESTING ==================
    async testLedEffect() {
      if (this.testingEffect) return;

      this.testingEffect = true;
      try {
        // Create effect parameters object (exclude system settings like brightness/refreshRate)
        let colors = this.colors || ["#0062ff"];

        // Only send relevant colors based on C++ config requirements
        if (
          this.effect === "chase_single" ||
          this.effect === "matrix" ||
          this.effect === "twinkle" ||
          this.effect === "pulse"
        ) {
          // Single color effects
          colors = [colors[0]];
        } else if (this.effect === "chase_multi") {
          // Multi color effects need exactly 3 colors
          colors = colors.slice(0, 3);
        } else if (this.effect === "rainbow") {
          // Rainbow effect uses no colors
          colors = [];
        }

        const effectParams = {
          effect: this.effect,
          speed: this.speed || 50,
          intensity: this.intensity || 50,
          cycles: this.cycles || 3,
          colors: colors,
        };

        console.log("Testing LED effect:", effectParams);

        // Use triggerLedEffect from API
        await triggerLedEffect(effectParams);
      } catch (error) {
        console.error("Failed to test LED effect:", error);
        this.showErrorMessage("Failed to test LED effect: " + error.message);
      } finally {
        this.testingEffect = false;
      }
    },

    async turnOffLeds() {
      try {
        console.log("Turning off LEDs");
        await turnOffLeds();
      } catch (error) {
        console.error("Failed to turn off LEDs:", error);
        this.showErrorMessage("Failed to turn off LEDs: " + error.message);
      }
    },

    // ================== UTILITY FUNCTIONS ==================
    showErrorMessage(message) {
      this.error = message;
    },
  };
}
